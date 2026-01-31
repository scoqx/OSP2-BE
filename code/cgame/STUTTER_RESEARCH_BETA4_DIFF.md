# Исследование подфриза: diff code_beta4 vs code (текущий)

Сравнение `code_beta4/cgame` (beta-4) и `code/cgame` (текущий). Цель — выявить изменения, которые могут давать подфриз на мышке (в т.ч. в 1v1). **Team task (ttwheel, border markers) по замечанию пользователя исключён из основных подозреваемых.**

---

## 1. Объём изменений (кратко)

- **78 файлов** изменено, ~23.5k строк добавлено, ~15.7k удалено.
- В beta-4 **нет**: teamtasks, `cg_printui.c`, `ANALYSIS_PERFORMANCE.md`, вызова `CG_UpdateIndicatorVisibilityCache`. Есть `cg_bescoreboard.c` и полный CherryHUD.
- В **текущем** добавлены: batch visibility cache для индикатора/HUD-маркеров, больше веток в `CG_ServerCommand`, доработки в `cg_players.c` (clientinfo, модели, маркеры).

---

## 2. Кандидаты на подфриз (по приоритету, без teamTask)

### 2.1 Server commands — пачка за кадр (приоритет 1)

**Где:** `cg_servercmds.c` — `CG_ExecuteNewServerCommands()` вызывается **дважды за переход снапшота**: в `CG_ReadNextSnapshot()` и в `CG_TransitionSnapshot()` (`cg_snapshot.c`). Обрабатываются **все** накопившиеся команды до `latestSequence` в одном кадре.

**Тяжёлые ветки в `CG_ServerCommand()`:**

- **`"cs"`** → `CG_ConfigStringModified()`: вызывает **`trap_GetGameState(&cgs.gameState)`** — копирование всего геймстейта из движка в QVM; затем разбор по типу строки (CS_MUSIC, CS_SERVERINFO, CS_SCORES1/2, и т.д.). При нескольких `"cs"` подряд — несколько вызовов `trap_GetGameState` в одном кадре.
- **`"scores"`** → `CG_ParseScores()`: цикл по **MAX_CLIENTS** (64), разбор аргументов, обновление массивов.
- **`"tinfo"`** → `CG_ParseTeamInfo()`: разбор team info.
- **`"statsinfo"`** → `CG_BEParseStatsInfo()`: цикл по аргументам, `trap_Argv`, `atoi`, сравнения.
- **`"chat"` / `"tchat"`**: копирование строк, фильтры, `CG_ChatCheckMessageAllowed`, `CG_ReplacePlayerNameInChat`, `CG_InsertChatColor`, `CG_SHUDEventChat` и т.д.

**Почему может давать подфриз:** при нестабильном пинге или всплеске (много чата, обновление скоров, конфигстрингов) в одном кадре выполняется много команд → один тяжёлый кадр → подфриз на мышке. В 1v1 тоже возможны всплески (чат, редкие `cs`/`scores`).

**Проверка:** `cg_debugServerCommandsPerFrame 3` (или 5); если подфризы станут реже/слабее — вклад server commands подтверждён.

---

### 2.2 CG_UpdateIndicatorVisibilityCache + цикл индикатора — каждый кадр (приоритет 2)

**Где:**  
- `cg_view.c`: вызов **`CG_UpdateIndicatorVisibilityCache()`** перед `CG_AddPacketEntities`. В beta-4 этого вызова **нет**.  
- `cg_players.c`: сама функция и цикл в **`CG_DrawPlayerIndicatorOnScreen()`** (`cg_draw.c`).

**Что делает batch cache:**  
Раз в кадр обрабатывает **2 слота** (константа `CG_INDICATOR_BATCH_SIZE`): для каждого слота вычисляется индекс `i = (indicatorStartIndex + n) % scanRange` (scanRange = MAX_CLIENTS или в freeze MAX_GENTITIES), затем вызывается **`CG_IsPlayerValidAndVisible(i, ...)`** → при необходимости **`CG_FriendVisible(cent)`** → **2× CG_Trace** на игрока. Итого до **4 трасс за кадр** при включённом индикаторе или HUD-маркерах.

