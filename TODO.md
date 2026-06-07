# MM-CODER Build Plan

**Goal:** Build the first real micronaut — GPT-2 coder fine-tuned on code + native C++ inference engine

**Timeline estimate:** 5-7 days (depends on training corpus size and iteration cycles)

---

## Phase 1: Prepare & Fine-Tune (2-3 days)

### 1.1 Prepare coding corpus
- [ ] **CODER DATA** (already available):
  - Train: `E:\data\smgm16_gpu_bridge\coder_outputs.train.jsonl` (1.8 GB, 158K records)
  - Val: `E:\data\smgm16_gpu_bridge\coder_outputs.val.jsonl` (207 MB, 17K records)
  - Format: `{"chat_format", "domain", "messages": [{"role", "content"}], "output", ...}`
  - Has conversation history + code domain labels
  
- [ ] **MAP NUMATIC DATA** to KXML domains:
  - Source files: `E:\data\combined_train2.jsonl.numatic`, `foundation_merged.jsonl.numatic`, etc. (9 files, 3.4 GB)
  - Format: `{"input", "output", "@numatics": {"semantic_fold", "kuhul_phases", "habitat", ...}}`
  - Already annotated with glyph phases (Pop, Wo, Yax, Sek, Ch'en, Xul)
  - Map `@numatics.semantic_fold` → KXML domain (D0-D5)
  - Extract `kuhul_phases` for token signal generator routing
  
- [ ] **CREATE TRAINING CURRICULUM**:
  - Reformat coder data: `{"prompt": "...", "response": "...", "curriculum_bucket": "easy|medium|hard", "domain": "D1|D2|D4"}`
  - Bucket by: message count (easy=1-2 turns, medium=3-5, hard=5+)
  - Output: `models/smgm-16/scxq2_dds_folds/coder_curriculum.jsonl`
  
- [ ] Tokenize via `micronaut/training/tokenize_dataset.py`
  - Input: coder_curriculum.jsonl
  - Output: `models/smgm-16/scxq2_dds_folds/coder_tokens.bin`

### 1.2 Fine-tune GPT-2 on coding data
- [ ] Run `micronaut/training/train_loop.py` with coder curriculum
  - Epochs: 3-5 (depending on convergence)
  - Batch: 8 (Intel HD 4600 shared VRAM cap)
  - LR: 1e-5
- [ ] Output checkpoint: `khl/checkpoint/gpt2_coder_finetuned.pt`
- [ ] Generate ASX proof chain for audit trail
- [ ] Convert to GGUF via `releases/v0.1.1/.../convert-to-gguf.py`
  - Output: `models/gguf/gpt2_coder.Q8_0.gguf`

### 1.3 Validate fine-tune
- [ ] Run inference on held-out test set
- [ ] Verify code generation quality
- [ ] Log metrics: perplexity, generation speed, memory usage
- [ ] Save checkpoint + metrics to `khl/checkpoint/mm-coder-finetuned.json`

---

## Phase 2: Semantic Kernel Integration (1 day)

### 2.1 Python HTTP Wrapper (inference_wrapper.py)
- [x] **CREATED** `micronaut-coder/src/inference_wrapper.py`
  - Wraps `scx_runtime.exe` (semantic kernel) as HTTP server
  - Endpoints: `POST /chat`, `GET /health`, `POST /tokens`
  - Runs scx_runtime subprocess with prompt + domain
  - Parses JSON output (text, tokens, glyph_phases)
  - Listen on port 25500

### 2.2 Token Signal Generator (token_signal_generator.py)
- [x] **CREATED** `micronaut-coder/src/token_signal_generator.py`
  - Custom mapping: code tokens → glyph phases
  - Patterns for 6 canonical phases:
    - `Pop` (output): function/class definitions
    - `Wo` (write): assignments, I/O
    - `Yax` (conditional): if/else, loops, try/except
    - `Sek` (semantic): imports, type hints
    - `Ch'en` (read): function calls, attribute access
    - `Xul` (terminate): return, break, raise
  - Domain-aware routing (D0-D5)
  - Tested: matrix_multiply code → `Sek -> Pop -> Wo -> Yax -> Xul`

### 2.3 Integrate Semantic Kernel
- [ ] **SEMANTIC KERNEL AVAILABLE**: `E:\data\smgm16_gpu_bridge\versions\cpu_thread_cluster_v1\native\scx_runtime\build-local\Release\scx_runtime.exe`
- [ ] **COMPILED SHADERS**: `E:\data\smgm16_gpu_bridge\versions\cpu_thread_cluster_v1\native\xvm-d3d12\compiled_shaders/` (20+ .cso files)
- [ ] Update inference_wrapper.py to call scx_runtime with:
  - `--prompt "<code prompt>"`
  - `--domain "D1|D2|D4|..."`
  - Expect JSON: `{"text": "...", "tokens": [...], "glyph_phases": [...]}`

### 2.4 Test integration
- [ ] Start inference server: `python inference_wrapper.py --port 25500`
- [ ] HTTP test: `curl -X POST http://127.0.0.1:25500/chat -d '{"prompt": "def factorial(n): ...", "domain": "D1"}'`
- [ ] Verify glyph phase sequence from output

---

## Phase 3: Registration & Integration (1 day)

### 3.1 Register MM-CODER in micronaut system
- [ ] Create `micronaut/micronaut.registry.xjson` (canonical registry)
  ```json
  {
    "micronauts": [
      {
        "id": "MM-CODER",
        "name": "CoderMicronaut",
        "port": 25500,
        "type": "MM-1",
        "variant": "token_signal_generator",
        "fold": "COMPUTE_FOLD",
        "model": "gpt2_coder_finetuned.pt",
        "capabilities": ["code_generation", "code_analysis", "fold_execution"],
        "domains": ["D1", "D2", "D4"]
      }
    ]
  }
  ```
- [ ] Register with coordinator on startup
- [ ] Add to `micronaut/launch-coordinator-and-services.ps1` as background job

### 3.2 Wire into config system
- [ ] Update `config/kxml_domain_loader.py`
  - Add MM-CODER routing for D1 (semantic code), D2 (algorithms), D4 (GPU code)
  - Map glyph opcodes to MM-CODER port
- [ ] Update `config/tool_dispatcher.py`
  - Route "code_executor" to MM-CODER HTTP endpoint
  - Fallback chain: MM-CODER → sandboxed Python → error

### 3.3 Wire into reply pipeline
- [ ] Update `config/reply_pipeline.py`
  - For code-related queries, route to MM-CODER via tool_dispatcher
  - Merge glyph results back into response
- [ ] Test: `python config/reply_pipeline.py` with code prompts

### 3.4 Build MM-MATH & MM-TOOLCALL (after MM-CODER proof)
- [ ] MM-MATH: GPT-2 fine-tuned on math problems (D3)
- [ ] MM-TOOLCALL: GPT-2 medium + ARC attention for tool calling (D0, D5)
- [ ] Same pipeline as MM-CODER (fine-tune → native engine → register)

---

## Testing & Validation

### 4.1 Unit tests
- [ ] Token signal generator (glyph mapping)
- [ ] GPU shader dispatch (matmul, attention)
- [ ] GGUF loader (weights shape, quantization)

### 4.2 Integration tests
- [ ] End-to-end: prompt → MM-CODER → glyph → response
- [ ] Performance: tokens/sec on Intel HD 4600
- [ ] Accuracy: held-out test set perplexity < 20

### 4.3 System tests
- [ ] MM-CODER registered with coordinator
- [ ] Config system routes code queries to MM-CODER
- [ ] Reply pipeline merges glyph results
- [ ] Splash UI shows MM-CODER online

---

## Artifacts & Checkpoints

| Artifact | Owner | Status |
|----------|-------|--------|
| `models/smgm-16/scxq2_dds_folds/coder_tokens.bin` | Phase 1.1 | Pending |
| `khl/checkpoint/gpt2_coder_finetuned.pt` | Phase 1.2 | Pending |
| `models/gguf/gpt2_coder.Q8_0.gguf` | Phase 1.2 | Pending |
| `micronaut-coder/src/main.cpp` | Phase 2.1 | Pending |
| `micronaut-coder/src/token_signal_generator.cpp` | Phase 2.2 | Pending |
| `micronaut/micronaut.registry.xjson` | Phase 3.1 | Pending |
| Integration test suite | Phase 4.2 | Pending |

---

## Dependencies & Blockers

- ✅ Training pipeline ready (`train_loop.py`, tokenize_dataset.py)
- ✅ GPU shaders installed (13 HLSL files in native/shaders/gpt2/)
- ✅ C++ build system ready (native/scx_runtime/)
- ✅ Config system ready (KXML + tool dispatcher)
- ⏳ Coding corpus (need to gather or provide)
- ⏳ C++ inference boilerplate (GGUF loader, HTTP server)

---

## Data Sources Confirmed

✅ **Coder Training Data:**
- `E:\data\smgm16_gpu_bridge\coder_outputs.train.jsonl` (1.8 GB, 158K records)
- `E:\data\smgm16_gpu_bridge\coder_outputs.val.jsonl` (207 MB, 17K records)
- Format: conversation history + code domain labels
- Ready to use immediately

✅ **Numatic Semantic Data:**
- 9 files, 3.4 GB total
- Already has glyph phase annotations (@numatics.kuhul_phases)
- Maps to KXML domain routing
- Can inform token signal generator

## Questions for User

1. **C++ inference boilerplate:** Do you have existing GGUF loader + HTTP server code?
2. **Token signal generator logic:** 
   - Given a coder prompt, which glyph phases should execute? (SKILL_PYTHON, GPU_MATMUL, etc.)
   - Use @numatics.kuhul_phases from numatic data?
3. **Domain mapping:** Should numatic data's semantic_fold map 1:1 to KXML domains (D0-D5)?
4. **Timeline:** Start Phase 1.1 immediately with coder data?

---

## Next Steps

1. User provides coding chunks (corpus, C++ boilerplate, etc.)
2. Begin Phase 1.1: Prepare corpus
3. Phase 1.2: Fine-tune GPT-2 on coding data
4. Phase 2: Build native inference engine
5. Phase 3: Register and integrate

**Ready when you are. What coding chunks do you have?**
