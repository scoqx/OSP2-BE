# Чеклист оптимизации: cg_weapons.c и cg_players.c

Документ создан для устранения падения FPS при большом количестве игроков. Основан на [qvm-architecture.md](qvm-architecture.md), [qvm-optimization-rules.md](qvm-optimization-rules.md), [qvm-performance-guide.md](qvm-performance-guide.md).

---

## Ссылки на QVM Architecture

- **Function call** ≈ 40–50 циклов (ENTER + LEAVE + CALL + ARG)
- **Syscall** — стоимость зависит от движка; `trap_CM_BoxTrace` — самый дорогой
- **LCC не делает inlining** — тривиальный код в hot path надо inline вручную
- **Один проход > кеш** — кеш оправдан только для дорогих операций
- **Early return** — отсекать работу до входа в тяжёлую логику

---

## Горячие точки: дорогие циклы (сводка)

Циклы, в которых в каждой итерации вызываются **функции** или **syscalls** — главные кандидаты на оптимизацию по [qvm-optimization-rules.md](qvm-optimization-rules.md).

| Файл | Строки | Цикл | Дорого в теле цикла | Примечание |
|------|--------|------|---------------------|------------|
| **cg_ents.c** | 1249–1256 | `num < cg.snap->numEntities` | **CG_AddCEntity(cent)** → для ET_PLAYER вызывается **CG_Player(cent)** | Главный hot spot (CG_Player ~230400 вызовов за сессию). Цикл неизбежен; оптимизировать нужно внутри CG_Player. |
| **cg_ents.c** | 1198–1205 | `index < MAX_CLIENTS` | **CG_ConfigString(CS_PLAYERS + index)** (syscall), **CG_NewClientInfo(index)** | Только при смене spectator target. До 64 syscalls + 64 тяжёлых вызовов за один проход. |
| **cg_players.c** | 2221–2227 | `index < MAX_CLIENTS` | **CG_ConfigString(CS_PLAYERS + index)** (syscall), **CG_NewClientInfo(index)** | CG_UpdateOtherClientsInfo. До 64 syscalls за вызов. |
| **cg_players.c** | 2234–2238 | `index < MAX_CLIENTS` | **CG_ConfigString(CS_PLAYERS + index)** (syscall), **CG_NewClientInfo(index)** | CG_UpdateAllClientsInfo. То же. |
| **cg_players.c** | 879–904 | `i < MAX_CLIENTS` | **CG_TeamTaskMark**, **Distance** | CG_ClientInfoUpdateTeamTask (TARGET). CG_IS_ENEMY_SIMPLE — макрос, ок. |
| **cg_players.c** | 930–952 | `i < MAX_CLIENTS` | **CG_TeamTaskMark**, **CG_IS_ENEMY_SIMPLE** (макрос) | TEAMTASK_FROZEN_* branch. |
| **cg_players.c** | 1003–1065 | `i < MAX_GENTITIES` | **Q_stricmp**, **CG_TeamTaskMark**, **Distance** | TEAMTASK_* items. **Не оптимизировать** кэшем индексов — ломает позицию метки на части карт. Оставить Q_stricmp. |
| **cg_players.c** | 1077–1093 | `i < MAX_CLIENTS` | **CG_TeamTaskMark**, **Distance** | TEAMTASK_*_CARRIER. |

**Рекомендации:**
- **CG_ConfigString в цикле:** по возможности проверять один раз «есть ли игрок» без syscall (например, флаг/кэш из snapshot или cgs.clientinfo[index].infoValid), и только при необходимости вызывать CG_ConfigString и CG_NewClientInfo.
- **Цикл по MAX_GENTITIES (items):** early exit при `foundItem` уже есть; рассмотреть ограничение числа проверяемых entity (например, только первые N item-сущностей) или сравнение по modelindex/шаблону без Q_stricmp где возможно.
- **CG_Player:** основной расход — внутри функции (shadow trace, RefEntity, powerups, weapon). См. разделы 5–8 ниже.

---

## cg_weapons.c

