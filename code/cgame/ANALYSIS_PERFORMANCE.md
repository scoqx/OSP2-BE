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
- [x] **CG_UpdateSelwheelMark** — уменьшить trap-вызовы при активном ttwheel
- [ ] **Timescale fade** — не вызывать trap_Cvar_Set каждый кадр (тик по таймеру или по изменению)

### Подфризы

- [x] **CG_ScanForCrosshairEntity** — кэшировать результат на кадр, не вызывать 3+ раз из HUD
- [ ] **CG_AddPacketEntities / CG_Player** — оптимизация при большом числе сущностей
- [ ] **CG_DrawTeamTaskMarks / Border** — оптимизация при большом числе меток
- [x] **BE: CG_DrawPlayerIndicatorOnScreen** — ранний выход: только infoValid; в игре только союзники, без тяжёлых проверок (см. раздел 6.1 п.2)
- [x] **BE: CG_FriendVisible** — кэш по кадру и по entity index (см. раздел 6.1 п.1)
- [ ] **BE: CG_AddOutline** — 3× refentity на игрока
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
| см. выше | **CG_UpdateCvars** | Раз в 12 кадров | Небольшие всплески |
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
- `CG_UpdateCvars()` — раз в 12 кадров.
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

- Раз в **12 кадров** (условие `cvarsUpdateDelay++ > 11`): цикл по **cvarTable** — **trap_Cvar_Update(cv->vmCvar)** и при изменении — callback `onChanged`.

**Вывод:** раз в 12 кадров — всплеск по trap-вызовам; небольшая неравномерность задержки.

#### CG_UpdateCvars — обновлять только при изменении?

**Почему не «только при изменении»:** QVM не знает, что cvar изменился, пока не спросит движок. Текущий API: **trap_Cvar_Update(vmCvar)** копирует значение и modificationCount из движка в VM; движок увеличивает modificationCount при изменении cvar (консоль, set и т.д.). Узнать «изменился ли cvar» без вызова trap_Cvar_Update нельзя — в VM нет отдельного trap вида «верни только modificationCount».

**Варианты без правок движка:**

1. **Обновлять только при открытой консоли/меню**  
   Cvar меняются в основном из консоли или меню. Вызов **trap_Key_GetCatcher()**: если `(catcher & (KEYCATCH_CONSOLE | KEYCATCH_UI)) != 0` — консоль или меню открыты. В этом режиме вызывать **CG_UpdateCvars** раз в 12 кадров (как сейчас). Когда `catcher == 0` — не вызывать. При переходе catcher из ненулевого в 0 (консоль/меню закрыли) — один раз вызвать **CG_UpdateCvars**, чтобы подхватить изменения. В итоге в геймплее (консоль закрыта) trap_Cvar_Update не вызываются вообще.

2. **Редкий опрос при закрытой консоли**  
   Если консоль закрыта, всё равно раз в N кадров (например 30–60) вызывать **CG_UpdateCvars**, чтобы подхватить изменения из exec/config или серверных команд. Компромисс: меньше всплесков, но задержка подхвата до N кадров.

3. **Гибрид**  
   Консоль/меню открыты → обновление раз в 12 кадров. Консоль закрыта → не обновлять, но при переходе catcher в 0 — один полный проход; плюс раз в 60 кадров один проход на случай смены cvar без консоли (exec, сервер).

4. **Реже текущего опроса**  
   Оставить периодический опрос, но вызывать **CG_UpdateCvars** раз в 15–30 кадров вместо 6. Меньше trap-вызовов, задержка подхвата cvar до 15–30 кадров (часто приемлемо).

**Вариант с правкой движка (для справки):**  
Движок мог бы экспортировать trap вида «верни modificationCount для cvar по имени» или «верни битовую маску изменённых cvar за последний кадр». Тогда VM вызывал бы **trap_Cvar_Update** только для действительно изменившихся cvar. Требует изменений в коде движка (в репозитории OSP2 его нет).

---

### 2.4 CG_UpdateSelwheelMark()

**Файл:** `cg_teamtasks_selwheel.c`

