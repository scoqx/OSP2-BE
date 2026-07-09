---
name: qvm-optimization
description: Q3VM hot-path rules for code/cgame, code/game, code/q3_ui. Fewer calls/branches/syscalls; direct cvar reads; pointers only when reused. Use for QVM optimization or cvar/entity access patterns.
---

# QVM Optimization (OSP2-PBE)

**Where:** `code/cgame`, `code/game`, `code/q3_ui` — per-frame, HUD, entities, prediction.

## Costs

| | |
|---|---|
| Function call | ~40–50 cycles (LCC never inlines) |
| `cg_foo.integer` / global field | ~2–3 cycles |
| `trap_*` | much worse — minimize |

## Do NOT add functions

**Default: inline.** No new helpers unless **the same non-trivial code** is **strongly duplicated** in multiple places.

- No micro-helpers (1–3 lines), no `GetX`→`CheckX` chains.
- No wrapper around one cvar or one field read.
- Worst case: **one** function for the duplicated block only.

Extra functions on hot paths = extra CALL cost every time.

## Hot-path rules

1. Flat code: early return, shallow nesting, `if (a && b)` not nested pyramids.
2. One loop/pass; avoid fancy caches unless syscall/heavy work + stable data.
3. Cull in VM before `trap_*`; cache only expensive reused syscall results.
4. Read cvars directly (`cg_x.integer`) — **do not** copy into `cgs` “for speed”.
5. Local variable only for expensive value (`trap_*`, heavy fn). Re-read simple ints freely.
6. `ci = &cgs.clientinfo[i]` only if **2+** uses of **same** slot. One read → direct access, no pointer.
7. Validate index; keep pointer scope minimal.
8. Per-frame cache: `static` + `cg.time` in that function — not new `cg_t`/`cgs_t` fields.
9. Never `(void)param;` — not used here; delete if seen.

## Match local style (do not invent your own)

When editing existing code, **mirror the surrounding pattern** — do not swap in a different construct.

- If the nearby block uses `if / else if / else`, extend it with another `else if`. Do **not** replace it with a ternary.
- If the nearby block already uses a ternary chain, keep the ternary.
- New branch goes in the same place a logical peer would (e.g. a new case right after the existing `freeze` check, matching its `if/else` shape).
- Keep indentation, brace placement, and naming identical to the block you touch.
- Change only what the request needs; do not "improve" formatting of untouched lines.

## SuperHUD

Match time: `SHUD_WRITE_MATCH_TIME_STR`. Local/spawn time: inline `cg_drawTimer.integer == 0 && !CG_IS_LOCAL_CLIENT_SPECTATOR()` where format differs.

## Optimize?

**Yes:** entity/draw/predict hot paths, syscall spam, same index addressed many times.

**No:** menus, rare events, already-cheap code.

## Checklist before commit

1. Hot path? If not — don’t refactor for QVM.
2. Added functions? Undo unless strong duplication.
3. Nested `if`, tiny helpers, `trap_*` in loops? Fix.
4. Cvar mirrors / pointless locals / pointer for one read? Fix.
5. Behavior unchanged.
