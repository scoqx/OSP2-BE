# QVM Logic: How to Write Fast Code for the Quake III Virtual Machine

This document summarizes how the QVM (Quake III Virtual Machine) works and how C code should be written so that the compiled bytecode runs as fast as possible. Target: cgame (and game/ui) modules that are built to `.qvm` and run inside the engine's VM.

---

## 1. What is the QVM?

- **Stack-based virtual machine**: All operations use a stack. There are no CPU-style registers; LCC (the compiler) emits bytecode where every parameter and result is pushed/popped on the stack.
- **Compilation path**: Each `.c` file is compiled by **LCC** to a **text IR** (intermediate representation) — stack-machine assembly. Then **q3asm** assembles all units into a single **.qvm** (code + data + bss). No optimizations are done at assembly time.
- **Execution**: The engine either **interprets** the bytecode opcode-by-opcode or (in many builds) **JIT-compiles** it to x86 on load. Either way, the cost of each C construct is largely determined by the number and kind of opcodes (CALL, ENTER/LEAVE, LOAD/STORE, branches, etc.).

References: [Q3VM specs](https://www.icculus.org/~phaethon/q3mc/q3vm_specs.html), [Fabien Sanglard – Q3 VM](https://fabiensanglard.net/quake3/qvm.php).

---

## 2. Cost of Key C Constructs in QVM

### 2.1 Function calls — expensive

- Each **call** in C becomes: push arguments (ARG/ADDRFP/LOCAL), **CALL** (jump to function), **ENTER** (new stack frame), then in callee **LEAVE** and return.
- **No inlining**: LCC does not inline. Every small helper (getter, check, math) is a real call with full stack frame setup/teardown.
- **Implication**: Prefer **fewer, larger functions** over many tiny helpers. Inline critical logic into the hot path instead of splitting into 5 small functions.

### 2.2 Branches and conditionals

- Comparisons compile to **EQ/NE/LT/LE/GT/GE** opcodes (with jump target). Every `if` is a jump in bytecode.
- **Many branches** = more opcodes and more unpredictable control flow for the interpreter (and sometimes for JIT).
- **Implication**: Prefer **linear flow** where possible. Avoid long `if/else if` chains and deeply nested conditions when a simple loop or a few checks would do. “Simple” code often generates fewer and cheaper opcodes than “clever” branching.

### 2.3 Static and global state

- **Static variables**: Stored in BSS/DATA. Access is **ADDRGP** + **LOAD/STORE** (global address + read/write). Cheap per access, but **more static state** = more code to maintain it (e.g. “cache valid” flags, phases, last batch size).
- **Implication**: Caches and “phase” logic add branches and stores every frame. In QVM, that extra logic can cost more than the savings from the optimization. Prefer **simple invariants** (e.g. one loop, one pass) over “smart” state machines and multi-step caches.

### 2.4 Loops

- A **for** loop is: init, conditional jump (e.g. LEI/GTI), body, increment, jump back. Loop body cost dominates.
- **Implication**: One **simple loop** (e.g. iterate and do one thing per element) is usually cheaper than multiple passes, or “batch this frame, next frame next batch” with extra state and branches. Fewer iterations with a bit more work per iteration can still win if it avoids extra functions and branches.

### 2.5 System calls (trap_* / engine calls)

- **System calls** (e.g. `trap_R_AddRefEntityToScene`, `trap_CM_BoxTrace`) are implemented **outside** the VM. The VM does a CALL to a negative “syscall” index; the engine executes native code.
- **Implication**: The **expensive part** is usually the engine work (trace, draw call), not the VM call overhead. Reducing the **number** of trap calls (e.g. fewer traces per frame) matters more than micro-optimizing C code around them. Inside the VM, keep hot paths simple so the VM executes fewer opcodes per frame.

### 2.6 Memory access

- **LOAD4/STORE4** (and 1/2) for every read/write. Struct field access = base address + offset + load. No CPU-style register allocation across calls.
- **Implication**: Repeated access to the same “hot” data is fine; the main cost is still **call and branch count** and **number of trap calls**, not raw load/store inside one function.

---

## 3. Practical Guidelines for Fast QVM Code

1. **Prefer simple, linear code**
   - One clear path through the function, minimal branching. Avoid “optimization” that adds phases, modes, and “if (cache valid) … else rebuild cache”.

2. **Minimize function calls in hot paths**
   - Inline logic into the caller when it’s small and hot (e.g. one loop that does “get entity + check + use” in one place). Avoid chains of getters/checkers that each become a full VM call.

3. **Avoid “clever” caches and state machines**
   - Per-frame caches, “batch phase”, “last batch size” checks, etc. add branches and stores. In QVM, that complexity often costs more than a straightforward loop (e.g. over entities or marks). Prefer one pass that does the work over “build cache then lookup”.

4. **Fewer trap calls**
   - The biggest wins come from doing **fewer** `trap_CM_*`, `trap_R_*`, etc. (e.g. fewer traces, fewer draw calls). Structure code so that hot paths call traps less often, without adding heavy VM-side logic (extra functions, many branches, big caches).

5. **One loop instead of many**
   - If you can do “for each X do Y” in one loop, that’s usually better than “first loop build cache, second loop use cache” or “this frame do slice A, next frame slice B” with extra state. Simpler bytecode = faster VM execution.

6. **No LCC optimizations**
   - LCC does not perform inlining, loop optimizations, or advanced codegen. What you write is what you get in terms of call/loop/branch structure. So **source-level simplicity** directly translates to simpler and often faster bytecode.

7. **C89 and project rules**
   - Declare variables at the start of blocks; no C99-style in-loop declarations. This keeps the code compatible with the QVM toolchain and avoids surprises.

---

## 4. What to Avoid (Summary)

- **Many small functions** in hot paths (each call = ENTER + LEAVE + CALL).
- **Heavy use of static state** to “optimize” (cache valid, phase, last size, etc.) — often adds more VM work than it saves.
- **Spreading work over frames** with phases/batches when a single simple loop per frame would do.
- **Indirection** (e.g. “build array of indices then look up”) when a direct loop (e.g. “for each entity, if match do thing”) is simpler and often cheaper in QVM.
- **Assuming** that “fewer iterations” is always better: if the alternative is more branches, more functions, and more state, the simple loop can still be faster.

---

## 5. CG_BE_FEATURE_ENABLED: Feature-Gating Checks

`CG_BE_FEATURE_ENABLED(feature)` is a macro: `(BE_ENABLED && !(cgs.be.disableFeatures & (feature)))`. It gates BE (OSP2-BE) features per server config.

### How it fits QVM_LOGIC

- **No function call**: It's a macro, so it expands inline. No `CALL`/`ENTER`/`LEAVE` overhead — aligns with "minimize function calls" (Section 3).
- **Per-expansion cost**: Each use is ~2 loads (`be_enabled.integer`, `cgs.be.disableFeatures`), one bitwise AND, one logical AND with short-circuit, and a conditional jump. Globals (BSS) are cheap (Section 2.3).
- **Branch count**: Used 50+ times across cgame, often in hot paths (per-entity, per-frame). Each use adds a branch. Example: outline check in `cg_players.c` three times per player (legs, torso, head) — 192 branches per frame for 64 players. Many small branches are what Section 2.2 cautions against.
- **Trade-off**: The macro itself is lightweight. The cost comes from repeating the check in many places instead of consolidating. An early-exit pattern (e.g. "if (!BE_ENABLED) return" at function start) can skip whole blocks, but then every caller must handle it. Per-entity checks are often unavoidable when behavior differs per entity (e.g. outline only for some players).

### Practical takeaway

- The macro is appropriate: no call overhead, simple logic.
- When adding new `CG_BE_FEATURE_ENABLED` checks, prefer:
  - One check at the start of a function when the whole function is feature-gated, instead of repeating inside loops.
  - Avoiding extra nesting when the check can be combined with an existing condition.
- Avoid turning it into a function (would add call overhead) or building "cached" feature flags (adds state and branches per frame; Section 4 warns against that).

---

## 6. References

- [Q3VM Specifications](https://www.icculus.org/~phaethon/q3mc/q3vm_specs.html) — opcodes, memory, code/data separation.
- [Fabien Sanglard – Quake 3 VM](https://fabiensanglard.net/quake3/qvm.php) — toolchain (LCC → text IR → q3asm → .qvm), vmMain, system calls.
- John Carmack .plan (Aug 1999): VM was first interpreter; performance was disappointing; runtime x86 compiler was added in one day.

---

*Document created for OSP2-PBE cgame: prioritize simple code for QVM to reduce VM overhead and lag.*
