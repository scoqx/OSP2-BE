---
name: qvm-optimization
description: Optimize Quake 3 QVM (cgame/game/ui) code in this repo by applying project-specific rules for function calls, branching, loops, syscalls, cvar/global access, and pointer reuse. Use when editing performance-sensitive QVM code under code/cgame, code/game, or code/q3_ui, or when the user mentions QVM optimization, cvar usage, or client/entity pointer patterns.
---

# QVM Optimization for This Project

This skill encodes local Q3VM optimization rules from:

- `docs/qvm-optimization-rules.md`
- `docs/VarUsing_QVM.md`
- `.cursor/rules/optimization-qvm.mdc`

Use it whenever you are:

- modifying `cgame`, `game`, or `ui` C files that run inside Q3VM;
- touching hot paths (per-frame logic, entity updates, HUD, prediction, tracers);
- changing how cvars, globals, clientinfo, or `cg_entities` are accessed.

Always prefer **clear, linear C code** that matches these rules over clever but QVM-unfriendly patterns.

## 1. QVM performance model (mental model)

Before optimizing, remember the approximate costs in Q3VM:

- **Function call**: ~40–50 cycles (ENTER + LEAVE + CALL + ARG).
- **Memory access** (LOAD/STORE of 4 bytes): ~2–3 cycles — cheap.
- **Syscalls** (`trap_*`): cost depends on engine, but **much more expensive** than plain VM operations.
- The VM is **stack-based**; LCC does **no** inlining, loop unrolling, or constant folding for you.

Implications:

- Avoid tiny helper functions in hot paths; inline manually.
- Favor fewer function calls, fewer branches, and simple linear code.
- Treat syscalls as expensive; do culling/filtering in VM first.

For details, see `docs/qvm-optimization-rules.md`.

## 2. Golden rules for hot paths

Apply these when a function is called many times per frame or sits on a known critical path.

### 2.1 Inline > tiny functions

- Keep **trivial logic (1–3 lines) inline** in hot paths.
- Avoid chains like `GetX() → CheckX() → UseX()` where each is tiny.
- Reserve separate functions for:
  - complex logic;
  - infrequently used code;
  - code outside hot paths (menus, infrequent events).

### 2.2 Flat, linear code > deep branching

- Each `if` adds branches; mispredictions and extra VM ops hurt.
- Prefer **early return** instead of nested `if`/`else` pyramids.
- Combine conditions: `if (a && b)` vs nested `if (a) { if (b) { ... } }`.
- Avoid nesting deeper than 3 levels in hot code.

### 2.3 One pass > multi-phase processing

- One straightforward loop over data is often faster and simpler than:
  - phase 0 / phase 1 passes;
  - complicated caches with many checks and updates.
- Reserve caches for **truly expensive** operations on **rarely changing** data.
- Prefer O(n) single-pass algorithms over O(n²) with repeated shifts/removals.

### 2.4 Minimize syscalls

- Do as much culling and condition checking in VM as possible.
- Only call into the engine when absolutely necessary.
- Cache results of **expensive** syscalls (e.g. `trap_CM_BoxTrace`) when reused.
- Prefer batch operations over many small syscalls if the API allows.

### 2.5 Simplicity wins

- Fewer VM opcodes → usually faster and easier to maintain.
- Avoid “smart” micro-optimizations that increase branching, state, or complexity without clear gain.

See sections “Золотые правила”, “Антипаттерны”, and the hot-path checklist in `docs/qvm-optimization-rules.md` for more nuance.

## 3. Cvars and globals (`docs/VarUsing_QVM.md`)

### 3.1 Cost of accessing cvars and globals

- Reading `cvar.integer` or `cgs.someField` is one 4-byte load (`LOAD4`), ~2–3 cycles.
- Reading a `vmCvar_t` **does not** perform a syscall: the engine synchronizes values when cvars change.
- Access is essentially: base address + field offset + LOAD4 — **cheap**.

### 3.2 Do NOT mirror cvars into `cgs` “for speed”

Anti-pattern:

- Copying once per frame:
  - `cgs.shud = cg_shud.integer;` then using `cgs.shud` everywhere.

Why this is bad:

- You replace N cheap loads from `cg_shud` with:
  - 1 store + N loads from `cgs` — **no real operation savings**.
- Code becomes more complex and less obvious.

**Rule:** When you need a simple value/flag from a cvar, **read the cvar directly**, e.g.:

- `cg_shud.integer`
- `cg_drawAmmoWarning.integer`

### 3.3 When to cache into a local variable

Caching into a local **only helps** if getting the value is **expensive**, e.g.:

- result of `trap_*` call;
- result of a non-trivial function;
- repeated pointer chasing / complicated access pattern.

In that case:

1. Compute the value once.
2. Store in a local.
3. Reuse the local in loops or further logic.

For a **simple int from a global or cvar**:

- Reading directly from the global/cvar is fine, even in conditions.
- Caching to a local adds:
  - one extra store;
  - the same number of loads (now from the stack).
- Operations increase, but you gain practically nothing.

### 3.4 Simple rules table

- **Single int/flag from cvar or `cgs`**:
  - Read directly (`cg_shud.integer`, `cgs.someFlag`).
- **Expensive computation (`trap_*`, heavy function)**:
  - Compute once → store in local → reuse.
- **Multiple checks of the same cvar in one function**:
  - Optional: cache in a local at the top for clarity if it reads better.

See `docs/VarUsing_QVM.md` for detailed rationale.

## 4. Pointer and structure access

Key idea: **A pointer caches the address, not the value.**

In Q3VM, computing the address of `array[index]` (base + index * size) costs multiple instructions. If you access the same slot several times, it can be worth caching the address in a pointer.