### 1. CG_RegisterWeapon в hot path

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_AddPlayerWeapon` | 1627 | Вызывается при каждом рендере оружия (все игроки). Внутри есть `if (weaponInfo->registered) return`, но сам вызов — overhead. |
| `CG_AddViewWeapon` | 2025 | Каждый кадр для локального игрока. |
| `CG_DrawWeaponSelect` | 2111 | В цикле по оружию: `CG_RegisterWeapon(w)` на каждую иконку. |

**Сделано:**
- [x] Inline check `!cg_weapons[weaponNum].registered` перед `CG_RegisterWeapon` в `CG_AddPlayerWeapon`, `CG_AddViewWeapon`, `CG_DrawWeaponSelect`, `CG_OSPDrawWeaponBar*` — избегаем function call overhead (40-50 циклов) когда оружие уже зарегистрировано.

---

### 2. CG_RailTrail — цикл с CG_AllocLocalEntity

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_RailTrail` | 333–387 | Цикл `for (i = 0; i < len; i += cg_railRingsSpacing.integer)` — на каждую итерацию: `CG_AllocLocalEntity()` (функция) + настройка le. При длинном рейле — много аллокаций. |

**Сделано:**
- [x] Вынести общую настройку перед циклом: shader (cg_nomip check), radius, color (railRings * 255, * 0.75), shaderTime, staticRings, rotationStep — больше не вычисляются на каждой итерации.

---

### 3. CG_AddRailEffect — syscall в цикле

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_AddRailEffect` | 399–562 | `for (i = 0; i < len; i += spacing)` — в каждой итерации `trap_R_AddRefEntityToScene(&particle)` (syscall). |

**Чеклист:**
- [ ] Уменьшить плотность спавна (увеличить `spacing`) для длинных рейлов.
- [ ] Добавить early exit при `i > MAX_SPIRAL_PARTICLES` (например, 64).
- [ ] Если движок поддерживает batch — рассмотреть batch syscall.

---

### 4. CG_LightningBolt — CG_Trace ✅ частично

| Место | Проблема |
|-------|----------|
| `CG_LightningBolt` | CG_Trace для каждого игрока с LG. |

**Сделано:** Early exit при `cg_lightningHide` — skip trace и beam для локального игрока. Объединены два блока angle blend — меньше дублирования. Кеш для trace не добавлен (qvm-optimization-rules: антипаттерн).

---

### 5. CG_RocketTrail — CG_SmokePuff в цикле

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_RocketTrail` | 489–503 | Цикл `for (; t <= ent->trailTime; t += step)` — вызов `CG_SmokePuff` в каждой итерации. |

**Чеклист:**
- [ ] Early exit: если `cg_noProjectileTrail.integer`, выйти в начале.
- [ ] Ограничить `step` снизу (не делать слишком частый smoke).

---

### 6. CG_AddPlayerWeapon — множественные trap_R_AddRefEntityToScene

| Место | Строка | Проблема |
|-------|--------|----------|
| Gun/barrel/powerups | 1798–1910 | До 3–4 вызовов `trap_R_AddRefEntityToScene` на оружие (gun, barrel, invis, quad, battlesuit). |
| Flash | 1959 | Ещё один syscall. |

**Сделано:**
- [x] Flash culling: уже реализовано корректно (`cg_muzzleFlash.integer || clientNum != cg.clientNum`).
- [x] Объединён gun+barrel powerups блок: `pw`, `pwInvis`, `pwBattle`, `pwQuad`, `battleShader` вычисляются один раз, используются для обоих. Убрано дублирование ternary для battlesuit shader.

---

### 7. CG_DrawWeaponSelect — цикл с CG_RegisterWeapon

| Место | Строка | Проблема |
|-------|--------|----------|
| Цикл по оружию | 2099–2119 | `CG_RegisterWeapon(w)` в каждой итерации. |

**Чеклист:**
- [ ] Вынести регистрацию в отдельный проход при смене `cg.snap->ps.stats[STAT_WEAPONS]` или вызывать один раз до цикла для всех имеющихся оружий.

---

### 8. CG_GetWeaponTempAccuracy — кеш с ветвлениями

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_GetWeaponTempAccuracy` | 1843–1882 | Static cache `lastHits`, `lastShots`, `lastAcc` — проверка `if (hits == lastHits && shots == lastShots)` + ветвления. |

**Чеклист:**
- [ ] Оценить частоту вызова. Если вызывается редко (HUD) — оставить. Если в hot path — упростить или убрать кеш.

---

### 9. memset vs Com_Memset

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_AddRailEffect` | 414 | `memset(&particle, 0, sizeof(particle))` — в QVM может быть syscall; лучше `Com_Memset` для единообразия. |

