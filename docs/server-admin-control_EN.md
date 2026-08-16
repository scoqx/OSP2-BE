# OSP2-BE server control and extended features (Quake 3)

## Disabling BE features

### Main switch: be_enabled

All BE features can be fully disabled by setting `be_enabled 0` on the client. This turns off every OSP2-BE extended capability.

```c
// Fully disable all BE features
seta be_enabled "0"
```

### Server-side feature control (CS 888)

The server can selectively disable specific BE features via configstring **888** (`CS_OSP2BE_DISABLED_FEATURES`). The value is the sum of bitmasks for the features to disable.

| Bitmask | Feature         | Description                                                   |
|---------|-----------------|---------------------------------------------------------------|
| 1       | Team wh         | Teammate wallhack, global pos (bit 4 of `cg_friendsWallhack`) |
| 2       | Model sound     | Model sound selection                                         |
| 4       | Alt grenades    | Alternative grenades                                          |
| 8       | Enemy lightning | Enemy lightning color                                         |
| 16      | Mark teammate   | Teammate marking                                              |
| 32      | Alt shadow      | Alternative shadow                                            |
| 64      | Alt blood       | Alternative blood                                             |
| 128     | Alt plasmagun   | Alternative plasmagun                                         |
| 256     | Outline         | Player outlines                                               |
| 512     | Team indicator  | Team indicator                                                |
| 1024    | Damage info     | Damage dealt info                                             |
| 2048    | Fullbright      | Fullbright models/weapons                                     |
| 4096    | Team tasks      | Team tasks                                                    |
| 8192    | No explosions   | Disable explosions                                            |
| 16384   | Team frozen     | Wallhack for frozen teammates                                 |
| 32768   | HUD markers     | HUD markers (players)                                         |
| 65536   | Replace steps   | Remove footsteps on metal surfaces                            |
| 131072  | Autojump        | Automatic jump                                                |
| 262144  | Hitbox          | Forbid hitbox display                                         |

```c
// Disable damage info (1024) and fullbright (2048)
set cs 888 "3072"

// Disable all features
set cs 888 "524287"
```

Players can check availability with `/allowedfeatures`.

---

## Features that require server support

### cg_damagePlums (floating damage numbers)

- **Server cvar:** `g_damagePlums`
- **Client cvar:** `cg_damagePlums`
- **Weapon filter:** `cg_damagePlumsWeapons` (bitmask: 1 gauntlet, 2 MG, 4 SG, 8 GL, 16 RL, 32 LG, 64 RG, 128 PG, 256 BFG)

### cg_autohop (automatic jump)

- **Server cvar:** `pmove_autohop`

```c
set pmove_autohop "0"   // disabled
set pmove_autohop "1"   // enabled, players cannot turn it off
set pmove_autohop "2"   // enabled, players can control it
```

### CS 889 — tourney restrictions

**Configstring:** `CS_OSP2BE_TOURNEY_RESTRICTIONS` (889). Integer bitmask.

| Bit | Value    | Description                                               |
|-----|----------|-----------------------------------------------------------|
| 1   | FPS lock | Force `com_maxfps` to 125 or 250 for the match duration   |

```c
set cs 889 "1"
```

### CS 890–909 — item / spawn / teleporter catalog

**Configstrings:** `CS_OSP2BE_ITEM_CATALOG0` … `CS_OSP2BE_ITEM_CATALOG_LAST` (890–909). The client concatenates all 20 strings into one payload (up to ~20 KiB). BE-only — a normal `/configstrings` **does not show** them (same as 888/889); check the server log `Item catalog: mode=…` or `/testq3lb`.

A single catalog serves:

| Client / HUD element                              | Mode  | Purpose                                           |
|---------------------------------------------------|-------|---------------------------------------------------|
| `cg_shadowItems`                                  | bit 1 | Ghost 3D pickups at respawn locations             |
| SuperHUD `Spectator_ItemSpawns*`, `ItemPickups*`  | bit 2 | Timers and pickup notifications (spectators)      |
| `cg_drawSpawns` / world flags                     | bit 4 | Respawn points (if no local QSP1)                 |
| `cg_drawTPexit`                                   | bit 8 | Teleporter exits (if no local QTX1)               |

