# Анализ производительности cgame: инпутлаг и подфризы

Цель: выявить потенциальные места, вызывающие высокие задержки (инпутлаг) и подфризы. Ниже — чеклист для отметки выполненных оптимизаций.

---

## Чеклист оптимизаций

Отмечайте выполненное: замените `[ ]` на `[x]`.

### Инпутлаг

- [ ] **CG_PredictPlayerState** — сократить/оптимизировать число trap-вызовов или трасс в Pmove
- [ ] **CG_ProcessSnapshots** — ограничить число trap_GetSnapshot/команд за кадр или размазать по кадрам
- [ ] **CG_ExecuteNewServerCommands** — размазать обработку пачки команд по кадрам
- [ ] **CG_UpdateCvars** — реже обновлять или уменьшить число trap_Cvar_Update за проход
- [ ] **CG_UpdateSelwheelMark** — уменьшить trap-вызовы при активном ttwheel
- [ ] **Timescale fade** — не вызывать trap_Cvar_Set каждый кадр (тик по таймеру или по изменению)

### Подфризы

- [x] **CG_ScanForCrosshairEntity** — кэшировать результат на кадр, не вызывать 3+ раз из HUD
- [ ] **CG_AddPacketEntities / CG_Player** — оптимизация при большом числе сущностей
- [ ] **CG_DrawTeamTaskMarks / Border** — оптимизация при большом числе меток
- [ ] **CG_ExecuteNewServerCommands** — не обрабатывать слишком много команд за один кадр
- [ ] **CG_UpdateAllClientsInfo** — отложенное/пошаговое обновление при спеке (cg_spectOrigModel)
- [ ] **CG_PrintLog (cg_showmiss)** — не писать в файл в горячем пути предсказания

### Мелкие улучшения

- [ ] **strlen(cgs.osp.testFont)** — убрать из горячего пути CG_Draw2D (кэш или проверка по флагу)
- [ ] **Timescale** — trap_Cvar_Set при фейде вызывать раз в N мс или только при изменении значения

---

## Сводная таблица (справочно)

| Чеклист | Участок | Частота | Влияние |
|--------|---------|--------|--------|
| см. выше | **CG_PredictPlayerState** | Каждый кадр | Сильный инпутлаг |
| см. выше | **CG_ProcessSnapshots** | Каждый кадр | Инпутлаг + подфризы при лаге |
| см. выше | **CG_ExecuteNewServerCommands** | При переходе снапшота | Подфризы при пачке команд |
| см. выше | **CG_UpdateCvars** | Раз в 6 кадров | Небольшие всплески |
| см. выше | **CG_AddPacketEntities / CG_Player** | Каждый кадр | Подфризы при многих сущностях |
| см. выше | **CG_ScanForCrosshairEntity** | 3+ раз за кадр (SuperHUD) | Подфризы, избыточные трассы |
| см. выше | **CG_UpdateSelwheelMark** | Каждый кадр (при ttwheel) | Небольшой инпутлаг |
| см. выше | **Timescale fade** | Каждый кадр во время фейда | Небольшой инпутлаг |
| см. выше | **CG_UpdateAllClientsInfo** | По событиям / спек | Сильные подфризы |
| см. выше | **CG_PrintLog (cg_showmiss + лог)** | При ошибках предсказания | Подфризы при лагах |
| см. выше | **CG_DrawTeamTaskMarks / Border** | Каждый кадр | Подфризы при многих метках |

---

## 1. Точка входа и порядок вызовов в кадре

**Цепочка кадра:** `vmMain(CG_DRAW_ACTIVE_FRAME)` → `CG_DrawActiveFrame()` → … → `CG_DrawActive()`.