**Чеклист:**
- [ ] Заменить `memset` на `Com_Memset` в VM-коде (cg_weapons.c, cg_players.c) для согласованности с qvm-architecture.

---

## cg_players.c

### 1. CG_IsPlayerAvailable — кеш + цикл MAX_GENTITIES ✅ DONE

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_IsPlayerAvailable` | — | При cache miss — цикл по `MAX_GENTITIES`. Per-frame scan — лишняя работа. |

**Сделано:** Per-frame scan убран. `CG_IsPlayerAvailable` удалена. `CG_GetAimContactPoint` встроена в do_aim_point. Используется `cg_entities[clientNum]` напрямую (без скана MAX_GENTITIES).

---

### 2. CG_GetAimContactPoint — встроена ✅ DONE

| Место | Проблема |
|-------|----------|
| Была отдельная функция | MAX_GENTITIES scan, вложенность. |

**Сделано:** Логика встроена в `CG_ClientInfoUpdateTeamTask` do_aim_point. Используется `cg_entities[clientNum]` напрямую (мы знаем clientNum). Цикл MAX_GENTITIES убран.

---

### 3. CG_ClientInfoUpdateTeamTask — вложенные циклы

| Место | Строка | Проблема |
|-------|--------|----------|
| TEAMTASK_TARGET | 879–904 | Цикл `for (i = 0; i < MAX_CLIENTS)` с `CG_IS_ENEMY_SIMPLE`, `Distance`, `CG_TeamTaskMark`. |
| TEAMTASK_FROZEN_* | 930–952 | Цикл по `MAX_CLIENTS` с `CG_IS_ENEMY_SIMPLE`, `CG_TeamTaskMark`. |
| TEAMTASK_* items | 1003–1110 | Цикл по item entities или `MAX_GENTITIES` с ~~Q_stricmp~~, `Distance`, `CG_TeamTaskMark`. |

**Чеклист:**
- [ ] Inline `CG_IS_ENEMY_SIMPLE` и `CG_IS_FROZEN_ENTITY` в циклах (избежать вызовов функций).
- [ ] **TEAMTASK_* items:** не менять — кэш индексов (itemIndex == s_ttItemIndex) ломает позицию метки на части карт; оставить оригинал с Q_stricmp.
- [ ] Early exit: выйти из цикла при `foundEnemy/foundFrozen/foundItem` (уже есть `break`/`!foundItem`).
- [ ] Для TEAMTASK_TARGET: ограничить `searchRadius` или число проверяемых игроков.

---

### 4. CG_PlayerNameCheckDuplicate / CG_PlayerNameFindAndUpdateExisting

| Место | Строка | Проблема |
|-------|--------|----------|
| Оба | 1810, 1978 | Цикл `for (p = 0; p < MAX_CLIENTS)` с `CG_RemoveChatEscapeCharAll`, `strcmp`. |

**Чеклист:**
- [ ] Вызываются при `CG_NewClientInfo` (смена конфига игрока), не каждый кадр — приоритет низкий.
- [ ] Inline `CG_RemoveChatEscapeCharAll` если это короткая функция и вызывается в цикле.

---

### 5. CG_Player — hot path (вызывается N раз за кадр)

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_Player` | 3419–4118 | Уже есть QVM_LOGIC-оптимизации. Остаются: `CG_PlayerAngles`, `CG_PlayerAnimation`, `CG_PlayerPowerups`, `CG_PlayerSplash`, shadow trace, `CG_AddRefEntityWithPowerups`, `CG_AddPlayerWeapon`, `CG_AddHitBox`. |

**Чеклист:**
- [ ] `CG_PlayerPowerups`: вызов уже пропускается при `!cent->currentState.powerups && !(spec glow)` — OK.
- [ ] `CG_PlayerSplash`: пропускается при `!cg_shadows.integer` — OK.
- [ ] Shadow: `trap_CM_BoxTrace` один раз на игрока — дорого. Рассмотреть LOD: не делать trace для далёких игроков (расстояние > 1024).
- [ ] `CG_AddHitBox`: 6x `trap_R_AddPolyToScene`. Early exit при `!cg_drawHitBox.integer` — уже есть. Ограничить отрисовку (например, только враги в радиусе).