If the server does not set the catalog (or the mode lacks the needed bit), the client falls back to **QITM** from mapdata for `cg_shadowItems`, and to QSP1/QTX1 for spawns/teles.

**Spawn segment:** `sp;<x>,<y>,<z>,<yaw>,<sf>,<count>,<role>,<team>`  
**TP exit segment:** `tde;<x>,<y>,<z>,<yaw>`  
Coordinates are origin × 10. Role/team match QSP1 (0..6 / 0..3).

#### How to build a catalog for PUI

**mode** for PUI-only is **`2`**. If shadow items are also needed — **`3`**. Spawns/teles add bits **4** / **8** (everything at once — mode **`f`** = 15).

##### Which items get a trackId

Only these abbrevs (others may be added for shadow, but trackId and `pui` ignore them):

| abbrev | Item           | abbrev | Item         |
|--------|----------------|--------|--------------|
| `mh`   | Mega Health    | `qd`   | Quad Damage  |
| `ra`   | Red Armor      | `bs`   | Battle Suit  |
| `ya`   | Yellow Armor   | `reg`  | Regeneration |
|        |                | `inv`  | Invisibility |
|        |                | `hst`  | Haste        |
|        |                | `flg`  | Flight       |

##### Build algorithm

1. Walk pickups **left to right** in the order they should appear in SuperHUD.
2. For each **physical point** on the map — one **segment**:
   - single item: `mh;1280,960,320`
   - team pad (RA+YA): `ra/ya;640,1280,64,1` — abbrevs joined with `/`, **one** coordinate triple
3. Coordinates are **origin × 10** (deci-units). Example: `(128, 96, 32)` → `1280,960,320`.
4. In **CTF**, append **`,side`** to each PUI segment: `0` neutral, `1` red, `2` blue.
5. Join segments with **`|`**; put **mode** at the start of the payload.
6. Split the string across CS 890–909 as convenient — the client concatenates them back.

##### Example: map with mega, red RA/YA pad, neutral quad

HUD order: mega → RA → YA → quad.

```
2mh;1280,960,320|ra/ya;640,1280,64,1|qd;5120,3840,160
```

| trackId | segment               | abbrev | side |
|---------|-----------------------|--------|------|
| 0       | `mh;1280,960,320`     | mega   | —    |
| 1       | `ra/ya;640,1280,64,1` | RA     | red  |
| 2       | `ra/ya;640,1280,64,1` | YA     | red  |
| 3       | `qd;5120,3840,160`    | quad   | —    |

```c
set cs 890 "2mh;1280,960,320|ra/ya;640,1280,64,1|qd;5120,3840,160"
```

These trackIds (`0…3`) are the same numbers used as **`pickId`** / **`nextId`** in the `pui` command (see below).

##### Common mistakes

| Mistake | Result |
|---------|--------|
| `mode=1` | PUI does not work — client ignores the catalog for spectator HUD |
| `rl;…` in catalog without a PUI abbrev | Visible in shadow, no trackId, `pui` is not sent for it |
| RA and YA as separate segments | Two pads instead of one — timers and `nextId` will not behave as a team pad |
| Missing `,side` in CTF | Client drops the segment |
| Coordinates without ×10 | Ghosts/timers are offset |

#### Payload format (reference)

```
<mode><segment>|<segment>|...
```

- **mode** — one hex digit `0-9a-f` (bitmask & 15):
  - `1` — shadow catalog
  - `2` — PUI (spectator HUD)
  - `4` — spawns (`sp;…`)
  - `8` — tp exits (`tde;…`)
  - can be combined (e.g. `3` = shadow+PUI, `f` = all = 15)
- **segment** — item/PUI slot:

```
<abbrev>[/<abbrev>...];<x>,<y>,<z>[,<side>]
```

or spawn / tp exit:

```
sp;<x>,<y>,<z>,<yaw>,<sf>,<count>,<role>,<team>
tde;<x>,<y>,<z>,<yaw>
```