**Сравнение с beta-4 и поведение в 1v1:**  
В beta-4 в **`CG_FriendHudMarker`** для каждого рисуемого союзника вызывался **`CG_FriendVisible(cent)`** (до 2 трасс на игрока). В **1v1 (дуэль/FFA)** в текущем коде в **`CG_UpdateIndicatorVisibilityCache`** для противника вызывается **`CG_IsEnemy(ci)`** → в не-командных режимах противник считается врагом → **`continue`**, до **`CG_IsPlayerValidAndVisible`** дело не доходит. Итого в 1v1: **0 трасс за кадр** в batch (оба слота — либо сам игрок, либо враг, оба скипаются). В командных режимах: до 4 трасс за кадр. Вывод: в 1v1 batch не даёт трасс, но **цикл 64 итерации в CG_DrawPlayerIndicatorOnScreen** и сам вызов **CG_UpdateIndicatorVisibilityCache** (2 итерации, проверки clientInfo/CG_IsEnemy) выполняются каждый кадр при включённых индикаторе или HUD-маркерах.

**Цикл отрисовки индикатора:**  
`CG_DrawPlayerIndicatorOnScreen()` каждый кадр выполняет цикл **`for (i = 0; i < scanRange; i++)`** (scanRange = 64 или в freeze 1024). В теле — только чтение кэша и при `visible` вызов `CG_DrawPlayerIndicator(i)`. Трасс в цикле нет, но **64 или 1024 итерации** каждый кадр при включённом индикаторе/маркерах; в режиме freeze — до 1024 проходов.

**Проверка:** временно закомментировать вызов `CG_UpdateIndicatorVisibilityCache()` в `cg_view.c` и выключить индикатор и `cg_drawHudMarkers`; сравнить подфриз. Отдельно можно уменьшить `CG_INDICATOR_BATCH_SIZE` до 1 и посмотреть, изменится ли нагрузка.

---

### 2.3 CG_FriendHudMarker — чтение кэша, без трасс в текущем (приоритет 3)

В текущем коде **`CG_FriendHudMarker`** не вызывает `CG_FriendVisible`: использует **batch cache** (`indicatorCacheVisible[entityIndex]`). То есть трассы за маркеры уже учтены в п. 2.2. Дополнительный вес — только цикл по игрокам в `CG_AddPacketEntities` и отрисовка маркера для видимых; сам по себе путь легче, чем в beta-4 (где были трассы в маркере). Имеет смысл смотреть в связке с п. 2.2 (включён ли индикатор/маркеры, крутится ли batch).

---

### 2.4 CG_ConfigStringModified и trap_GetGameState (часть server commands)

**Где:** `cg_servercmds.c` — при каждой команде **`"cs"`** вызывается `CG_ConfigStringModified()` → в начале **`trap_GetGameState(&cgs.gameState)`**.

**Почему важно:** один вызов копирует весь геймстейт (конфигстринги и т.д.) из движка в QVM. Если за один кадр приходит несколько `"cs"` (например, при смене карты или обновлении игроков), в одном кадре выполняется несколько таких копий — сильный кандидат на пик времени кадра.

Уже учтён в п. 2.1; при точечном разборе server commands имеет смысл отдельно смотреть частоту `"cs"` и возможность ограничить обработку или размазать по кадрам.

---

### 2.5 CG_ScanForCrosshairEntity — в текущем легче, не источник

В текущем коде добавлен кэш по кадру (`cg.crosshairScanTime == cg.time` → ранний return). В beta-4 кэша не было. Этот путь **облегчён**, не рассматривается как источник подфриза.

---

### 2.6 cg_players.c — прочие циклы (не каждый кадр или с кэшем)

- **`CG_IsPlayerAvailable`**, **`CG_GetAimContactPoint`**: циклы по MAX_GENTITIES, но с кэшем по `cg.time` или вызов по событию (например, team task сообщения), не «на каждый кадр для всех».
- **`CG_NewClientInfo`** и сброс игрока: цикл по MAX_GENTITIES при смене модели — по событию, не каждый кадр.

Имеет смысл возвращаться к ним, только если после проверки п. 2.1 и 2.2 подфриз останется.

---

### 2.7 Team task (исключён из основных подозреваемых)

По замечанию пользователя: **«Точно не teamTask»**. Соответственно, CG_CrosshairTarget при открытии ttwheel, CG_DrawTeamTaskBorderHudMarkers, CG_DrawSelwheelPointers и связанная логика не рассматриваются как основные кандидаты на подфриз в этом документе. При необходимости их можно проверить отдельно.

---

## 3. Теории (кратко)