### 4.1 When a pointer is justified

Use a pointer when:

- you have **2+ accesses** to the same array slot (e.g. `clientinfo[num]`, `cg_entities[entNum]`);
- code is:
  - in a loop;
  - in frequently executed logic (players, entities, HUD, prediction);
  - or simply becomes significantly clearer.

Recommended pattern:

```c
clientInfo_t *ci = &cgs.clientinfo[num];

ci->team   = ...;
ci->health = ...;
```

Benefits:

- address of `cgs.clientinfo[num]` is computed once;
- fewer repeated address calculations;
- code is shorter and easier to extend (`ci->thirdField`, etc.).

### 4.2 When a pointer is NOT needed

- Only **one** access to an array element:

```c
team = cgs.clientinfo[num].team;
```

- No loop, no repetition, no hot-path benefit.

In these cases, introducing:

```c
clientInfo_t *ci = &cgs.clientinfo[num];
team = ci->team;
```

is pointless and slightly more expensive.

### 4.3 Pointer scope and safety

- Keep pointer scope as **narrow as possible**.
- Only create the pointer when the index is known to be valid:

```c
if ( num >= 0 && num < MAX_CLIENTS ) {
    clientInfo_t *ci = &cgs.clientinfo[num];
    ...
}
```

- Do **not** hoist such pointers to function scope if they are used only in one block.

### 4.4 Don’ts

- Do **not** cache a single field via pointer “just in case”:

```c
int team = ci->team;  /* without a clear reason */
```

- Do **not** use a pointer only to immediately read one field:

```c
clientInfo_t *ci = &cgs.clientinfo[num];
team = ci->team;  /* pointless indirection */
```

- Do **not** use pointers as a substitute for logic:
  - If semantics require fresh reads, read at the point of use.

See the pointer section and summary in `docs/VarUsing_QVM.md` and `.cursor/rules/optimization-qvm.mdc`.

## 5. QVM-specific pointer reuse and globals (`.cursor/rules/optimization-qvm.mdc`)

### 5.1 Cost comparison: direct access vs pointer

For a **single** read:

- Direct: `x = cg.test;`
  - base load → offset → load value → **~2 VM instructions**.
- Via pointer:
  - `int *p = &cg.test; x = *p;`
  - base load → offset → store local → load pointer → load value → **~3–4 VM instructions**.

👉 For one access, a pointer is **worse**. Do not introduce a pointer just for a single read.

For **multiple** reads of the same path:

```c
int *p = &cg.test;
x = *p;
y = *p;
z = *p;
```

- Now the one-time address computation + repeated dereferences can be cheaper than repeating the full global-path access 3+ times.

### 5.2 Deeply nested globals

- Deep paths like `cgs.osp.crosshair.x` are more expensive than flatter structures.
- Prefer:
  - flattening frequently used data into a dedicated global (`crosshairData.distance`),
  - or taking a single pointer:

```c
crosshair_t *ch = &cgs.osp.crosshair;
if ( ch->enable ) {
    color = ch->color;
    size  = ch->size;
}
```

### 5.3 Per-frame caches and `static`

- Per-frame caches keyed by `cg.time` are best stored:
  - as `static` variables within the function or file that uses them;
  - **not** inside large globals like `cg_t` or `cgs_t`.
- This reduces offset size and access cost in QVM.

### 5.4 Cvar access in hot paths

- Cvar reads are just memory loads, but **deep global paths** can still be more expensive.
- In very hot loops, consider:
  - caching the cvar’s **value** once per frame into a `static` (using `cg.time`);
  - then using that local copy.

Follow `.cursor/rules/optimization-qvm.mdc` when making code changes; this skill summarizes the intent behind that rule file.

## 6. When to optimize (and when not to)

- **Optimize**:
  - functions called hundreds or thousands of times per frame;
  - code that profiling shows as a bottleneck;
  - entity update, prediction, movement, rendering, and HUD hot paths.
- **Usually skip micro-optimizations** for:
  - rarely used code;
  - menu/UI logic called infrequently;
  - code already well under budget (< 0.1 ms).

Default workflow:

1. **Profile first** (or rely on known hot paths).
2. Apply the rules in this skill.
3. Re-measure or at least sanity-check that the code is still simple and correct.

## 7. Recommended workflow for the agent

When the user asks to optimize QVM code in this repo or you are editing hot paths in `cgame/game/ui`:

1. **Identify context**
   - Confirm the file is part of Q3VM (`code/cgame`, `code/game`, `code/q3_ui`).
   - Determine whether the function is hot-path or latency-sensitive.
2. **Review against golden rules**
   - Look for tiny helper functions in hot paths; consider inlining.
   - Check for deep nesting and multi-phase passes; flatten if reasonable.
   - Look for per-iteration syscalls; move culling/checks into VM and minimize calls.
3. **Audit cvar/global usage**
   - Replace unnecessary `cgs` mirrors of cvars with direct cvar reads.
   - Only introduce locals when underlying computation is expensive.
4. **Audit pointers and structure access**
   - Introduce pointers only when there are multiple accesses to the same array slot.
   - Keep pointer scopes narrow and indices validated.
5. **Review caches and statics**
   - Prefer `static` per-function caches keyed by `cg.time` for per-frame data.
   - Avoid bloating `cg_t`/`cgs_t` just for caching.
6. **Double-check correctness**
   - Ensure that any refactors preserve semantics.
   - Prefer simpler, more readable code when multiple versions are equivalent.

For deeper background or edge cases, read:

- `docs/qvm-optimization-rules.md`
- `docs/VarUsing_QVM.md`
- `.cursor/rules/optimization-qvm.mdc`