- Каждый кадр при активном ttwheel: **trap_GetCurrentCmdNumber()**, **trap_GetUserCmd(...)** — два trap на кадр.

**Вывод:** небольшой постоянный вклад в инпутлаг при открытом ttwheel.

**Сделано:** **trap_GetUserCmd()** вызывается только при смене номера команды (`currentCmdNum != lastCmdNum`). Если за кадр не пришла новая команда ввода, второй trap не вызывается — один trap на кадр вместо двух в типичных случаях (частота ввода ≤ частота кадров).

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

- <!-- **CG_DrawBefore2D:** CG_UpdateHealthLowFlag, при опциях — overlay/damage frame, ttwheel, --> **CG_DrawTeamTaskMarks()**, team indicator.
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
- Вызывается при первом снапшоте, рестарте, при смене игрока в спеке (cg_spectOrigModel), из **cg_localevents.c** при изменении cvars (cg_teamColors, cg_teamModel, cg_swapSkins и т.д.) — смена влияет на отображение всех игроков, поэтому обновляют всех.

**Вывод:** при спеке или массовом обновлении — сильные подфризы из-за регистрации ресурсов и I/O.

#### CG_UpdateAllClientsInfo — предложения по снижению подфризов

1. **Размазать по кадрам (отложенное обновление)**  
   Вместо обновления всех 64 клиентов в одном кадре: при вызове **CG_UpdateAllClientsInfo** выставлять флаг «ожидает полное обновление clientinfo» и запоминать «осталось обновить N клиентов» (или список индексов). Каждый кадр в подходящем месте (например, в начале **CG_DrawActiveFrame** или после **CG_ProcessSnapshots**) вызывать функцию, которая обрабатывает **4–8 клиентов** из очереди и сбрасывает флаг, когда очередь пуста. Таким образом, 64 вызова **CG_NewClientInfo** распределяются на 8–16 кадров; один кадр не получает пика по trap и I/O. Несколько кадров часть игроков может отображаться со старыми моделью/цветами — обычно приемлемо при смене cvar или переключении спека.

2. **Приоритет: сначала свой и наблюдаемый**  
   При пошаговом обновлении в первый кадр обновлять **cg.clientNum** (локальный игрок) и, в спеке, **игрока, за которым следим** (cg.snap->ps.clientNum), чтобы их модель/цвет обновились сразу. Остальных — в следующих кадрах по 4–8 за раз.

3. **Смена спек-игрока (cg_spectOrigModel)**  
   При переключении наблюдаемого игрока вызывается **CG_UpdateAllClientsInfo**. Можно не менять момент вызова, а внутри использовать тот же механизм «очередь + N клиентов за кадр»: в первый кадр обновить только нового follow target, затем в фоне — остальных. Тогда тяжёлый кадр при переключении спека исчезает.

4. **Не трогать первый снапшот / рестарт**  
   При самом первом снапшоте или рестарте карты полное обновление за один кадр может остаться (экран загрузки и т.д.). Либо тоже размазать, если загрузка уже скрыта и возможны подфризы.

---

### 3.6 Логирование и вывод в консоль

**CG_Printf** (cg_main.c): **CG_PrintLog()** при открытом логе — **va()**, **trap_FS_Write()**; **trap_Print(text)**. При **cg_showmiss** в **CG_PredictPlayerState** при ошибках предсказания вызывается **CG_Printf** → синхронная запись в файл в середине предсказания.

**Вывод:** логирование в файл в горячем пути даёт заметные подфризы при лагах.

---

## 4. Рекомендации по приоритетам (ориентир)

1. **Инпутлаг:** в первую очередь — **CG_PredictPlayerState** (число команд, трасс в Pmove, CG_Trace/CG_ClipMoveToEntities) и **CG_ProcessSnapshots** + **CG_ExecuteNewServerCommands**.
2. **Подфризы:** кэш результата **CG_ScanForCrosshairEntity** на кадр; размазать по кадрам **CG_ExecuteNewServerCommands** и **CG_UpdateAllClientsInfo**; при спеке с cg_spectOrigModel — отложенное/пошаговое обновление клиентов.
3. **Мелкие улучшения:** убрать **strlen(cgs.osp.testFont)** из горячего пути; при фейде timescale не вызывать trap_Cvar_Set каждый кадр (тик раз в N мс или по изменению значения).