| # | Теория | Когда проявляется | Как проверить |
|---|--------|-------------------|----------------|
| 1 | Всплеск server commands в одном кадре (trap_GetGameState при "cs", циклы при "scores"/"tinfo"/"statsinfo", чат) | При лагах, чате, обновлении скоров/конфигов | `cg_debugServerCommandsPerFrame 3` |
| 2 | Постоянный оверхед в 1v1: 0 трасс в batch (враг скипается), но вызов CG_UpdateIndicatorVisibilityCache (2 итерации + CG_IsEnemy) и цикл 64 в CG_DrawPlayerIndicatorOnScreen каждый кадр | Всегда при включённых индикаторе или HUD-маркерах, в т.ч. 1v1 | Убрать вызов CG_UpdateIndicatorVisibilityCache(), выключить индикатор и cg_drawHudMarkers |
| 3 | Комбинация: пачка команд + постоянные 4 трассы и цикл индикатора | Постоянно или при всплесках | Последовательно проверять п. 1 и п. 2 |

---

## 4. Рекомендуемый порядок проверки

1. **Server commands:** выставить `cg_debugServerCommandsPerFrame 3` (или 5); сравнить частоту/силу подфризов.
2. **Indicator / HUD markers:** отключить индикатор и `cg_drawHudMarkers`; временно закомментировать вызов `CG_UpdateIndicatorVisibilityCache()` в `cg_view.c` и проверить.
3. При необходимости — точечный разбор тяжёлых веток в `cg_servercmds.c` (частота "cs", "scores", "tinfo") и циклов в `cg_players.c` / `cg_draw.c` (индикатор, маркеры).

---

## 5. Файлы для углублённого разбора

- **`cg_servercmds.c`** — ветки "cs", "scores", "tinfo", "statsinfo", "chat"/"tchat"; вызовы `trap_GetGameState`, циклы по MAX_CLIENTS.
- **`cg_view.c`** — вызов `CG_UpdateIndicatorVisibilityCache()` и порядок относительно `CG_AddPacketEntities`.
- **`cg_players.c`** — `CG_UpdateIndicatorVisibilityCache`, `CG_IsPlayerValidAndVisible`, `CG_FriendVisible`, кэши по кадру.
- **`cg_draw.c`** — `CG_DrawPlayerIndicatorOnScreen` (цикл scanRange 64/1024), цепочка Draw.

---

## 6. Анализ cg_players.c — возможные источники подфриза

### 6.1 CG_NewClientInfo — тяжёлый путь при каждом обновлении игрока

**Вызовы:**
- Из **CG_ConfigStringModified** при каждой команде **"cs"** для игрока (num в [CS_PLAYERS, CS_PLAYERS+MAX_CLIENTS)) — в одном кадре может прийти несколько таких команд.
- Из **CG_ProcessPendingClientInfoUpdates** — **2 раза за кадр** в течение 32 кадров после любого вызова **CG_UpdateAllClientsInfo** (очередь по 2 клиента за кадр).

**Что делает один вызов CG_NewClientInfo:**
- **CG_PlayerNameBuildFinal** → внутри:
  - **CG_IsReplacementNameUsed** — цикл по **MAX_CLIENTS** (64), сравнение имён.
  - **CG_PlayerNameCheckDuplicate** — цикл по **MAX_CLIENTS** (64), сравнение имён.
  - При дубликате **CG_PlayerNameFindAndUpdateExisting** — цикл по **MAX_CLIENTS** (64) и вызов **CG_PlayerNameBuild** для каждого совпадения.
- **CG_LoadClientInfo** — множество **trap_R_RegisterModel**, **trap_R_RegisterSkin**, **trap_FS_***, **trap_Cvar_VariableStringBuffer**.
- В конце **CG_CountRealClients** — ещё один цикл по **MAX_CLIENTS** (64).

Итого на один CG_NewClientInfo: до **3×64** итераций только по логике имён + загрузка моделей/скинов. При **2× CG_NewClientInfo за кадр** (отложенное обновление) — до **384** итераций по именам и 2 полных пересборки clientinfo за кадр.

