"""
Token Signal Generator for MM-CODER

Maps generated code tokens -> custom glyph phases based on:
- Code type (function, class, loop, etc.)
- Domain (D0-D5)
- Semantic intent (numatic @numatics.kuhul_phases)

Custom mapping (not direct from numatic, but informed by it):
- Pop (output): function/class definitions
- Wo (write): variable assignments, I/O
- Yax (conditional): if/elif/else, try/except
- Sek (semantics): imports, type hints, docstrings
- Ch'en (read): function calls, attribute access
- Xul (terminate): return/break/continue, exit
"""

import re
from typing import List, Dict, Any, Optional
from enum import Enum


class GlyphPhase(Enum):
    """K'UHUL canonical glyph phases."""
    POP = "Pop"  # Output: function/class definitions
    WO = "Wo"  # Write: assignments, I/O
    YAX = "Yax"  # Conditional: if/else, try/except
    SEK = "Sek"  # Semantics: imports, types
    CHEN = "Ch'en"  # Read: function calls
    XUL = "Xul"  # Terminate: return/break


class TokenSignalGenerator:
    """Maps code tokens to glyph phases."""

    def __init__(self):
        """Initialize pattern matchers."""
        # Python code patterns
        self.patterns = {
            GlyphPhase.POP: [
                r'^def\s+',  # function def
                r'^class\s+',  # class def
                r'^async\s+def\s+',  # async function
            ],
            GlyphPhase.SEK: [
                r'^import\s+',  # imports
                r'^from\s+.*\s+import',  # from imports
                r':\s*\w+\s*=',  # type hints
                r'""".*"""',  # docstrings
                r"'''.*'''",  # docstrings
            ],
            GlyphPhase.WO: [
                r'=\s*(?!.*==)',  # assignment (not comparison)
                r'print\(',  # output
                r'\.write\(',  # file write
                r'\.append\(',  # list mutation
            ],
            GlyphPhase.YAX: [
                r'^if\s+',  # if statement
                r'^elif\s+',  # elif
                r'^else\s*:',  # else
                r'^try\s*:',  # try
                r'^except\s+',  # except
                r'^for\s+',  # for loop
                r'^while\s+',  # while loop
            ],
            GlyphPhase.CHEN: [
                r'\w+\(',  # function call
                r'\.\w+\(',  # method call
                r'\[\w+\]',  # indexing
                r'\.\w+(?!\()',  # attribute access
            ],
            GlyphPhase.XUL: [
                r'^return\s+',  # return
                r'^break\s*$',  # break
                r'^continue\s*$',  # continue
                r'^raise\s+',  # raise
                r'^exit\(',  # exit
            ],
        }

        # Compile patterns for speed
        self.compiled_patterns = {}
        for phase, patterns in self.patterns.items():
            self.compiled_patterns[phase] = [
                re.compile(p, re.MULTILINE) for p in patterns
            ]

    def classify_line(self, line: str) -> Optional[GlyphPhase]:
        """Classify a single line of code to a glyph phase."""
        line = line.strip()
        if not line or line.startswith("#"):
            return None

        # Check each phase's patterns
        for phase, compiled_patterns in self.compiled_patterns.items():
            for pattern in compiled_patterns:
                if pattern.search(line):
                    return phase

        return None

    def generate_phases(self, code: str) -> List[str]:
        """
        Generate sequence of glyph phases from code.

        Args:
            code: Python code snippet

        Returns:
            List of glyph phase names (e.g., ["Sek", "Pop", "Wo", "Yax", ...])
        """
        phases = []

        for line in code.split("\n"):
            phase = self.classify_line(line)
            if phase:
                phases.append(phase.value)

        # If no phases found, default sequence
        if not phases:
            phases = [GlyphPhase.SEK.value, GlyphPhase.POP.value, GlyphPhase.WO.value]

        return phases

    def generate_with_context(
        self,
        code: str,
        domain: str = "D1",
        numatic_phases: Optional[List[str]] = None
    ) -> Dict[str, Any]:
        """
        Generate glyph phases with domain and numatic context.

        Args:
            code: Generated code
            domain: KXML domain (D1=semantic_code, D2=algorithms, D4=gpu)
            numatic_phases: Optional phases from numatic @numatics.kuhul_phases

        Returns:
            {
                "phases": ["Pop", "Wo", "Yax", ...],
                "primary_phase": "Pop",
                "domain_mapping": {"D1": "semantic_code_math", ...},
                "numatic_influence": [list of phases from numatic if provided]
            }
        """
        phases = self.generate_phases(code)

        # Domain-specific routing
        domain_map = {
            "D0": "kuhul_source_semantic",
            "D1": "semantic_code_math",
            "D2": "algorithmic_code",
            "D3": "mathematical_analysis",
            "D4": "gpu_shader_code",
            "D5": "cryptographic_math",
        }

        # If domain is D4 (GPU), prioritize computational phases
        if domain == "D4":
            phases = [
                GlyphPhase.SEK.value,  # Import CUDA/OpenGL
                GlyphPhase.POP.value,  # Define kernel
                GlyphPhase.WO.value,  # Memory alloc
                GlyphPhase.CHEN.value,  # Call kernel
                GlyphPhase.XUL.value,  # Cleanup
            ]

        return {
            "phases": phases,
            "primary_phase": phases[0] if phases else None,
            "domain": domain,
            "domain_name": domain_map.get(domain, "unknown"),
            "numatic_phases": numatic_phases or [],
            "code_lines": len(code.split("\n")),
            "phase_sequence": " -> ".join(phases),
        }


# Global generator instance
_generator = None


def get_generator() -> TokenSignalGenerator:
    """Get or create the global token signal generator."""
    global _generator
    if _generator is None:
        _generator = TokenSignalGenerator()
    return _generator


def classify_code(code: str, domain: str = "D1") -> Dict[str, Any]:
    """
    Classify code to glyph phases.

    Convenience function wrapping TokenSignalGenerator.

    Args:
        code: Python code
        domain: KXML domain

    Returns:
        Glyph phase mapping
    """
    gen = get_generator()
    return gen.generate_with_context(code, domain)


# Test
if __name__ == "__main__":
    gen = TokenSignalGenerator()

    test_code = """
import numpy as np

def matrix_multiply(A, B):
    '''Multiply two matrices.'''
    result = np.zeros((A.shape[0], B.shape[1]))
    for i in range(A.shape[0]):
        for j in range(B.shape[1]):
            for k in range(A.shape[1]):
                result[i][j] += A[i][k] * B[k][j]
    return result
"""

    print("[TEST] Token Signal Generator")
    print("=" * 60)
    print(f"Code:\n{test_code}")
    print("\nAnalysis:")
    result = gen.generate_with_context(test_code, "D2")
    print(f"  Domain: {result['domain_name']}")
    print(f"  Phases: {' -> '.join(result['phases'])}")
    print(f"  Primary: {result['primary_phase']}")
    print(f"  Lines: {result['code_lines']}")