---

### 6. CG_Player — Hud markers (CG_Trace x2)

| Место | Строка | Проблема |
|-------|--------|----------|
| `cg_drawHudMarkers` | 3772–3776 | При `!(wh & 2)` — два вызова `CG_Trace` на каждого тиммейта в радиусе. |

**Чеклист:**
- [ ] Кешировать результат trace по (clientNum, vieworg, lerpOrigin, frameCount) — видимость меняется не каждый кадр.
- [ ] Ограничить число проверяемых тиммейтов (например, только ближайшие 8).

---

### 7. CG_Player — outline логика

| Место | Строка | Проблема |
|-------|--------|----------|
| Outline block | 3544–3631 | Много ветвлений: `outlineIsSpec`, `outlineIsEnemy`, `drawOutline`, и т.д. |

**Чеклист:**
- [ ] Уже есть предвычисление `useOutline`, `outlineShader`, `outlineColorBytes` — OK.
- [ ] Объединить проверки: `if (!useOutline) skip outline block` в начале.

---

### 8. CG_AddHitBox — 6x trap_R_AddPolyToScene

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_AddHitBox` | 3374–3409 | Шесть вызовов `trap_R_AddPolyToScene` на игрока. |

**Чеклист:**
- [ ] Culling: не рисовать hitbox для игроков за спиной или вне frustum (если движок не делает сам).
- [ ] Опция `cg_drawHitBox` = 0 — отключить полностью. Рекомендовать для low-end.

---

### 9. CG_LoadClientInfo — цикл MAX_GENTITIES

| Место | Строка | Проблема |
|-------|--------|----------|
| `CG_LoadClientInfo` | 579–587 | Цикл `for (i = 0; i < MAX_GENTITIES)` с `CG_ResetPlayerEntity` при смене модели. |

**Чеклист:**
- [ ] Вызывается при смене модели игрока (редко). Можно сузить до `MAX_CLIENTS` для player entities.

---

### 10. CG_UpdateOtherClientsInfo / CG_UpdateAllClientsInfo

| Место | Строка | Проблема |
|-------|--------|----------|
| Обе функции | 2217–2239 | Цикл по `MAX_CLIENTS`: в каждой итерации **CG_ConfigString(CS_PLAYERS + index)** (syscall) и **CG_NewClientInfo(index)** (тяжёлая функция). До 64 syscalls за вызов. |

**Чеклист:**
- [ ] Вызываются при смене конфига/команды, не каждый кадр — приоритет средний.
- [ ] Уменьшить число syscalls: проверять `cgs.clientinfo[index].infoValid` (или аналог) до вызова CG_ConfigString; вызывать CG_ConfigString и CG_NewClientInfo только для индексов, где конфиг мог измениться.
- [ ] `CG_NewClientInfo` — проверить early exit при неизменном configstring (уже есть для teamTask).

---

## Общий чеклист (QVM anti-patterns)

| Правило | cg_weapons | cg_players |
|---------|------------|------------|
| Нет тривиальных getters в hot path | [ ] | [ ] |
| Критичная логика inline | [ ] | [ ] |
| Нет вызовов функций внутри горячих циклов | [ ] CG_RailTrail, CG_AddRailEffect | [ ] CG_ClientInfoUpdateTeamTask |
| Early return, объединённые условия | [ ] | [ ] |
| Минимум syscalls в циклах | [ ] CG_AddRailEffect | [ ] CG_Player (shadow, hud markers) |
| Кеш только для дорогих операций | [x] CG_LightningBolt (early exit, без кеша) | [x] CG_IsPlayerAvailable, [x] CG_GetAimContactPoint (встроена в do_aim_point) |
| CG_Trace / trap_CM_BoxTrace — кешировать | [ ] CG_LightningBolt (early exit done) | [ ] hud markers |

---

## Приоритеты

1. **Высокий (сразу):** CG_RegisterWeapon в CG_AddPlayerWeapon/CG_DrawWeaponSelect, shadow LOD в CG_Player.
2. **Средний:** CG_AddRailEffect density limit.
3. **Низкий:** CG_ClientInfoUpdateTeamTask inline, CG_PlayerName* functions, memset→Com_Memset.

---

*Версия: 1.0 | Дата: 2026-02-02*