В каждом кадре по порядку:
- `CG_UpdateCvars()` — раз в 6 кадров.
- `trap_S_ClearLoopingSounds`, `trap_R_ClearScene`.
- **CG_ProcessSnapshots()** — снапшоты, переходы, команды.
- **CG_UpdateMyTeam()** — обновление своей команды.
- `trap_SetUserCmdValue`.
- **CG_PredictPlayerState()** — предсказание игрока (горячий путь).
- **CG_UpdateSelwheelMark()** — позиция маркера ttwheel.
- **CG_CalcViewValues()** — refdef, камера.
- **CG_AddPacketEntities()** — все сущности.
- CG_AddMarks, CG_AddParticles, CG_AddLocalEntities.
- CG_AddViewWeapon, CG_PlayBufferedSounds, CG_PowerupTimerSounds.
- `trap_S_Respatialize`.
- **CG_AddLagometerFrameInfo()** — лагометр.
- При изменении timescale — `va()` + `trap_Cvar_Set("timescale", ...)` каждый кадр.
- **CG_DrawActive()** → CG_DrawTeamTaskBorderHudMarkers, CG_DrawSelwheelPointers, **trap_R_RenderScene**, **CG_DrawBefore2D()**, **CG_Draw2D()**.

---

## 2. Инпутлаг: детали по участкам

### 2.1 CG_PredictPlayerState() — основной источник задержки ввода

**Файл:** `cg_predict.c`

- Цикл по **CMD_BACKUP (64)** командам: от `current - 64 + 1` до `current`.
- На каждую команду: `trap_GetUserCmd(cmdNum, &cg_pmove.cmd)` — до 64 вызовов trap; при `pmove_fixed`/`cg_oversampleMouse` — `PM_UpdateViewAngles`; вызов **Pmove**.
- Pmove использует **CG_Trace** (trap_CM_BoxTrace + CG_ClipMoveToEntities) и **CG_PointContents** (trap_CM_* по solid-сущностям).

**Вывод:** предсказание — самое тяжёлое место по количеству trap-вызовов и по влиянию на задержку ввода.

---

### 2.2 CG_ProcessSnapshots()

**Файл:** `cg_snapshot.c`

- `trap_GetCurrentSnapshotNumber`; циклы с **CG_ReadNextSnapshot()** (внутри — **trap_GetSnapshot**); при переходе снапшота — **CG_BuildSolidList()**, **CG_ExecuteNewServerCommands()**.
- **CG_ExecuteNewServerCommands()**: цикл по отставшим командам — **trap_GetServerCommand** + **CG_ServerCommand()** (парсинг, ветвление). При пачке команд — всплеск времени.
- При `cg_spectOrigModel` при смене игрока — **CG_UpdateAllClientsInfo()** (много trap_R_*, trap_S_*, trap_FS_*).

**Вывод:** пики времени при нестабильном пинге или большом числе команд; при спеке — тяжёлые кадры из-за CG_UpdateAllClientsInfo.

---

### 2.3 CG_UpdateCvars()

**Файл:** `cg_main.c`

- Раз в **6 кадров**: цикл по **cvarTable** — **trap_Cvar_Update(cv->vmCvar)** и при изменении — callback `onChanged`.

**Вывод:** раз в 6 кадров — всплеск по trap-вызовам; небольшая неравномерность задержки.

---

### 2.4 CG_UpdateSelwheelMark()

**Файл:** `cg_teamtasks_selwheel.c`

- Каждый кадр при активном ttwheel: **trap_GetCurrentCmdNumber()**, **trap_GetUserCmd(...)** — два trap на кадр.

**Вывод:** небольшой постоянный вклад в инпутлаг при открытом ttwheel.

---

### 2.5 Timescale fade (cg_view.c)

- Во время фейда каждый кадр: пересчёт value, **va("%f", ...)** и **trap_Cvar_Set("timescale", ...)**.

**Вывод:** лишний trap + форматирование каждый кадр во время фейда.

---

## 3. Подфризы: детали по участкам

### 3.1 CG_AddPacketEntities() и цепочка по сущностям

**Файл:** `cg_ents.c`

- **CG_CalcEntityLerpPositions()** для predicted player и для каждой сущности; при `cg_projectileNudge` — **CG_Trace** для снарядов.
- Для каждой сущности: **CG_AddCEntity()** → **CG_CalcEntityLerpPositions**, **CG_EntityEffects**, по типу **CG_Player()** и др.
- **CG_Player()** (cg_players.c): **trap_CM_PointContents**, **trap_R_AddRefEntityToScene**, при эффектах — **trap_R_AddLightToScene**, **trap_S_AddLoopingSound**.

**Вывод:** при большом числе сущностей — подфризы и всплески в середине кадра.

---