---

## 5. Анализ: be_enabled 0 vs 1 при большом числе игроков (Outline, players.c)

**Цель:** выяснить, почему при `be_enabled 0` игра идёт заметно плавнее при многих игроках. Основные подозреваемые — Outline и проверки в `cg_players.c`.

### 5.1 Что отключается при be_enabled 0

- **CG_BE_FEATURE_ENABLED(x)** всегда false (макрос: `BE_ENABLED && !(cgs.be.disableFeatures & (feature))`).
- Не вызываются: **CG_DrawPlayerIndicatorOnScreen**, **CG_DrawTeamTaskMarks**, Outline в **CG_Player**, часть веток wallhack/frozen и alt shadow в **CG_PlayerShadow** / **CG_FriendHudMarker**.

### 5.2 Слабые места при be_enabled 1 (высокая нагрузка)

#### A. CG_DrawPlayerIndicatorOnScreen (cg_draw.c)

- Вызывается каждый кадр при `CG_BE_FEATURE_ENABLED(CG_BE_TEAM_INDICATOR) && cg_teamIndicator.integer`.
- Цикл **for (i = 0; i < scanRange; i++)**, где `scanRange = MAX_CLIENTS` (64) или при freeze — **MAX_GENTITIES** (1024).
- На каждой итерации: **CG_IsPlayerValidAndVisible(i, useWallhack, ...)**. Внутри при `needVis` вызывается **CG_FriendVisible(cent)**.
- **CG_FriendVisible** (cg_players.c): два вызова **CG_Trace** (vieworg → lerpOrigin и vieworg → lerpOrigin+24 по Z).

**Итого:** до **64 × 2 = 128 трасс/кадр** (обычный режим) или до **1024 × 2 = 2048 трасс/кадр** (freeze) только на отрисовку индикаторов союзников на экране. При `be_enabled 0` весь этот блок не выполняется — одна из главных причин улучшения FPS.

#### B. CG_FriendVisible (cg_players.c)

- Вызывается из **CG_IsPlayerValidAndVisible** (индикатор, см. выше) и из **CG_FriendHudMarker** (для каждого игрока в **CG_Player** при `cg_drawHudMarkers.integer`).
- Каждый вызов — **2 × CG_Trace** (тяжёлые trap-вызовы). Кэша по кадру/игроку нет: один и тот же игрок может проверяться и в цикле индикатора, и в **CG_FriendHudMarker**, что даёт дублирование трасс.

#### C. Outline (CG_AddOutline, cg_players.c)

- При `CG_BE_FEATURE_ENABLED(CG_BE_OUTLINE) && cg_drawOutline.integer` вызывается **три раза за игрока**: для legs, torso, head.
- Каждый вызов: установка **ent->customShader** (outlineShader/teamOutlineShader), расчёт цвета (в т.ч. **CG_BE_MARK_TEAMMATE**, **cg_enemyOutlineColorUnique**), **trap_R_AddRefEntityToScene(ent)**.
- Итого **3 дополнительных refentity на игрока** с outline-шейдером. При 32 видимых игроках — до 96 лишних draw calls и перекрытий. При `be_enabled 0` все три вызова пропускаются — меньше нагрузка на рендер и бatching.

#### D. CG_Player — прочие проверки на игрока (cg_players.c)

- **CG_PlayerSprites(cent)** → **CG_HasActiveTeamTaskOverhead(clientNum)**: при BE teamtasks — цикл по **cgs.teamMarkers.numTeamTaskMarks** для каждого игрока. При `be_enabled 0` сразу выход.
- **CG_FriendHudMarker(cent)** при `cg_drawHudMarkers.integer`: снова **CG_FriendVisible(cent)** — 2 трассы на каждого союзника, видимого в мире (в дополнение к трассам в индикаторе). Кэш CG_FriendVisible по entity уже убирает повторные трассы для одного cent в одном кадре, но внутри CG_FriendHudMarker CG_FriendVisible вызывается до двух раз (две ветки) — лишний вызов.
- **CG_PlayerShadow(cent)**: **trap_CM_BoxTrace** — один раз на игрока (есть и без BE); при BE — дополнительно выбор alt shadow и цвета.