| Field    | Description                                                                                         |
|----------|-----------------------------------------------------------------------------------------------------|
| `abbrev` | Short item name (`mh`, `ra`, `ya`, `qd`, …). Full list — table in `cg_shadowitems.c`.               |
| `/`      | Alternatives for one slot (team pad: RA/YA, etc.).                                                  |
| `x,y,z`  | Coordinates in **deci-units** (origin × 10).                                                        |
| `side`   | `0` neutral, `1` red, `2` blue. In CTF the server must send this for PUI segments; outside CTF it may be omitted (all `0`). Shadow-only segments do not use side. |

Full abbrev list for the shadow catalog — `cg_shadowitems.c`. Splitting across CS 890–909 is arbitrary.

#### `pui` command on pickup

When the catalog has PUI mode enabled (bit 2), on pickup of a tracked item the server sends **only to spectators** (`TEAM_SPECTATOR`), not broadcast `-1`.

```
pui <cmd> <readyTime>
```

##### Step 1 — trackId from catalog

Numbering is **left to right** over all PUI abbrevs in the payload (`mh`, `ra`, `ya`, `qd`, `bs`, `reg`, `inv`, `hst`, `flg`). Weapons and other abbrevs are excluded from trackId.

For the catalog example above (`2mh;…|ra/ya;…|qd;…`):

| trackId | Segment           | Item |
|---------|-------------------|------|
| 0       | `mh;1280,960,320` | mega |
| 1       | `ra/ya;…,1`       | RA   |
| 2       | `ra/ya;…,1`       | YA   |
| 3       | `qd;0,0,512`      | quad |

On a team pad `ra/ya` there are two consecutive trackIds: picked RA → `pickId=1`, `nextId=2`; picked YA → `pickId=2`, `nextId=64` (no next item on the pad).

##### Step 2 — pack cmd

```
cmd = picker * 1048576 + pickId * 1024 + nextId
```

| Field       | Source                                                                              |
|-------------|-------------------------------------------------------------------------------------|
| `picker`    | `clientNum` of the player who picked up (`0…63`)                                    |
| `pickId`    | trackId of the picked item                                                          |
| `nextId`    | trackId of the next item **on the same pad**, or **64** if none                     |
| `readyTime` | `level.time + respawn_ms` — respawn moment in serverTime; **0** clears the timer    |

##### Step 3 — examples

Player **client 2** picks **RA** (track 1), YA on pad respawns in 25 s, `level.time = 120000`, respawn 25000:

```
picker  = 2
pickId  = 1
nextId  = 2
cmd     = 2*1048576 + 1*1024 + 2 = 2098178
readyTime = 120000 + 25000 = 145000

pui 2098178 145000
```

Same player picks **mega** (track 0), single item on the slot, respawn 35 s:

```
picker  = 2
pickId  = 0
nextId  = 64
cmd     = 2*1048576 + 0*1024 + 64 = 2097216
readyTime = level.time + 35000

pui 2097216 155000
```

##### Step 4 — who to send to

Iterate clients in `TEAM_SPECTATOR` and for each:

```c
trap_SendServerCommand(spectatorClientNum, "pui %d %d", cmd, readyTime);
```

Do **not** send to players in red/blue/follow — the client handles `pui` only in spectator mode.

> **Metarena:** the mod builds the catalog itself (CS 890–899) and sends `pui` on pickup. Manual assembly is only needed for your own server mod code.

Ghost categories for shadow items are controlled by the client **`cg_shadowItems`** (bitmask: health, armor, weapon, ammo, powerup, flag).

---

## Features with optional server logic

### cg_respawnWeapon (weapon on respawn)

The client chooses a weapon via `cg_respawnWeapon` (`0` — default, `1`…`8` — gauntlet…plasmagun). For correct behavior the server should honor the client setting when granting weapons on respawn; otherwise the player first gets the server default weapon, then switches to the chosen one.

```c
seta cg_respawnWeapon "7"   // railgun
seta cg_respawnWeapon "0"   // server weapon
```
