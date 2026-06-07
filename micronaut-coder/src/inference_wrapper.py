"""
MM-CODER Inference Wrapper

Wraps scx_runtime.exe (semantic kernel) as HTTP chat endpoint.
Maps code output -> custom glyph phases via token signal generator.
"""

import json
import subprocess
import sys
from pathlib import Path
from flask import Flask, request, jsonify
from typing import Dict, Any, Optional
import hashlib
from datetime import datetime

# Get paths
SCRIPT_DIR = Path(__file__).parent
KERNEL_ROOT = Path(r"E:\data\smgm16_gpu_bridge\versions\cpu_thread_cluster_v1\native\scx_runtime\build-local\Release")
SCX_RUNTIME = KERNEL_ROOT / "scx_runtime.exe"

# Import token signal generator (will be created)
sys.path.insert(0, str(SCRIPT_DIR))


class InferenceEngine:
    """Wraps scx_runtime.exe semantic kernel."""

    def __init__(self, runtime_path: Path = SCX_RUNTIME):
        """Initialize inference engine."""
        self.runtime_path = Path(runtime_path)
        if not self.runtime_path.exists():
            raise FileNotFoundError(f"scx_runtime.exe not found at {self.runtime_path}")

        self.cache = {}
        print(f"[MM-CODER] Semantic kernel ready: {self.runtime_path}")

    def infer(self, prompt: str, domain: str = "D1") -> Dict[str, Any]:
        """
        Execute prompt through semantic kernel.

        Args:
            prompt: Code prompt or instruction
            domain: KXML domain (D0-D5)

        Returns:
            {
                "response": str,
                "tokens": list[int],
                "glyph_phases": list[str],
                "metadata": {...}
            }
        """
        try:
            # Call scx_runtime with prompt
            # Format: scx_runtime.exe --prompt "<prompt>" --domain <domain>
            result = subprocess.run(
                [str(self.runtime_path), "--prompt", prompt, "--domain", domain],
                capture_output=True,
                text=True,
                timeout=30
            )

            if result.returncode != 0:
                return {
                    "status": "error",
                    "error": result.stderr or "Inference failed",
                    "response": "",
                    "tokens": [],
                    "glyph_phases": []
                }

            # Parse output JSON from scx_runtime
            output_data = json.loads(result.stdout)

            return {
                "status": "success",
                "response": output_data.get("text", ""),
                "tokens": output_data.get("tokens", []),
                "glyph_phases": output_data.get("glyph_phases", []),
                "metadata": {
                    "model": "gpt2-coder-finetuned",
                    "domain": domain,
                    "inference_ms": output_data.get("inference_ms", 0),
                    "token_count": len(output_data.get("tokens", []))
                }
            }

        except subprocess.TimeoutExpired:
            return {
                "status": "error",
                "error": "Inference timeout (30s)",
                "response": "",
                "tokens": [],
                "glyph_phases": []
            }
        except json.JSONDecodeError as e:
            return {
                "status": "error",
                "error": f"Invalid JSON from kernel: {e}",
                "response": result.stdout[:200] if result.stdout else "",
                "tokens": [],
                "glyph_phases": []
            }
        except Exception as e:
            return {
                "status": "error",
                "error": str(e),
                "response": "",
                "tokens": [],
                "glyph_phases": []
            }


# Initialize Flask app
app = Flask(__name__)
engine = InferenceEngine()


@app.route("/health", methods=["GET"])
def health():
    """Health check endpoint."""
    return jsonify({
        "status": "ok",
        "service": "MM-CODER",
        "version": "1.0",
        "kernel": "scx_runtime.exe"
    })


@app.route("/chat", methods=["POST"])
def chat():
    """
    Chat endpoint: prompt -> code generation + glyph execution

    Request:
        {
            "prompt": "write python code for...",
            "domain": "D1" (optional, defaults to D1)
        }

    Response:
        {
            "response": "generated code",
            "tokens": [...],
            "glyph_phases": ["Pop", "Wo", "Yax", ...],
            "metadata": {...}
        }
    """
    data = request.get_json() or {}
    prompt = data.get("prompt", "")
    domain = data.get("domain", "D1")

    if not prompt:
        return jsonify({"error": "Missing prompt"}), 400

    result = engine.infer(prompt, domain)

    # Add request metadata
    result["request_id"] = hashlib.md5(prompt.encode()).hexdigest()[:16]
    result["timestamp"] = datetime.utcnow().isoformat()

    status_code = 200 if result.get("status") == "success" else 400
    return jsonify(result), status_code


@app.route("/tokens", methods=["POST"])
def tokenize():
    """
    Tokenize endpoint: code -> token IDs

    Request: {"code": "..."}
    Response: {"tokens": [...], "count": N}
    """
    data = request.get_json() or {}
    code = data.get("code", "")

    if not code:
        return jsonify({"error": "Missing code"}), 400

    # Use scx_runtime to tokenize
    result = engine.infer(code, domain="D1")

    return jsonify({
        "tokens": result.get("tokens", []),
        "count": len(result.get("tokens", [])),
        "status": result.get("status")
    })


@app.route("/info", methods=["GET"])
def info():
    """API documentation."""
    return jsonify({
        "name": "MM-CODER",
        "description": "GPT-2 code generation micronaut with glyph phase routing",
        "endpoints": {
            "GET /health": "Health check",
            "POST /chat": "Code generation (prompt -> response + glyphs)",
            "POST /tokens": "Tokenization (code -> token IDs)",
            "GET /info": "This endpoint"
        },
        "domains": ["D0", "D1", "D2", "D3", "D4", "D5"],
        "kernel": "scx_runtime.exe"
    })


def main():
    """Start MM-CODER inference server."""
    import argparse

    parser = argparse.ArgumentParser(description="MM-CODER Inference Server")
    parser.add_argument("--port", type=int, default=25500, help="HTTP port (default: 25500)")
    parser.add_argument("--host", default="127.0.0.1", help="HTTP host (default: 127.0.0.1)")
    parser.add_argument("--debug", action="store_true", help="Enable debug logging")

    args = parser.parse_args()

    print("")
    print("========================================")
    print("MM-CODER INFERENCE SERVER")
    print("========================================")
    print(f"Host: {args.host}:{args.port}")
    print(f"Kernel: {SCX_RUNTIME}")
    print(f"Debug: {args.debug}")
    print("")
    print("Endpoints:")
    print(f"  POST {args.host}:{args.port}/chat")
    print(f"  GET  {args.host}:{args.port}/health")
    print("")

    app.run(host=args.host, port=args.port, debug=args.debug)


if __name__ == "__main__":
    main()