#### CG_FriendHudMarker — предложения по снижению трасс

1. **Один вызов CG_FriendVisible на маркер**  
   В **CG_FriendHudMarker** сейчас в двух условиях вызывается **CG_FriendVisible(cent)** (заморозка и wallhack). Вычислить `visible = CG_FriendVisible(cent)` один раз в начале (после ранних return) и в обеих проверках использовать `visible`. Трасс не станет меньше на союзника (всё равно 2 трассы при первом обращении к этому cent за кадр), но убирается второй вызов функции и повторная проверка кэша.

2. **Общий кэш видимости с индикатором (batch)**  
   Обновлять кэш «кто видим» (как в **CG_DrawPlayerIndicatorOnScreen**) до **CG_AddPacketEntities**, например в начале кадра или перед 3D-фазой. И индикатор, и **CG_FriendHudMarker** тогда читают один и тот же кэш: в **CG_FriendHudMarker** не вызывать **CG_FriendVisible**, а смотреть в этот кэш по entity index / clientNum. Трасс будет только 4×2 за кадр (batch 4), но видимость для 3D-маркеров станет с задержкой на несколько кадров (как у индикатора). Подходит, если допустима задержка появления/исчезновения маркера над головой.

3. **Оставить как есть**  
   Кэш **CG_FriendVisible** по entity уже даёт не более одной пары трасс на cent за кадр: один и тот же игрок не проверяется дважды в одном кадре (индикатор и маркер делят кэш). Дополнительно можно только уменьшить число вызовов функции в CG_FriendHudMarker (п. 1) или перейти на batch-кэш с задержкой (п. 2).

### 5.3 Сводка: почему при be_enabled 0 лучше при многих игроках

| Участок | При be_enabled 1 | При be_enabled 0 |
|--------|------------------|-------------------|
| CG_DrawPlayerIndicatorOnScreen | До 128–2048 трасс/кадр (цикл по 64/1024, CG_FriendVisible) | Не вызывается |
| CG_AddOutline (на игрока) | 3× trap_R_AddRefEntityToScene | Не вызывается |
| CG_FriendHudMarker / CG_FriendVisible | 2 трассы на союзника в мире | Трассы остаются (не завязаны на BE) |
| CG_HasActiveTeamTaskOverhead | Цикл по меткам на каждого игрока | Ранний выход |

Главные факторы падения кадров при включённом BE и большом числе игроков:
1. **Огромное число трасс** в **CG_DrawPlayerIndicatorOnScreen** (цикл по всем клиентам/сущностям + CG_FriendVisible).
2. **Outline**: по 3 лишних refentity на игрока (draw calls и перекрытия).
3. **Дублирование трасс**: один и тот же игрок проверяется в индикаторе и в CG_FriendHudMarker без кэша.

---

## 6. Что можно сделать (BE / players / Outline)

### 6.1 Высокий приоритет

1. **Кэш CG_FriendVisible по кадру и по игроку**  
   Ввести массив (или флаг по clientNum/entityNum) вида `visibleLastFrame[clientNum] == cg.time` и при совпадении возвращать закэшированный результат вместо двух трасс. Сбрасывать кэш в начале кадра. Это сократит трассы и в **CG_DrawPlayerIndicatorOnScreen**, и в **CG_FriendHudMarker**.  
   **Сделано:** static local friendVisibleCacheTime/Result[MAX_GENTITIES] в CG_FriendVisible, ключ по entity index, инвалидация по cg.time.

2. **Ограничить цикл в CG_DrawPlayerIndicatorOnScreen**  
   Не итерировать по всем 64/1024 слотам: сначала собирать список только валидных игроков (например, по флагу currentValid / eType == ET_PLAYER), затем для этого списка вызывать **CG_IsPlayerValidAndVisible** и рисовать индикатор. Либо ранний выход по числу уже отрисованных индикаторов (лимит N на кадр).  
   **Сделано:** ранний выход по !infoValid; в игре (не спектатор) — только союзники, враги пропускаются без тяжёлых проверок.


