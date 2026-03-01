# QVM Logic: Quick Reference for OSP2-BE Developers

This is a **quick reference** for writing fast QVM code in OSP2-BE cgame/game/ui modules. For complete documentation, see:

- **[qvm-architecture.md](../../docs/qvm-architecture.md)** — Полная документация по архитектуре QVM, опкодам, формату файлов
- **[qvm-performance-guide.md](../../docs/qvm-performance-guide.md)** — Практические примеры оптимизации, паттерны, benchmarks

---

## 1. Что такое QVM?

**Q3VM (Quake III Virtual Machine)** — стековая виртуальная машина для безопасного выполнения игровых модулей.

- **Stack-based**: все операции используют стек (нет CPU-регистров)
- **Compilation**: C → LCC → .asm → q3asm → .qvm (без оптимизаций)
- **Execution**: интерпретация или JIT-компиляция в x86

**Ссылки:** [Q3VM specs](https://www.icculus.org/~phaethon/q3mc/q3vm_specs.html), [Fabien Sanglard](https://fabiensanglard.net/quake3/qvm.php), [q3vm-master](https://github.com/jnz/q3vm)

---

## 2. Правила корректности для QVM

Код должен работать в QVM, не только в native компиляторе. Эти правила предотвращают баги, которые проявляются **только** в QVM.

### 2.1 Initialization — always explicit

In QVM you **cannot** rely on “maybe it was zeroed”. Local variables are **never** auto-initialized.

- **Scalars:** `int i = 0;`, `float f = 0.0f;`
- **Vectors:** `vec3_t v = {0, 0, 0};`
- **Structs:** use `memset(&s, 0, sizeof(s));` or `= {0}` where valid
- **Pointers:** set explicitly to `NULL`

**Wrong (even if it “works” elsewhere):** `int i;` or `myStruct_t s;` with no initializer.

### 2.2 C89: all declarations at the start of the block

QVM is built with strict C89. Declarations in the middle of a block are forbidden.

**Wrong:** `if (cond) { int x = 5; }`  
**Right:** declare `int x;` at the start of the block, then `x = 5;` inside the `if`.

See project C89 rules (e.g. `.cursor/rules/c89.mdc`) for full details.

### 2.3 Linear, deterministic functions

Functions should be predictable: input → process → output. Avoid hidden dependence on global state and “magic” side effects. One clear purpose per function.

**Bad:** `void DoStuff(void) { if (level.time > lastTime) foo++; }`  
**Better:** pure-ish helper, e.g. `int CalcFoo(int time, int lastTime, int foo)` that returns the new value; caller updates state explicitly.

### 2.4 Minimize global variables

Globals in QVM make debugging harder, reduce predictability, and limit extensibility. If a global is needed, it should have a clear responsibility, be initialized at module startup, and not be updated from arbitrary places.

### 2.5 No dynamic memory

QVM has no `malloc`/`free` in the usual sense. Use static arrays or memory provided by the engine. No `new`, `malloc`, `realloc`.

### 2.6 Bounds checks — always

The VM does not forgive out-of-bounds access. Always guard array and index use:

```c
if (index < 0 || index >= MAX_CLIENTS) {
    return;
}
```

### 2.7 Do not rely on order of evaluation

The QVM compiler and VM may evaluate expressions differently than native GCC. Avoid undefined or implementation-dependent order, e.g. `a = b++ + b;`.

### 2.8 Structures: keep them POD and simple

No nested pointers without a clear need, no “clever” structs. Data should be copyable byte-by-byte without surprises.

### 2.9 Code must survive VM compilation, not only GCC

If code “works in a DLL” but fails in QVM, the code is wrong, not the VM. Target the QVM toolchain and runtime.

### 2.10 OSP / id style

- **Simpler > faster > safer** in that order of priority when trading off.
- Fewer abstractions, more explicitness.
- More explicit zeros and initializers.

---

## 3. Стоимость операций в QVM

> **Детальная информация:** см. [qvm-architecture.md § Набор инструкций](../../docs/qvm-architecture.md#набор-инструкций-opcodes) и [qvm-performance-guide.md](../../docs/qvm-performance-guide.md)

### 3.1 Function calls — ДОРОГО (40-50 циклов overhead)

- **ENTER** (10-20 циклов) + **LEAVE** (5-10 циклов) + **CALL** (5 циклов) = **40-50 циклов** на каждый вызов
- **LCC НЕ делает inlining** — каждый getter/setter это полноценный CALL
- **Правило**: inline критичный код в hot paths вручную

**Пример:**
```c
// ❌ Плохо: 4 function calls
if (IsValidClient(i) && IsAlive(i) && IsVisible(i)) { ... }

// ✅ Хорошо: inline checks
if (i >= 0 && i < MAX_CLIENTS && 
    cg_entities[i].currentState.health > 0 &&
    !(cg_entities[i].currentState.eFlags & EF_NODRAW)) { ... }
```

> **Подробнее:** [qvm-performance-guide.md § Pattern 1: Function Call Hell](../../docs/qvm-performance-guide.md#pattern-1-function-call-hell)

### 3.2 Branches and conditionals

- Comparisons compile to **EQ/NE/LT/LE/GT/GE** opcodes (with jump target). Every `if` is a jump in bytecode.
- **Many branches** = more opcodes and more unpredictable control flow for the interpreter (and sometimes for JIT).
- **Implication**: Prefer **linear flow** where possible. Avoid long `if/else if` chains and deeply nested conditions when a simple loop or a few checks would do. “Simple” code often generates fewer and cheaper opcodes than “clever” branching.

### 2.3 Static and global state

- **Static variables**: Stored in BSS/DATA. Access is **ADDRGP** + **LOAD/STORE** (global address + read/write). Cheap per access, but **more static state** = more code to maintain it (e.g. “cache valid” flags, phases, last batch size).
- **Implication**: Caches and “phase” logic add branches and stores every frame. In QVM, that extra logic can cost more than the savings from the optimization. Prefer **simple invariants** (e.g. one loop, one pass) over “smart” state machines and multi-step caches.

### 3.4 Loops

- A **for** loop is: init, conditional jump (e.g. LEI/GTI), body, increment, jump back. Loop body cost dominates.
- **Implication**: One **simple loop** (e.g. iterate and do one thing per element) is usually cheaper than multiple passes, or “batch this frame, next frame next batch” with extra state and branches. Fewer iterations with a bit more work per iteration can still win if it avoids extra functions and branches.

### 3.5 System calls (trap_* / engine calls)

- **System calls** (e.g. `trap_R_AddRefEntityToScene`, `trap_CM_BoxTrace`) are implemented **outside** the VM. The VM does a CALL to a negative “syscall” index; the engine executes native code.
- **Implication**: The **expensive part** is usually the engine work (trace, draw call), not the VM call overhead. Reducing the **number** of trap calls (e.g. fewer traces per frame) matters more than micro-optimizing C code around them. Inside the VM, keep hot paths simple so the VM executes fewer opcodes per frame.

### 3.6 Memory access — ДЁШЕВО (2-3 цикла)

- **LOAD4/STORE4**: ~2-3 цикла на операцию
- Struct field access: base + offset + load (тоже дёшево)
- **Правило**: не бойтесь обращаться к памяти, это не bottleneck

**Вывод:** основная стоимость — **function calls** и **branches**, не memory access!

### 3.7 Loop vs single check

- A loop (init, conditional jump, increment, jump back) costs multiple opcodes per iteration. When you only need to know "is string at least N chars" or "is index in range", a **single character/index check** is cheaper than a loop.
- **Example**: `cmd[3] != '\0'` instead of `i = 0; while (cmd[i] != '\0' && i < 4) i++; if (i > 3)`. One load + one comparison vs loop overhead.

- **Implication**: Repeated access to the same “hot” data is fine; the main cost is still **call and branch count** and **number of trap calls**, not raw load/store inside one function.

---

## 4. Practical Guidelines for Fast QVM Code

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

## 5. What to Avoid (Summary)

- **Many small functions** in hot paths (each call = ENTER + LEAVE + CALL).
- **Heavy use of static state** to “optimize” (cache valid, phase, last size, etc.) — often adds more VM work than it saves.
- **Spreading work over frames** with phases/batches when a single simple loop per frame would do.
- **Indirection** (e.g. “build array of indices then look up”) when a direct loop (e.g. “for each entity, if match do thing”) is simpler and often cheaper in QVM.
- **Assuming** that “fewer iterations” is always better: if the alternative is more branches, more functions, and more state, the simple loop can still be faster.

---

## 6. CG_BE_FEATURE_ENABLED: Feature-Gating Checks

`CG_BE_FEATURE_ENABLED(feature)` is a macro: `(cgs.beFeaturesEnabled & (feature))`. It gates BE (OSP2-BE) features per server config and user settings.

### How it works

The system uses cached bitmasks for maximum efficiency:

- `cgs.beEnabled` — cached `be_enabled.integer` (is BE globally on?)
- `cgs.beFeaturesEnabled` — final bitmask: user-enabled features ANDed with server permissions
- `cgs.beDisableFeatures` — server-disabled features (received from configstring)

`CG_UpdateBeFeatures()` computes `featuresEnabled = beFlags & ~disableFeatures` whenever:
- User changes any BE-related cvar (via `CG_LocalEventBeFeaturesChanged`)
- Server changes `disableFeatures` (via configstring update)

### How it fits QVM_LOGIC

- **No function call**: It's a macro, so it expands inline. No `CALL`/`ENTER`/`LEAVE` overhead.
- **Per-expansion cost**: Single load (`cgs.beFeaturesEnabled`) + one bitwise AND. No logical AND, no short-circuit, no extra branches.
- **Cache locality**: Both `beFeaturesEnabled` and `beDisableFeatures` live in `cgs_t`, often in same cache line.
- **Compared to old approach**: Previously was `(be_enabled.integer && !(cgs.beDisableFeatures & feature))` = 2 loads + AND + logical AND + branch. Now = 1 load + AND. ~2x fewer operations per check.

### Practical takeaway

- The macro is very cheap: one AND against a pre-computed bitmask.
- When adding new BE features:
  1. Add the feature flag (e.g. `CG_BE_MY_FEATURE (1 << N)`)
  2. In `CG_UpdateBeFeatures()`, set the bit when user cvar is enabled
  3. Use `CG_BE_FEATURE_ENABLED(CG_BE_MY_FEATURE)` anywhere — it's just a bitwise AND
- Use `cgs.beEnabled` directly when you need to check if BE is globally on (e.g. for help text).

---

## 6.1 Team task marks (clearing / packing)

Team task marks live in a single array `cg.teamMarkers.teamTaskMarks` with count `numTeamTaskMarks`. There is no dynamic allocation, so "removing" marks is done by **packing**: one pass over the array, copying "keep" entries to the front and updating the count.

- **One pack pass** (O(n)) is better than multiple "remove one, shift tail" loops (O(n²) in worst case). Use a single loop: `for (i = 0; i < numTeamTaskMarks; i++) { if (keep(mark)) { if (w != i) copy; w++; } } numTeamTaskMarks = w;`
- **Avoid extra functions for a single use.** If the only caller of a "clear marks" function is one place (e.g. respawn), inline the pack loop there. That removes one CALL/ENTER/LEAVE in QVM and keeps the action obvious at the call site.
- **Shared clear logic** (e.g. clear round-specific marks, clear HERE_DEAD for a client) stays in dedicated functions in `cg_teamtasks_utils.c`; they are called from several contexts. The rule is: one caller → consider inlining; several callers → one function with one pack loop.
- **CG_TeamTaskMark** (when adding a new mark): one pack pass removes marks to be replaced (same client+type, same client+overhead, same client+PLAYERDEAD/HERE_DEAD/WARN_DEAD as applicable) before appending. Inline `isOverhead` check instead of `CG_IsTeamTaskOverhead` call.
- **CG_InitTeamTaskMarkers**: skip `memset` on `teamTaskMarks` — when `numTeamTaskMarks = 0` no code iterates over the array; saves ~1.5KB of writes per warmup.

---

## 7. References

### Документация OSP2-BE

- **[qvm-architecture.md](../../docs/qvm-architecture.md)** — полная архитектура QVM: опкоды, память, toolchain, формат .qvm
- **[qvm-performance-guide.md](../../docs/qvm-performance-guide.md)** — практические примеры оптимизации, паттерны, benchmarks

### Внешние ресурсы

- [Q3VM Specifications](https://www.icculus.org/~phaethon/q3mc/q3vm_specs.html) — спецификация опкодов, памяти
- [Fabien Sanglard – Q3 VM](https://fabiensanglard.net/quake3/qvm.php) — отличное введение с диаграммами
- [q3vm-master](https://github.com/jnz/q3vm) — современная имплементация Q3VM
- [LCC Compiler](https://github.com/drh/lcc) — retargetable C compiler
- John Carmack .plan (1998): оригинальное описание Q3VM

---

*Quick reference для OSP2-PBE cgame. Детали в [qvm-architecture.md](../../docs/qvm-architecture.md) и [qvm-performance-guide.md](../../docs/qvm-performance-guide.md).*
