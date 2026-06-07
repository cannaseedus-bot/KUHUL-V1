"""
Integration test suite for K'UHUL Gold Mine Integration (Tiers 1-4)
"""

import sys
from pathlib import Path

# Add config to path
config_path = Path(__file__).parent / "config"
sys.path.insert(0, str(config_path))

training_path = Path(__file__).parent / "micronaut" / "training"
sys.path.insert(0, str(training_path))

micronaut_path = Path(__file__).parent / "micronaut"
sys.path.insert(0, str(micronaut_path))


def test_config_loader():
    """Test Tier 1-2: Config system loads and validates"""
    from config_loader import initialize
    loader = initialize()
    assert loader.get_all_personas()
    assert len(loader.get_all_personas()) == 10, "Should have 10 personas (6 KXML + 4 legacy)"
    assert len(loader.get_all_tools()) == 20, "Should have 20 tools"
    assert len(loader.get_all_domains()) == 6, "Should have 6 KXML domains (D0-D5)"
    print("[PASS] config_loader: 10 personas, 20 tools, 6 domains")


def test_kxml_domains():
    """Test Tier 1: KXML domain classification"""
    from kxml_domain_loader import classify_domain, get_all_domains

    domains = get_all_domains()
    assert len(domains) == 6, "Should have 6 canonical domains"

    # Test classification
    domain_id, conf = classify_domain("kuhul glyph fold")
    assert domain_id == 0, "Should classify to D0 (kuhul_source_semantic)"
    assert conf > 0.2, "Should have positive confidence"
    print("[PASS] kxml_domains: 6 domains, classification working")


def test_glyph_routing():
    """Test Tier 1: Glyph opcode routing"""
    from kxml_domain_loader import get_glyph_for_tool, get_micronaut_target

    glyph = get_glyph_for_tool("python")
    assert glyph == "U+E010", "python tool should map to U+E010"

    target = get_micronaut_target(glyph)
    assert target == "PYIDE-1", "U+E010 should target PYIDE-1"
    print("[PASS] glyph_routing: tool-glyph-target chain working")


def test_tool_dispatcher():
    """Test Tier 2: Tool dispatcher with real code executor"""
    from tool_dispatcher import dispatch_tool

    result = dispatch_tool("code_executor", {"code": "print(42)"})
    assert result.get("status") == "success", "code_executor should succeed"
    assert "42" in result.get("stdout", ""), "Should execute and print 42"
    print("[PASS] tool_dispatcher: code_executor working")


def test_asx_proof_chain():
    """Test Tier 3: ASX proof chain for training auditability"""
    from asx_proof import TrainingSession

    session = TrainingSession("test_001")
    session.open({"epochs": 1})
    session.tick(epoch=0, shard=0, loss=5.0)
    session.close(final_loss=3.5)

    result = session.verify()
    assert result["ok"], "Proof chain should verify"
    assert result["count"] == 3, "Should have 3 events (open, tick, close)"
    print("[PASS] asx_proof_chain: proof chain generated and verified")


def test_training_modules():
    """Test Tier 3: Training pipeline modules import"""
    import train_loop
    import tokenize_dataset
    import split_tokens

    # Verify path updates worked
    assert hasattr(train_loop, 'run_trainer'), "train_loop should have run_trainer"
    assert hasattr(tokenize_dataset, 'main'), "tokenize_dataset should have main"
    print("[PASS] training_modules: all modules import successfully")


def test_native_glyph_runtime():
    """Test Tier 1b: Native glyph runtime path fix"""
    from native_glyph_runtime import KuhulDirectXNative

    runtime = KuhulDirectXNative()
    # Check that path was updated correctly
    assert "v0.2.0-kuhul-directx-native" in str(runtime.native_engine)
    assert runtime.native_engine.exists(), "native_glyph_engine.exe should exist"
    print("[PASS] native_glyph_runtime: path fix verified, exe exists")


def test_shader_files():
    """Test Tier 3-4: HLSL shaders installed"""
    shaders_path = Path(__file__).parent / "native" / "shaders"

    gpt2_shaders = list((shaders_path / "gpt2").glob("*.hlsl"))
    moe_shaders = list((shaders_path / "moe").glob("*.hlsl"))

    assert len(gpt2_shaders) == 13, f"Should have 13 GPT-2 shaders, found {len(gpt2_shaders)}"
    assert len(moe_shaders) >= 1, "Should have at least 1 MoE shader"
    print("[PASS] shader_files: 13 GPT-2 + MoE shaders present")


def test_cpp_sources():
    """Test Tier 4: C++ compiler sources in native build"""
    scx_src = Path(__file__).parent / "native" / "scx_runtime" / "src"

    scxgraph = scx_src / "scxgraph_to_xvm.cpp"
    kbc1 = scx_src / "kbc1_compiler.cpp"

    assert scxgraph.exists(), "scxgraph_to_xvm.cpp should exist"
    assert kbc1.exists(), "kbc1_compiler.cpp should exist"
    print("[PASS] cpp_sources: compiler sources integrated")


def test_ebnf_grammar():
    """Test Tier 4: EBNF grammar updated with KXML"""
    grammar_path = Path(__file__).parent / "releases" / "Kuhul-PY" / "engine3d" / "grammar" / "kuhul_3d_v3.ebnf"

    content = grammar_path.read_text()
    assert "KXMLDomain" in content, "Grammar should have KXMLDomain production"
    assert "AdapterShard" in content, "Grammar should have AdapterShard production"
    assert "DDS_SHARD_" in content, "Grammar should reference DDS shards"
    print("[PASS] ebnf_grammar: KXML domain productions added")


if __name__ == "__main__":
    tests = [
        test_config_loader,
        test_kxml_domains,
        test_glyph_routing,
        test_tool_dispatcher,
        test_asx_proof_chain,
        test_training_modules,
        test_native_glyph_runtime,
        test_shader_files,
        test_cpp_sources,
        test_ebnf_grammar,
    ]

    print("\n" + "=" * 70)
    print("K'UHUL UNIFIED SYSTEM INTEGRATION TEST")
    print("=" * 70 + "\n")

    passed = 0
    failed = 0

    for test in tests:
        try:
            test()
            passed += 1
        except Exception as e:
            print(f"[FAIL] {test.__name__}: {e}")
            failed += 1

    print("\n" + "=" * 70)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 70)

    if failed == 0:
        print("\n[SUCCESS] All integration tests passed!")
        sys.exit(0)
    else:
        print(f"\n[ERROR] {failed} test(s) failed")
        sys.exit(1)