3. **Outline: один проход вместо трёх refentity на игрока**  
   По возможности объединить legs/torso/head в один outline-вызов (зависит от возможностей рендера) или отключать outline по дистанции/LOD (например, не рисовать outline для игроков дальше X единиц).

### 6.2 Средний приоритет

4. **Размазать CG_DrawPlayerIndicatorOnScreen по кадрам**  
   Обрабатывать не всех игроков за один кадр, а, например, 8–16 за кадр по кольцу (индекс начала сдвигать каждый кадр). Видимость и трассы будут обновляться с задержкой в 1–2 кадра, но нагрузка станет ровнее.  
   **Сделано:** обновление кэша видимости — по 2 слота за кадр (CG_INDICATOR_BATCH_SIZE), round-robin; отрисовка всех закэшированных индикаторов каждый кадр (без мигания).

5. **CG_HasActiveTeamTaskOverhead**  
   Не проходить по всем меткам для каждого игрока: построить один раз за кадр множество clientNum’ов с активной overhead-меткой и в **CG_PlayerSprites** делать только проверку по этому множеству (O(1) на игрока).  
   **Сделано:** раз за кадр (по cg.time) строится overheadClientSet[MAX_CLIENTS], проверка — один lookup.

### 6.3 Низкий приоритет

6. **CG_IsEnemy** вызывается многократно за кадр; функция лёгкая (без trap), но при желании можно кэшировать результат по (clientNum, cg.time) на кадр для одних и тех же запросов.  
   **Сделано:** static local isEnemyCacheTime/Result[MAX_CLIENTS], ключ (target - cgs.clientinfo), инвалидация по cg.time.

7. **CG_PlayerShadow**: trap_CM_BoxTrace один раз на игрока — оставить как есть; при необходимости позже рассмотреть кэш по позиции/кадру для неподвижных игроков.

---

## 7. cg_players.c — прочие участки (дополнительный обзор)

Участки, не завязанные на BE, но влияющие на нагрузку при большом числе игроков.

### 7.1 CG_PlayerSplash (игрок в воде)

- Вызывается из **CG_Player** каждый кадр для каждого игрока.
- **2× trap_CM_PointContents** (ноги, голова) + **1× trap_CM_BoxTrace** (поиск поверхности воды). Ранний выход, если ноги не в жидкости.
- При многих игроках в воде — до 3× N trap-вызовов за кадр (N — игроки в воде).
- **Идеи:** кэш по (entityIndex, lerpOrigin): если позиция не менялась с прошлого кадра, повторно использовать результат «в воде / поверхность» и не вызывать PointContents/BoxTrace. Либо вызывать только при заметном изменении Z или позиции.

### 7.2 CG_PlayerShadow (уже в п. 6.3.7)

- **1× trap_CM_BoxTrace** на игрока за кадр (вниз до земли).
- При желании: кэш по (entityIndex, lerpOrigin) на кадр — для неподвижных игроков не повторять трассу.

### 7.3 CG_PlayerPowerups

- На игрока с пауэрпапами: **trap_R_AddLightToScene** (quad, battlestation и т.д.), **trap_S_AddLoopingSound** (flight). Без trap_CM, нагрузка умеренная; при большом числе игроков с quad/flight — вклад в рендер и звук.
- Оптимизация не приоритетна; при необходимости — лимит числа одновременно отображаемых огней или один общий звук полёта.

### 7.4 CG_IsPlayerAvailable / CG_GetAimContactPoint

- **Цикл по MAX_GENTITIES** (1024) для поиска сущности игрока по clientNum. Вызываются по запросу (меню, прицел и т.д.), не в горячем пути **CG_AddPacketEntities**.
- Если эти функции вызываются часто (например, каждый кадр из HUD), имеет смысл кэшировать результат по (clientNum, cg.time) или по номеру снапшота: «последний найденный cent для clientNum» и проверка currentValid.  
  **Сделано (CG_IsPlayerAvailable):** static local isPlayerAvailableCacheTime/Result[MAX_CLIENTS], ключ clientNum, инвалидация по cg.time; локальный игрок без кэша.