**Когда срабатывает в 1v1:**
- **CG_UpdateAllClientsInfo** вызывается: при смене карты/инициализации (CG_InitPlayerColors при T/E в цветовых cvars), при смене cvars (localevents: model, colors), при смене наблюдаемого игрока в спеке (cg_spectOrigModel), в демо при смене clientNum. В обычном 1v1 без спека — в основном при смене карты или при изменении cvars. После каждого такого вызова идут **32 кадра подряд** с 2× CG_NewClientInfo за кадр → возможный подфриз на ~0.1–0.25 сек (зависит от FPS).
- При пачке команд **"cs"** в одном кадре (обновление конфигстрингов игроков) — несколько **CG_NewClientInfo** в одном кадре без размазки → один тяжёлый кадр.

**Проверка:** временно увеличить `CG_CLIENTINFO_BATCH_SIZE` до 1 (в `cg_players.c`) и сравнить длительность/силу подфриза после смены карты или cvar. Либо временно отключить вызов **CG_ProcessPendingClientInfoUpdates** в `cg_view.c` и проверить, не исчезнет ли подфриз сразу после захода на карту.

---

### 6.2 CG_ProcessPendingClientInfoUpdates — каждый кадр

Вызывается **каждый кадр** из **CG_DrawActiveFrame** (`cg_view.c`). Пока `updateAllPendingStartIndex >= 0`, обрабатывает 2 клиента за кадр (см. п. 6.1). В 1v1 после любого триггера **CG_UpdateAllClientsInfo** это даёт 32 кадра нагрузки подряд; без частых триггеров — не постоянный источник подфриза.

---

### 6.3 CG_UpdateIndicatorVisibilityCache (уже в п. 2.2)

В 1v1: 2 итерации batch, 0 трасс (противник — враг, скип по **CG_IsEnemy**). Оверхед — 2 проверки clientInfo и 2 вызова **CG_IsEnemy** (с кэшем по clientNum и cg.time). Постоянный, но небольшой.

---

### 6.4 CG_IsPlayerAvailable / CG_GetAimContactPoint

Используются только в **CG_ClientInfoUpdateTeamTask** (team task: WARN_ALIVE и др.). В 1v1 в начале **CG_ClientInfoUpdateTeamTask** стоит `if (cgs.gametype < GT_TEAM) return;` — путь не выполняется. **Не источник подфриза в 1v1.**

---

### 6.5 CG_CountRealClients

Цикл по **MAX_CLIENTS** (64). Вызывается: в конце **CG_NewClientInfo** (при каждом обновлении игрока), при уходе игрока (configstring), при отрисовке скорборда (раз в 2 сек при открытом скорборде). При активной очереди отложенного обновления — 2 раза за кадр (в каждом из 2× CG_NewClientInfo). Отдельно не размазывает нагрузку, но вносит вклад в стоимость **CG_NewClientInfo**.

---

### 6.6 Итог по cg_players.c

| Кандидат | В 1v1 | Когда проявляется | Проверка |
|----------|-------|-------------------|----------|
| CG_NewClientInfo (пачка вызовов) | Да | Пачка "cs" в одном кадре или 32 кадра после CG_UpdateAllClientsInfo | Ограничить batch до 1; отключить ProcessPending на время теста |
| CG_ProcessPendingClientInfoUpdates | Да | 32 кадра после смены карты/cvar/спека | Закомментировать вызов в cg_view.c |
| CG_UpdateIndicatorVisibilityCache | Да, малый оверхед | Каждый кадр при индикаторе/HUD-маркерах | См. п. 2.2 |
| CG_CountRealClients | Да, как часть NewClientInfo | Внутри CG_NewClientInfo и при скорборде | — |

**Главный подозреваемый в cg_players.c для 1v1:** отложенное обновление clientinfo (**CG_UpdateAllClientsInfo** → 32 кадра по 2× **CG_NewClientInfo**). Если подфриз появляется в первые секунды после захода на карту или после смены cvar — в первую очередь проверять этот путь.

---

## 7. Как посмотреть diff локально (PowerShell)

В PowerShell использовать `;` вместо `&&`. Примеры:

```powershell
cd c:\git\OSP2-PBE
git diff --no-index code_beta4/cgame/cg_servercmds.c code/cgame/cg_servercmds.c
git diff --no-index code_beta4/cgame/cg_snapshot.c code/cgame/cg_snapshot.c
git diff --no-index code_beta4/cgame/cg_view.c code/cgame/cg_view.c
```

Объём изменений (по `--stat`): cg_servercmds.c ~+785/-27, cg_snapshot.c +5/-1, cg_view.c +211/-2.
