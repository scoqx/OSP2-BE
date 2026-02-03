# QVM Performance Guide: Практическое руководство по оптимизации

Руководство по написанию быстрого кода для Q3VM. Реальные примеры, антипаттерны, benchmarks и best practices для OSP2-BE cgame/game/ui модулей.

---

## Содержание

1. [Быстрый старт: золотые правила](#быстрый-старт-золотые-правила)
2. [Анатомия медленного кода](#анатомия-медленного-кода)
3. [Pattern 1: Function Call Hell](#pattern-1-function-call-hell)
4. [Pattern 2: Cache Complexity](#pattern-2-cache-complexity)
5. [Pattern 3: Phase Processing](#pattern-3-phase-processing)
6. [Pattern 4: Deep Branching](#pattern-4-deep-branching)
7. [Pattern 5: Syscall Spam](#pattern-5-syscall-spam)
8. [Оптимизация реальных примеров](#оптимизация-реальных-примеров)
9. [Измерение производительности](#измерение-производительности)
10. [Чеклист code review](#чеклист-code-review)

---

## Быстрый старт: золотые правила

### 🥇 Топ-5 правил производительности QVM

1. **Inline > Functions**
   - Function call = 40-50 циклов overhead
   - LCC не делает inlining
   - Inline критичный код вручную

2. **Линейный код > Branches**
   - Каждый `if` = 5-10 циклов + branch misprediction
   - Простой цикл быстрее, чем "умные" проверки
   - Avoid deep nesting (> 3 уровней)

3. **Один проход > Несколько**
   - Кеши добавляют branches и state
   - Один простой цикл часто быстрее двух "оптимизированных"
   - Memory access дешёвый (2-3 цикла), function calls — дорогие

4. **Меньше syscalls > Больше VM-кода**
   - Syscall = переход в native код (стоимость зависит от функции)
   - Batch операции: добавить 10 entities за раз лучше, чем по одному
   - Кешируйте результаты `trap_CM_BoxTrace` (самый дорогой syscall)

5. **Simple > Clever**
   - Простой код = меньше опкодов = быстрее
   - "Clever" оптимизации часто медленнее в QVM
   - Читаемость = maintainability = performance

### ⚠️ Топ-5 анти-паттернов

1. ❌ **Getter/Setter chains**: `GetX() → ValidateX() → UseX()` = 3 function calls
2. ❌ **Cache everything**: добавляет branches, state management, memory
3. ❌ **Phase processing**: разбиение работы на фреймы с фазами
4. ❌ **Deep nesting**: `if { if { if { if { ... } } } }` = jump hell
5. ❌ **Syscall в loop**: `for (i) { trap_Something() }` убивает FPS

---

## Анатомия медленного кода

### Пример: медленный код (реальный антипаттерн)

```c
// ❌ ПЛОХО: 240+ function calls на один entity
void CG_DrawEntity(int entityNum) {
    centity_t* cent;
    
    if (!IsValidEntity(entityNum)) return;           // Call 1
    cent = GetEntity(entityNum);                      // Call 2
    if (!IsVisible(cent)) return;                     // Call 3
    if (IsSpectator(cent)) return;                    // Call 4
    
    vec3_t origin;
    GetEntityOrigin(cent, origin);                    // Call 5
    
    if (IsBehindWall(origin)) return;                 // Call 6
    
    refEntity_t ent;
    InitRefEntity(&ent);                              // Call 7
    SetupEntityModel(&ent, cent);                     // Call 8
    SetupEntityShader(&ent, cent);                    // Call 9
    
    if (ShouldDrawOutline(cent)) {                    // Call 10
        DrawOutline(&ent);                            // Call 11
    }
    
    trap_R_AddRefEntityToScene(&ent);                 // Syscall
}

// Вызывается 64 раза за фрейм (для всех игроков)
// = 64 * 12 = 768 function calls + 64 syscalls
// = ~40 000 циклов только на overhead + syscalls
```

### Что не так?

1. **11 function calls** для одного entity (overhead ~500 циклов)
2. Каждая функция тривиальная (1-3 строки), но вызывается как полноценная функция
3. **Нет early exit** — проверки разбросаны
4. **Множественные syscalls** при повторном вызове

### Быстрая версия

```c
// ✅ ХОРОШО: 0 function calls, inline всё
void CG_DrawEntity(int entityNum) {
    centity_t* cent;
    refEntity_t ent;
    
    // Early checks inline
    if (entityNum < 0 || entityNum >= MAX_GENTITIES) return;
    cent = &cg_entities[entityNum];
    if (!cent->currentValid) return;
    if (cent->currentState.eType == ET_INVISIBLE) return;
    if (cg.snap->ps.clientNum == entityNum && cg.renderingThirdPerson == 0) return;
    if (cent->currentState.eFlags & EF_NODRAW) return;
    
    // Setup inline
    Com_Memset(&ent, 0, sizeof(ent));
    ent.reType = RT_MODEL;
    VectorCopy(cent->lerpOrigin, ent.origin);
    ent.hModel = cent->currentState.modelindex;
    ent.customShader = cent->currentState.customShader;
    
    // Outline check inline (if enabled)
    if (CG_BE_FEATURE_ENABLED(BE_FEATURE_OUTLINE)) {
        if (cent->currentState.eType == ET_PLAYER && 
            cent->currentState.number != cg.clientNum) {
            ent.renderfx |= RF_OUTLINE;
        }
    }
    
    trap_R_AddRefEntityToScene(&ent);
}

// 64 вызова * ~20 циклов inline проверки = ~1 300 циклов
// + 64 syscalls
// = 30x быстрее за счёт устранения function call overhead!
```

**Результат:**
- **Before**: ~40 000 циклов (11 calls * 64 entities)
- **After**: ~1 300 циклов (inline checks * 64 entities)
- **Speedup**: ~30x

---

## Pattern 1: Function Call Hell

### Проблема

```c
// ❌ Плохой паттерн: цепочка тривиальных функций
qboolean IsAlive(int clientNum) {
    return GetHealth(clientNum) > 0;  // Call 1
}

int GetHealth(int clientNum) {
    centity_t* cent = GetCEntity(clientNum);  // Call 2
    return cent->currentState.health;
}

centity_t* GetCEntity(int clientNum) {
    if (!IsValidClient(clientNum)) return NULL;  // Call 3
    return &cg_entities[clientNum];
}

qboolean IsValidClient(int clientNum) {
    return clientNum >= 0 && clientNum < MAX_CLIENTS;
}

// Использование:
for (int i = 0; i < MAX_CLIENTS; i++) {
    if (IsAlive(i)) {  // 4 nested calls!
        DrawPlayer(i);
    }
}

// = 64 clients * 4 calls = 256 function calls
// = ~12 000 циклов только на overhead!
```

**Почему медленно:**
- 4 уровня вложенности
- Каждый call = ENTER (10 циклов) + LEAVE (5 циклов) + CALL (5 циклов) = 20 циклов
- 256 calls * 20 = **5 120 циклов** чистого overhead'а
- Реальная работа: 2 сравнения + 1 load = **6 циклов**

**Соотношение:** overhead / работа = 5120 / 6 = **850x больше overhead!**

### Решение: Inline

```c
// ✅ Inline всё в одну функцию
for (int i = 0; i < MAX_CLIENTS; i++) {
    // Validate (2 comparisons = 4 cycles)
    if (i < 0 || i >= MAX_CLIENTS) continue;
    
    // Get entity (1 load = 2 cycles)
    centity_t* cent = &cg_entities[i];
    
    // Check health (1 load + 1 compare = 4 cycles)
    if (cent->currentState.health > 0) {
        DrawPlayer(i);
    }
}

// Total: 64 * (4 + 2 + 4) = 640 циклов
// Speedup: 5120 / 640 = 8x faster
```

### Когда функции OK?

```c
// ✅ Функция OK: сложная логика, используется в разных местах
void CG_CalculateComplexPhysics(centity_t* cent, vec3_t result) {
    // 50+ строк сложных вычислений
    // Используется в 5 разных местах
    // Inlining увеличит .qvm на 250 строк * 5 = код bloat
}

// ✅ Функция OK: не в hot path
void CG_Menu_OnButtonClick(void) {
    // Вызывается 1-2 раза в секунду (при клике)
    // Overhead не критичен
}

// ❌ Функция НЕ OK: тривиальная, в hot path
qboolean IsPlayer(int num) {
    return num >= 0 && num < MAX_CLIENTS;
}
// Используется 1000+ раз за фрейм → inline!
```

---

## Pattern 2: Cache Complexity

### Проблема: "умный" кеш

```c
// ❌ Плохо: кеш добавляет сложность
static int cachedPlayerList[MAX_CLIENTS];
static int cachedPlayerCount = 0;
static qboolean cacheValid = qfalse;
static int lastFrameCount = 0;

void CG_GetAlivePlayers(int* outList, int* outCount) {
    // Check cache validity (2 loads + 2 compares + 1 branch)
    if (cacheValid && lastFrameCount == cg.frameCount) {
        // Copy from cache (memcpy syscall or loop)
        Com_Memcpy(outList, cachedPlayerList, cachedPlayerCount * sizeof(int));
        *outCount = cachedPlayerCount;
        return;
    }
    
    // Rebuild cache (1 store + 1 store + loop)
    cachedPlayerCount = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (IsPlayerAlive(i)) {  // Function call!
            cachedPlayerList[cachedPlayerCount++] = i;
        }
    }
    cacheValid = qtrue;
    lastFrameCount = cg.frameCount;
    
    // Copy to output
    Com_Memcpy(outList, cachedPlayerList, cachedPlayerCount * sizeof(int));
    *outCount = cachedPlayerCount;
}

// Используется 3 раза за фрейм:
// 1. CG_DrawPlayers()
// 2. CG_DrawRadar()
// 3. CG_DrawTeamInfo()
//
// First call: rebuild (miss) = loop + stores = 1000 cycles
// Next 2 calls: copy from cache = 2 * memcpy = 200 cycles
// Cache management: checks + stores = 50 cycles
// Total: 1250 cycles
```

**Проблемы кеша:**
1. **Cache management overhead**: проверки валидности, фреймкаунты
2. **Extra memory**: `cachedPlayerList[64]` + `cachedPlayerCount` + флаги
3. **Branches**: `if (cacheValid && ...)` каждый вызов
4. **Memcpy cost**: копирование массива (даже если cached)
5. **Function call overhead**: `IsPlayerAlive()` в rebuild

### Решение: простой цикл

```c
// ✅ Хорошо: простой inline цикл, без кеша
void CG_DrawPlayers(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        centity_t* cent = &cg_entities[i];
        
        // Inline check (no function call)
        if (!cent->currentValid) continue;
        if (cent->currentState.eType != ET_PLAYER) continue;
        if (cent->currentState.eFlags & EF_DEAD) continue;
        
        // Draw inline
        DrawPlayerIcon(cent);
    }
}

void CG_DrawRadar(void) {
    // Тот же цикл inline
    for (int i = 0; i < MAX_CLIENTS; i++) {
        centity_t* cent = &cg_entities[i];
        if (!cent->currentValid) continue;
        if (cent->currentState.eType != ET_PLAYER) continue;
        if (cent->currentState.eFlags & EF_DEAD) continue;
        
        DrawRadarBlip(cent);
    }
}

// Каждый цикл: 64 * 6 операций = 384 цикла
// 3 функции * 384 = 1152 цикла
// Speedup: 1250 / 1152 = 1.08x (не сильно, но проще!)
```

**Почему проще лучше:**
- Нет state management (cacheValid, lastFrameCount)
- Нет extra memory (cachedPlayerList[64])
- Нет memcpy
- Легче читать и debug'ить
- Меньше мест для bug'ов

### Когда кеш OK?

```c
// ✅ Кеш OK: дорогое вычисление, редко меняется
static int teamScoreCache[2] = {0, 0};
static int lastScoreTime = 0;

void CG_GetTeamScores(int* red, int* blue) {
    // Обновляем только раз в секунду (scores меняются редко)
    if (cg.time - lastScoreTime > 1000) {
        teamScoreCache[0] = CG_CalculateTeamScore(TEAM_RED);   // Дорого
        teamScoreCache[1] = CG_CalculateTeamScore(TEAM_BLUE);  // Дорого
        lastScoreTime = cg.time;
    }
    
    *red = teamScoreCache[0];
    *blue = teamScoreCache[1];
}
// CG_CalculateTeamScore() = 10 000 циклов
// Вызывается 60 раз за секунду (каждый фрейм)
// Без кеша: 60 * 2 * 10 000 = 1 200 000 циклов/сек
// С кешем: 1 * 2 * 10 000 = 20 000 циклов/сек
// Speedup: 60x
```

---

## Pattern 3: Phase Processing

### Проблема: разбиение на фреймы

```c
// ❌ Плохо: фазовая обработка
static int currentPhase = 0;
static int processedCount = 0;

void CG_ProcessEntities_Phased(void) {
    const int BATCH_SIZE = 16;
    
    if (currentPhase == 0) {
        // Phase 1: validate
        for (int i = processedCount; i < processedCount + BATCH_SIZE; i++) {
            if (i >= MAX_GENTITIES) {
                currentPhase = 1;
                processedCount = 0;
                return;
            }
            ValidateEntity(i);  // Function call
        }
        processedCount += BATCH_SIZE;
        
    } else if (currentPhase == 1) {
        // Phase 2: process
        for (int i = processedCount; i < processedCount + BATCH_SIZE; i++) {
            if (i >= MAX_GENTITIES) {
                currentPhase = 0;
                processedCount = 0;
                return;
            }
            ProcessEntity(i);  // Function call
        }
        processedCount += BATCH_SIZE;
    }
}

// Вызывается каждый фрейм
// 1024 entities / 16 batch = 64 фрейма для полной обработки
// Overhead: phase checks, processedCount management, 2 function calls
// = ~200 циклов overhead на фрейм * 64 фрейма = 12 800 циклов
```

**Проблемы:**
1. **State management**: `currentPhase`, `processedCount`
2. **Branches**: `if (currentPhase == 0) ... else if ...`
3. **Delay**: полная обработка занимает 64 фрейма (1 секунда на 60 FPS)
4. **Function calls**: `ValidateEntity()`, `ProcessEntity()`

### Решение: один проход

```c
// ✅ Хорошо: простой один проход
void CG_ProcessEntities(void) {
    for (int i = 0; i < MAX_GENTITIES; i++) {
        centity_t* cent = &cg_entities[i];
        
        // Validate inline
        if (!cent->currentValid) {
            cent->processed = qfalse;
            continue;
        }
        
        // Process inline
        if (cent->currentState.eType == ET_PLAYER) {
            // ... processing ...
            cent->processed = qtrue;
        }
    }
}

// 1024 entities * ~10 операций = 10 240 циклов за один фрейм
// Нет state management, нет delays, нет function calls
```

**Когда фазы OK:**

```c
// ✅ Фазы OK: действительно тяжёлая операция
void CG_PrecacheSounds_Async(void) {
    static int soundIndex = 0;
    const int SOUNDS_PER_FRAME = 5;
    
    // Загрузка звуков по 5 за фрейм, чтобы не вызвать lag spike
    for (int i = 0; i < SOUNDS_PER_FRAME && soundIndex < totalSounds; i++) {
        trap_S_RegisterSound(soundList[soundIndex++]);  // Очень дорого (I/O)
    }
}

// trap_S_RegisterSound = 1 000 000+ циклов (disk I/O!)
// Загрузить 100 звуков за раз = 100 000 000 циклов = lag spike
// По 5 за фрейм = 5 000 000 циклов = OK, без лага
```

---

## Pattern 4: Deep Branching

### Проблема: много вложенных if

```c
// ❌ Плохо: 6 уровней вложенности
void CG_DrawPlayerInfo(int clientNum) {
    if (clientNum >= 0) {                                    // Branch 1
        if (clientNum < MAX_CLIENTS) {                       // Branch 2
            centity_t* cent = &cg_entities[clientNum];
            if (cent->currentValid) {                        // Branch 3
                if (cent->currentState.eType == ET_PLAYER) { // Branch 4
                    if (!(cent->currentState.eFlags & EF_DEAD)) { // Branch 5
                        if (cg_drawPlayerInfo.integer) {     // Branch 6
                            // Actually draw...
                            DrawPlayerName(cent);
                            DrawPlayerHealth(cent);
                            DrawPlayerArmor(cent);
                        }
                    }
                }
            }
        }
    }
}

// 6 branches = 6 conditional jumps
// Каждый jump = 5-10 циклов (если mispredicted — 20+ cycles)
// Total overhead: ~60-120 циклов только на branches
// Реальная работа: DrawPlayerName/Health/Armor
```

**Проблемы:**
1. **Deep nesting** = читать сложно
2. **6 jumps** = много overhead
3. **Branch misprediction** = дорого (CPU pipeline flush)

### Решение: early returns

```c
// ✅ Хорошо: early returns, flat structure
void CG_DrawPlayerInfo(int clientNum) {
    // Early checks
    if (clientNum < 0 || clientNum >= MAX_CLIENTS) return;
    if (!cg_drawPlayerInfo.integer) return;
    
    centity_t* cent = &cg_entities[clientNum];
    if (!cent->currentValid) return;
    if (cent->currentState.eType != ET_PLAYER) return;
    if (cent->currentState.eFlags & EF_DEAD) return;
    
    // Happy path: linear execution
    DrawPlayerName(cent);
    DrawPlayerHealth(cent);
    DrawPlayerArmor(cent);
}

// Те же 6 checks, но:
// - Flat structure (легче читать)
// - CPU лучше предсказывает (early returns часто предсказуемы)
// - Меньше jumps в happy path
```

### Ещё лучше: combine checks

```c
// ✅ Ещё лучше: объединить где возможно
void CG_DrawPlayerInfo(int clientNum) {
    if (!cg_drawPlayerInfo.integer) return;
    if (clientNum < 0 || clientNum >= MAX_CLIENTS) return;  // Combined
    
    centity_t* cent = &cg_entities[clientNum];
    
    // Combined: 3 checks в одном if
    if (!cent->currentValid || 
        cent->currentState.eType != ET_PLAYER ||
        (cent->currentState.eFlags & EF_DEAD)) {
        return;
    }
    
    // Happy path
    DrawPlayerName(cent);
    DrawPlayerHealth(cent);
    DrawPlayerArmor(cent);
}

// 3 if statements вместо 6
// Short-circuit evaluation: если первый false, остальные не проверяются
// Меньше jumps = быстрее
```

---

## Pattern 5: Syscall Spam

### Проблема: syscalls в loop

```c
// ❌ ПЛОХО: syscall в каждой итерации
void CG_AddAllEntities(void) {
    for (int i = 0; i < cg.snap->numEntities; i++) {
        refEntity_t ent;
        SetupEntity(&ent, i);
        trap_R_AddRefEntityToScene(&ent);  // Syscall! ~100 cycles
    }
}

// 100 entities * 100 cycles = 10 000 циклов только на syscall overhead
// Не считая реальной работы движка (rendering, culling, etc.)
```

**Проблема:**
- Каждый `trap_R_AddRefEntityToScene` = переход VM → engine → VM
- Overhead: сохранение контекста, вызов callback, восстановление
- Даже если entity не рендерится (out of frustum), overhead остаётся

### Решение 1: culling в VM

```c
// ✅ Лучше: culling перед syscall
void CG_AddAllEntities(void) {
    for (int i = 0; i < cg.snap->numEntities; i++) {
        centity_t* cent = &cg_entities[i];
        
        // Cheap checks in VM (no syscall)
        if (cent->currentState.eFlags & EF_NODRAW) continue;
        if (!CG_IsInFrustum(cent->lerpOrigin)) continue;  // Simple check
        
        // Only add if passed checks
        refEntity_t ent;
        SetupEntity(&ent, cent);
        trap_R_AddRefEntityToScene(&ent);
    }
}

// 100 entities, 30 out of frustum
// = 70 syscalls вместо 100
// Saved: 30 * 100 = 3 000 циклов
```

### Решение 2: batch syscalls (если engine поддерживает)

```c
// ✅ Ещё лучше: batch entities (если engine API позволяет)
void CG_AddAllEntities(void) {
    refEntity_t entities[MAX_ENTITIES];
    int count = 0;
    
    for (int i = 0; i < cg.snap->numEntities; i++) {
        if (ShouldDraw(i)) {
            SetupEntity(&entities[count++], i);
        }
    }
    
    // Один syscall вместо N
    trap_R_AddRefEntitiesToScene(entities, count);
}

// 1 syscall вместо 100
// Saved: 99 * 100 = 9 900 циклов!
```

**Примечание:** стандартный Q3 API не поддерживает batch, но можно добавить свой syscall.

### Решение 3: кеширование дорогих syscalls

```c
// ✅ Кешировать результаты trap_CM_BoxTrace (самый дорогой syscall)
typedef struct {
    vec3_t start, end;
    trace_t result;
    int frameNum;
} traceCache_t;

static traceCache_t traceCache[16];  // LRU cache
static int traceCacheIndex = 0;

void CG_Trace(trace_t* result, vec3_t start, vec3_t end) {
    // Check cache
    for (int i = 0; i < 16; i++) {
        if (traceCache[i].frameNum == cg.frameCount &&
            VectorCompare(traceCache[i].start, start) &&
            VectorCompare(traceCache[i].end, end)) {
            // Cache hit!
            *result = traceCache[i].result;
            return;
        }
    }
    
    // Cache miss: do syscall
    trap_CM_BoxTrace(result, start, end, NULL, NULL, 0, MASK_SOLID);
    
    // Store in cache
    traceCache[traceCacheIndex].frameNum = cg.frameCount;
    VectorCopy(start, traceCache[traceCacheIndex].start);
    VectorCopy(end, traceCache[traceCacheIndex].end);
    traceCache[traceCacheIndex].result = *result;
    traceCacheIndex = (traceCacheIndex + 1) % 16;
}

// trap_CM_BoxTrace = ~10 000 циклов (дорогая операция)
// Cache hit = ~100 циклов (VectorCompare + copy)
// Если hit rate 50% → saved 50% * 10 000 = 5 000 циклов на trace
```

---

## Оптимизация реальных примеров

### Пример 1: Draw функция с outline (OSP2-BE)

#### До оптимизации (паттерн из реального кода)

```c
// ❌ Медленно: много function calls
void CG_Player(centity_t* cent) {
    refEntity_t legs, torso, head;
    
    if (!CG_ShouldDrawPlayer(cent)) return;  // Call 1
    
    CG_PlayerAngles(cent);                   // Call 2
    CG_PlayerAnimation(cent);                // Call 3
    
    Com_Memset(&legs, 0, sizeof(legs));
    Com_Memset(&torso, 0, sizeof(torso));
    Com_Memset(&head, 0, sizeof(head));
    
    CG_SetupLegs(cent, &legs);               // Call 4
    CG_SetupTorso(cent, &torso);             // Call 5
    CG_SetupHead(cent, &head);               // Call 6
    
    if (CG_ShouldDrawOutline(cent)) {        // Call 7
        legs.renderfx |= RF_OUTLINE;
        torso.renderfx |= RF_OUTLINE;
        head.renderfx |= RF_OUTLINE;
    }
    
    trap_R_AddRefEntityToScene(&legs);       // Syscall 1
    trap_R_AddRefEntityToScene(&torso);      // Syscall 2
    trap_R_AddRefEntityToScene(&head);       // Syscall 3
}

// 64 игрока * (7 calls + 3 syscalls) = 640 overhead
```

#### После оптимизации

```c
// ✅ Быстро: inline checks, меньше calls
void CG_Player(centity_t* cent) {
    refEntity_t legs, torso, head;
    int outlineFlags = 0;
    
    // Inline check
    if (cent->currentState.number == cg.snap->ps.clientNum &&
        !cg.renderingThirdPerson) return;
    if (cent->currentState.eFlags & EF_NODRAW) return;
    
    // Inline animation update (hot path only, не выносим в функцию)
    CG_PlayerAngles(cent);      // Сложная, оставляем
    CG_PlayerAnimation(cent);   // Сложная, оставляем
    
    // Memset inline (компилятор может оптимизировать)
    Com_Memset(&legs, 0, sizeof(legs));
    Com_Memset(&torso, 0, sizeof(torso));
    Com_Memset(&head, 0, sizeof(head));
    
    // Setup inline (вместо 3 function calls)
    legs.reType = RT_MODEL;
    legs.hModel = cent->currentState.legsModel;
    VectorCopy(cent->lerpOrigin, legs.origin);
    // ... ещё 5-10 строк setup для legs ...
    
    torso.reType = RT_MODEL;
    torso.hModel = cent->currentState.torsoModel;
    VectorCopy(cent->lerpOrigin, torso.origin);
    // ... setup для torso ...
    
    head.reType = RT_MODEL;
    head.hModel = cent->currentState.headModel;
    VectorCopy(cent->lerpOrigin, head.origin);
    // ... setup для head ...
    
    // Outline check inline (macro expansion)
    if (CG_BE_FEATURE_ENABLED(BE_FEATURE_OUTLINE)) {
        if (cent->currentState.eType == ET_PLAYER &&
            cent->currentState.number != cg.clientNum) {
            outlineFlags = RF_OUTLINE;
        }
    }
    
    legs.renderfx |= outlineFlags;
    torso.renderfx |= outlineFlags;
    head.renderfx |= outlineFlags;
    
    trap_R_AddRefEntityToScene(&legs);
    trap_R_AddRefEntityToScene(&torso);
    trap_R_AddRefEntityToScene(&head);
}

// 64 игрока * (2 calls + 3 syscalls) = 192 overhead
// Speedup: 640 / 192 = 3.3x меньше overhead
```

**Что сделали:**
1. Inline `CG_ShouldDrawPlayer()` → 1 call убрали
2. Inline `CG_SetupLegs/Torso/Head()` → 3 calls убрали
3. Inline `CG_ShouldDrawOutline()` → 1 call убрали
4. Вычисляем outline flags один раз, применяем к 3 entities

**Результат:** 5 function calls → 2 calls = **2.5x меньше overhead**

---

### Пример 2: Team Task Marks (OSP2-BE реальный случай)

#### До оптимизации (антипаттерн)

```c
// ❌ Медленно: много мелких функций
void CG_ClearDeadMarks(int clientNum) {
    for (int i = 0; i < cgs.teamMarkers.numTeamTaskMarks; i++) {
        if (IsDeadMark(&cgs.teamMarkers.teamTaskMarks[i]) &&  // Call 1
            GetMarkClient(&cgs.teamMarkers.teamTaskMarks[i]) == clientNum) {  // Call 2
            RemoveMarkAtIndex(i);  // Call 3
            i--;  // Adjust после удаления
        }
    }
}

qboolean IsDeadMark(teamTaskMark_t* mark) {
    return mark->taskType == TT_PLAYERDEAD ||
           mark->taskType == TT_HERE_DEAD ||
           mark->taskType == TT_WARN_DEAD;
}

int GetMarkClient(teamTaskMark_t* mark) {
    return mark->taskClient;
}

void RemoveMarkAtIndex(int index) {
    // Shift все marks влево (O(n))
    for (int i = index; i < cgs.teamMarkers.numTeamTaskMarks - 1; i++) {
        cgs.teamMarkers.teamTaskMarks[i] = 
            cgs.teamMarkers.teamTaskMarks[i + 1];
    }
    cgs.teamMarkers.numTeamTaskMarks--;
}

// 64 marks * 3 calls = 192 function call overhead
// + O(n²) complexity из-за shift'ов в RemoveMarkAtIndex
```

#### После оптимизации (из QVM_LOGIC.md)

```c
// ✅ Быстро: один проход (packing), inline checks
void CG_ClearDeadMarks(int clientNum) {
    int w = 0;  // Write index
    
    for (int i = 0; i < cgs.teamMarkers.numTeamTaskMarks; i++) {
        teamTaskMark_t* mark = &cgs.teamMarkers.teamTaskMarks[i];
        
        // Inline check (no function call)
        qboolean isDead = (mark->taskType == TT_PLAYERDEAD ||
                          mark->taskType == TT_HERE_DEAD ||
                          mark->taskType == TT_WARN_DEAD);
        
        qboolean keep = !isDead || mark->taskClient != clientNum;
        
        if (keep) {
            if (w != i) {
                // Copy (only if needed)
                cgs.teamMarkers.teamTaskMarks[w] = *mark;
            }
            w++;
        }
    }
    
    cgs.teamMarkers.numTeamTaskMarks = w;
}

// 0 function calls
// O(n) complexity (один проход)
// Speedup: O(n²) → O(n) + 192 calls → 0 calls = ~10-20x faster
```

**Что сделали:**
1. **Packing** вместо shift'ов → O(n²) → O(n)
2. Inline `IsDeadMark()` и `GetMarkClient()` → 128 calls убрали
3. Inline `RemoveMarkAtIndex()` → 64 calls убрали (+ O(n) на каждый!)

**Результат:** O(n²) → O(n), 192 calls → 0 calls = **~20x faster**

---

## Измерение производительности

### 1. Engine timers (r_speeds)

```
// В игре:
/r_speeds 1

// Console output:
CG_DrawActiveFrame: 2.3ms
  CG_AddPacketEntities: 0.8ms
  CG_DrawPlayers: 0.5ms
  CG_DrawMarks: 0.1ms
  CG_DrawWeaponSelect: 0.05ms
```

**Используйте для:**
- Профилирование high-level функций
- Поиск bottleneck'ов
- Сравнение до/после оптимизации

### 2. Manual timers в коде

```c
// VM не имеет rdtsc, используем cg.time
int CG_Benchmark_DrawPlayers(void) {
    int startTime = trap_Milliseconds();
    
    // ... функция ...
    CG_DrawPlayers();
    
    int endTime = trap_Milliseconds();
    int elapsed = endTime - startTime;
    
    // Log раз в секунду
    static int lastLog = 0;
    if (cg.time - lastLog > 1000) {
        CG_Printf("CG_DrawPlayers: %d ms\n", elapsed);
        lastLog = cg.time;
    }
    
    return elapsed;
}
```

**Важно:**
- `trap_Milliseconds()` имеет разрешение ~1ms
- Для точного измерения нужно усреднять за N фреймов

### 3. Профилирование с DEBUG_VM

```c
// Компилируем VM с DEBUG_VM:
// #define DEBUG_VM в vm.h

// После сессии игры:
VM_VmProfile_f(&vm);

// Output:
// Function                      Calls      Cycles
// ---------------------------------------------
// CG_DrawActiveFrame            3600       180000000
// CG_AddPacketEntities          3600       80000000
// CG_Player                     230400     120000000  <- Hot spot!
// CG_AddRefEntityToScene        691200     50000000
```

**Интерпретация:**
- `CG_Player` вызывается 230 400 раз (64 игрока * 3600 frames)
- Берёт 120 000 000 циклов = **52% времени CG_DrawActiveFrame**
- **→ Это наш bottleneck, оптимизируем первым!**

### 4. Counting opcodes

```bash
# Дизассемблируем .qvm, считаем опкоды в функции
python disassemble_qvm.py cgame.qvm | grep -A 100 "CG_DrawPlayers" | wc -l
```

**Эвристика:**
- Меньше опкодов = быстрее (обычно)
- Много CALL/ENTER/LEAVE = много overhead
- Много EQ/NE/GT/LT (conditions) = много branches

### 5. A/B тестирование

```c
// Version A (старый код)
#define USE_OLD_CODE 0

void CG_DrawPlayers(void) {
#if USE_OLD_CODE
    // Старая версия
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (IsValidPlayer(i)) {  // Function call
            DrawPlayer(i);
        }
    }
#else
    // Новая версия (inline)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i >= 0 && i < MAX_CLIENTS) {  // Inline
            centity_t* cent = &cg_entities[i];
            if (cent->currentValid) {
                DrawPlayer(i);
            }
        }
    }
#endif
}
```

**Процесс:**
1. Измерить FPS с `USE_OLD_CODE 1` → 60 FPS
2. Измерить FPS с `USE_OLD_CODE 0` → 65 FPS
3. Speedup: +8% FPS

---

## Чеклист code review

### ✅ Performance checklist для hot paths

#### Function calls
- [ ] Нет тривиальных getters/setters (1-2 строки) в hot path
- [ ] Критичная логика inline'нута
- [ ] Сложные функции (> 20 строк) выделены отдельно
- [ ] Нет цепочек вызовов (A → B → C → D) для простых операций

#### Loops
- [ ] Минимум вложенных циклов (< 2 уровней)
- [ ] Нет function calls внутри горячих циклов
- [ ] Early continue для skip'ов
- [ ] Bounds checks до цикла, не в каждой итерации

#### Branches
- [ ] Early returns вместо deep nesting
- [ ] Combined conditions (`if (a && b)` вместо `if (a) { if (b) }`)
- [ ] Feature flags проверяются один раз, не N раз
- [ ] Избегаем `if/else if/else if/else if` chains (> 4)

#### Memory
- [ ] Нет лишних `Com_Memset` (используем только когда нужно)
- [ ] Struct field access inline (не через getters)
- [ ] Нет dynamic allocation (VM не имеет malloc)
- [ ] Локальные массивы умеренного размера (< 1 KB)

#### Syscalls
- [ ] Минимум syscalls в loops
- [ ] Culling перед `trap_R_AddRefEntity`
- [ ] Кеширование `trap_CM_BoxTrace` (самый дорогой)
- [ ] Batch операции где возможно

#### Caching
- [ ] Кеши только для дорогих операций (> 1000 cycles)
- [ ] Нет кешей для дешёвых checks (< 50 cycles)
- [ ] Cache invalidation простой (framerate check)
- [ ] Нет "умных" caching strategies (LRU, etc.)

#### Code quality
- [ ] Код читаем (не обфусцирован ради "оптимизации")
- [ ] Нет преждевременной оптимизации
- [ ] Профилировали перед оптимизацией
- [ ] Измерили speedup после оптимизации

---

## Заключение

### Приоритеты оптимизации

1. **Профилируйте сначала** — не оптимизируйте вслепую
2. **Оптимизируйте hot paths** — 80% времени в 20% кода
3. **Simple > Clever** — простой код часто быстрее в QVM
4. **Measure everything** — всегда проверяйте speedup

### Типичные speedup'ы

| Оптимизация | Speedup | Сложность |
|------------|---------|-----------|
| Inline trivial functions | 2-10x | Low |
| Remove cache overhead | 1.5-3x | Low |
| One-pass vs multi-pass | 1.5-5x | Medium |
| Flatten branches | 1.2-2x | Low |
| Reduce syscalls | 2-20x | Medium |
| Algorithmic (O(n²)→O(n)) | 10-100x | High |

### Когда оптимизировать?

✅ **Оптимизируйте:**
- Функции, вызываемые 1000+ раз за фрейм
- Hot paths с измеримым impact (> 1% frame time)
- Bottleneck'и из профилировщика

❌ **НЕ оптимизируйте:**
- Код, вызываемый < 10 раз за фрейм
- Меню и UI (не критично)
- Код, который уже быстрый (< 0.1 ms)

### Мантра QVM оптимизации

> **"Inline the hot, simplify the complex, measure the impact"**

---

*Документ создан для OSP2-BE на основе исследования q3vm-master и реальных паттернов оптимизации.*
*Версия: 1.0 | Дата: 2026-02-01*