### 7.5 CG_ResetPlayerEntity (при смене модели)

- Цикл **for (i = 0; i < MAX_GENTITIES; i++)** при смене clientinfo/модели игрока — вызывается редко (смена команды, скина), не в каждом кадре. Оставлять как есть.

### 7.6 Итог по cg_players.c

| Участок              | Частота              | Тяжесть                          | Приоритет оптимизации |
|----------------------|----------------------|----------------------------------|------------------------|
| CG_PlayerSplash      | Кадр × игроки в воде | 2 PointContents + 1 BoxTrace     | Низкий                 |
| CG_PlayerShadow      | Кадр × игрок         | 1 BoxTrace                       | Уже учтён (п. 7)       |
| CG_PlayerPowerups    | Кадр × игрок с пауп. | AddLight, AddLoopingSound       | Низкий                 |
| CG_IsPlayerAvailable | По запросу           | Цикл MAX_GENTITIES               | Средний при частом вызове |
| CG_GetAimContactPoint| По запросу           | Цикл + 1 CG_Trace                | Средний при частом вызове |

---

## 8. Анализ cg_draw.c

**Точка входа:** `CG_DrawActive(stereoView)` → CG_TileClear, CG_DrawTeamTaskBorderHudMarkers, CG_DrawSelwheelPointers, **trap_R_RenderScene**, **CG_DrawBefore2D()**, **CG_Draw2D()**. Вся отрисовка 2D и часть «до 2D» (маркеры, ttwheel, team indicators) идёт из этого файла.

### 8.1 Цепочка CG_Draw2D

- Ветвление по `cg.levelShot`, `cg_draw2D`, `strlen(cgs.osp.testFont)` (уже в чеклисте — кэш/флаг), credits, shud, OSPHUD, intermission, spectator vs alive.
- В обычной игре: CG_DrawStatusBar, CG_DrawAmmoWarning, CG_DrawCrosshair, CG_DrawCrosshairNames, CG_DrawWeaponSelect, CG_DrawHoldableItem, CG_DrawReward, CG_DrawTeamInfo, CG_DrawVote, CG_DrawLagometer, **CG_DrawUpperRight**, **CG_DrawLowerRight**, **CG_DrawLowerLeft**, CG_DrawFollow, CG_DrawWarmup, CG_DrawIntermission, CG_OSPDrawCenterString.
- **CG_DrawBefore2D** (до основного 2D): CG_DrawDamageFrame, CG_DrawHealthLowOverlay, CG_DrawSelwheelMark, CG_DrawTeamTaskMarks, **CG_DrawPlayerIndicatorOnScreen** (при BE team indicator).

### 8.2 Участки с тяжёлыми операциями

#### CG_CrosshairTarget (cg_draw.c)

- **Вызовы:** из `cg_consolecmds.c` (по команде) и из `cg_teamtasks_selwheel.c` при **первом открытии** ttwheel с «smart mode» (bitmask & 2) — один раз при открытии колеса, не каждый кадр.
- **Что делает:** 1× CG_Trace(MASK_SHOT); проверка frozen/player; затем **цикл `for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)`** по всем сущностям. Для каждой ET_ITEM вызывается **CG_IsLookingAtItemHitbox** → **trap_CM_TempBoxModel** + **trap_CM_TransformedBoxTrace**; при попадании по hitbox — ещё **CG_Trace** для видимости. Порядок: до ~(MAX_GENTITIES - MAX_CLIENTS) итераций, в каждой — проверки и при подходящем айтеме — 2 trap_CM + 1 CG_Trace.
- **Влияние:** при вызове (открытие ttwheel / консольная команда) — заметный всплеск. Если в будущем CG_CrosshairTarget начнут вызывать каждый кадр из HUD — станет горячей точкой.
- **Рекомендации:** (1) кэш по кадру: результат (targetType, itemPos) по ключу (cg.time или snapshot); вызывающие раз в кадр читают кэш. (2) Сузить цикл по айтемам: только сущности в текущем снапшоте и/или в радиусе/конусе от камеры; ранний выход при первом найденном подходящем айтеме уже есть по смыслу кода.