### 3.2 CG_ScanForCrosshairEntity() — дублирование трасс

**Файл:** `cg_draw.c`

- Внутри: **два вызова CG_Trace** (разные маски).
- Вызывается из: **CG_DrawCrosshair()**, **CG_DrawCrosshairNames()**, элементов SuperHUD **target_status**, **target_name** — при SuperHUD получается **3+ вызова** → **6+ трасс в кадр**.

**Вывод:** избыточные трассы; логично кэшировать результат на кадр и переиспользовать.

**Сделано:** кэш по кадру: в начале `CG_ScanForCrosshairEntity` проверка `cg.crosshairScanTime == cg.time`; при совпадении — выход без трасс. После проверки присвоение `cg.crosshairScanTime = cg.time`. В результате трассы выполняются один раз за кадр.

---

### 3.3 Отрисовка 2D: CG_DrawBefore2D / CG_Draw2D

- **CG_DrawBefore2D:** CG_UpdateHealthLowFlag, при опциях — overlay/damage frame, ttwheel, **CG_DrawTeamTaskMarks()**, team indicator.
- **CG_DrawTeamTaskBorderHudMarkers()**: **CG_RemoveExpiredTeamTaskMarks()**, цикл по меткам, **CG_IsPointOutsideScreen**, **CG_DrawTeamTaskBorderMarker**.
- **CG_DrawTeamTaskMarks():** второй проход по меткам; при своих метках над головой — ещё цикл.
- **CG_Draw2D():** **strlen(cgs.osp.testFont)** каждый кадр; **CG_SHUDRoutine()** / **CG_OSPHUDRoutine()**; в **CG_SHUDRoutine()** — **CG_DrawCrosshair()** и цикл по элементам SuperHUD (каждый может делать трассу и т.д.).

**Вывод:** дублирование трасс прицела и тяжёлые элементы HUD дают подфризы в 2D-части кадра.

---

### 3.4 CG_ExecuteNewServerCommands и CG_ServerCommand

**Файл:** `cg_servercmds.c`

- При переходе снапшота: **trap_GetServerCommand** + **CG_ServerCommand()** (разбор строки, ветвление). Часть команд — тяжёлые (скор, конфиги, обновление клиентов). При отставании по командам за один кадр может выполниться много команд.

**Вывод:** выраженные подфризы в кадрах с большим пачётом серверных команд.

---

### 3.5 CG_UpdateAllClientsInfo

**Файл:** `cg_players.c`

- Цикл по **MAX_CLIENTS**; для каждого с непустым configstring — **CG_NewClientInfo()** (модели, скины, звуки, файлы — множество trap_R_*, trap_S_*, trap_FS_*).
- Вызывается при первом снапшоте, рестарте, при смене игрока в спеке (cg_spectOrigModel), из **cg_localevents.c** при изменении configstring.

**Вывод:** при спеке или массовом обновлении — сильные подфризы из-за регистрации ресурсов и I/O.

---

### 3.6 Логирование и вывод в консоль

**CG_Printf** (cg_main.c): **CG_PrintLog()** при открытом логе — **va()**, **trap_FS_Write()**; **trap_Print(text)**. При **cg_showmiss** в **CG_PredictPlayerState** при ошибках предсказания вызывается **CG_Printf** → синхронная запись в файл в середине предсказания.

**Вывод:** логирование в файл в горячем пути даёт заметные подфризы при лагах.

---

## 4. Рекомендации по приоритетам (ориентир)

1. **Инпутлаг:** в первую очередь — **CG_PredictPlayerState** (число команд, трасс в Pmove, CG_Trace/CG_ClipMoveToEntities) и **CG_ProcessSnapshots** + **CG_ExecuteNewServerCommands**.
2. **Подфризы:** кэш результата **CG_ScanForCrosshairEntity** на кадр; размазать по кадрам **CG_ExecuteNewServerCommands** и **CG_UpdateAllClientsInfo**; при спеке с cg_spectOrigModel — отложенное/пошаговое обновление клиентов.
3. **Мелкие улучшения:** убрать **strlen(cgs.osp.testFont)** из горячего пути; при фейде timescale не вызывать trap_Cvar_Set каждый кадр (тик раз в N мс или по изменению значения).