#### CG_IsLookingAtItemHitbox (static, вызывается из CG_CrosshairTarget)

- **trap_CM_TempBoxModel** + **trap_CM_TransformedBoxTrace** на каждый проверяемый айтем в цикле CG_CrosshairTarget. Дорого внутри цикла по MAX_GENTITIES.
- Оптимизация — вместе с сужением цикла в CG_CrosshairTarget (меньше вызовов).

#### CG_ScanForCrosshairEntity

- **Сделано:** кэш по кадру (`cg.crosshairScanTime == cg.time`) — трассы и trap_CM_PointContents выполняются один раз за кадр. Дополнительные вызовы из CG_DrawCrosshairNames, SuperHUD, OSPHUD только читают результат.

#### CG_DrawTeamOverlay

- Цикл **for (i = 1; i < MAX_LOCATIONS; i++)** — в каждой итерации **CG_ConfigString(CS_LOCATIONS + i)** (trap). До ~64 вызовов trap при каждом отображении оверлея.
- Внутренний цикл по игрокам (до 8): для каждого игрока цикл **for (j = 0; j <= PW_NUM_POWERUPS; j++)** с **trap_R_RegisterShader(item->icon)** при наличии паверапа. **RegisterShader** в цикле — дорого; шейдеры иконок паверапов не меняются, их можно кэшировать (таблица по powerup index, заполняется при первом использовании или при загрузке).

#### CG_DrawPowerups (нижний правый угол)

- Аналогично: для каждого активного паверапа в цикле вызывается **trap_R_RegisterShader(item->icon)**. Та же рекомендация — кэш шейдеров иконок паверапов (глобально или в cgs).

#### CG_DrawPlayerIndicatorOnScreen

- **Сделано:** использует кэш видимости (CG_UpdateIndicatorVisibilityCache в cg_players.c, по 2 слота за кадр); здесь только цикл по `scanRange` (MAX_CLIENTS или в freeze MAX_GENTITIES) и чтение кэша + CG_DrawPlayerIndicator. Тяжёлых trap в цикле нет; в режиме freeze 1024 итерации — в основном проверки и отрисовка уже отфильтрованных. При необходимости можно ограничить число рисуемых индикаторов за кадр (cap), но приоритет ниже.

#### trap_R_RegisterShader в других местах cg_draw.c

- Строки 1002 (CG_DrawTeamOverlay), 1354 (CG_DrawPowerups), 1687 (лагометр) — вызовы в циклах или при каждом кадре. Иконки паверапов и статичные шейдеры («gfx/2d/net.tga») лучше регистрировать один раз и хранить в cgs.media или в отдельной таблице.

### 8.3 Итог по cg_draw.c

| Участок                    | Частота                    | Тяжесть                                      | Приоритет оптимизации        |
|----------------------------|----------------------------|----------------------------------------------|------------------------------|
| CG_CrosshairTarget         | По событию (ttwheel open, консоль) | 1 Trace + цикл MAX_GENTITIES, trap_CM_* на айтем | Средний; высокий при вызове каждый кадр |
| CG_IsLookingAtItemHitbox   | Внутри цикла CG_CrosshairTarget    | TempBoxModel + TransformedBoxTrace на айтем  | Вместе с CG_CrosshairTarget  |
| CG_ScanForCrosshairEntity  | 1 раз за кадр (кэш)        | 2 CG_Trace + PointContents                   | Сделано                      |
| CG_DrawTeamOverlay         | При включённом overlay     | MAX_LOCATIONS × ConfigString, игроки × RegisterShader | Кэш ConfigString/локаций; кэш шейдеров иконок |
| CG_DrawPowerups            | Каждый кадр (правый нижний)| Цикл по паверапам × RegisterShader           | Кэш шейдеров иконок паверапов |
| CG_DrawPlayerIndicatorOnScreen | Каждый кадр (при BE)   | Цикл scanRange, только кэш + draw             | Низкий (уже кэш)             |
| strlen(cgs.osp.testFont) в CG_Draw2D | Каждый кадр          | O(n) по строке                                | Уже в чеклисте (мелкие улучшения) |
