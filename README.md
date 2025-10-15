# OSP2-BE: Extended OSP2 version

## Check website - [scoqx.github.io](https://scoqx.github.io)

## 🔧 Contributions

OSP2-BE incorporates code from several notable Quake mods and projects:
- **OSP2 by snems**: [Github Link](https://github.com/snems/OSP2)
- **baseq3a by Cyrax**: [GitHub Link](https://github.com/ec-/baseq3a)
- **XQ3E Engine**: [GitHub Link](https://github.com/xq3e/engine)
- **Unlagged Mod by Nail Toronto**
- **Ratmod**: [GitHub Link](https://github.com/rdntcntrl/ratoa_gamecode)



## ✨ BE-Features
## v1.0-beta-5
```
+ cg_chud
+ chud_file
+ cg_clearOnLevelLoad
+ cg_playersID 1/2
+ cg_spectPOV
+ cg_drawRealHeads
+ Introducing CherryHUD: table system to show scoreboard, stats etc.
  * beta, only nonTeam gametypes for a while
  * 2 configs available: default and d2
* cg_teamSound doesnt affect on local player model anymore
* cg_drawAmmoWarning fix
* cg_scoreboardBE fix
* be_enabled fix (fullbright, damage info)
* cg_friendsWallhack fix (not disabling drawHudElements now)
* r_fullbright unlocked again
* fix color by digit (0-9)
* add crouch animation to player model in air
SHUD:
 + update configs (m1r, m1rqlstyle, AGENT)
 * SCORE_OWN/NME fixed for nonTeam gametypes
 * tempAcc osp fix
+ tools / Updater OSP2-BE for Windows and Linux
* CHUD fixes
* hitbox 3 disabled
```
<!-- ## v0.92
```
* altGrenades fix
* scoreboard/wstats fix
```
## v0.91x
```
+ hitsound unlocked
+ friends wallhack unlocked
```
## v0.91
```
cg_friendsWallhack can be disabled on server. Set configstring 888 to 1.

+wstats reworked

description later
+ cg_bestats_style
+ cg_bestats_textSize
+ cg_bestats_font
+ cg_bestats_pos
+ cg_bestats_bgColor
+ cg_bestats_bgOpaque
+ cg_bestats_spacingAdjust
+ cg_bestats_widthCutoff
```
### v0.9e
```
Only for cg_scoreboardBE
+ cg_scoreboardDrawPowerUps
SHUD
+ FPS style 1
+ Localtime style 1
* fix powerup timer: starts from 30, last displayed second is 1 (previously 29 to 0)
```
## v0.9b
```
Only for cg_scoreboardBE
+ cg_scoreboardRtColors
+ cg_scoreboardBtColors
+ cg_scoreboardSpecColor
```
## v0.9
```
+ cg_friendHudMarkerMaxDist
+ cg_friendHudMarkerSize
+ cg_friendHudMarkerMaxScale
+ cg_friendHudMarkerMinScale
+ cg_friendsWallhack
+ cg_drawHudMarkers
+ cg_markTeam
+ cg_markTeamColor
+ cg_mySound
+ cg_teamSound
+ cg_enemySound
```
## v0.089
```
+ ch_crosshairAction bitmask 8 - hitcolor by damage
+ ch_crosshairDecorAction bitmask 8 - hitcolor by damage
 Set colors:
+ ch_crosshairActionColorLow
+ ch_crosshairActionColorMid
+ ch_crosshairActionColorHigh
+ ch_crosshairDecorActionColorLow
+ ch_crosshairDecorActionColorMid
+ ch_crosshairDecorActionColorHigh

+ color names: orange, pink

SHUD:
  + hud config - lwp9k 
```
## v0.088
```
+ cg_drawAccuracy
+ cg_accuracyFontSize
+ cg_accuracyIconSize
+ cg_accuracyFont
```
## v0.087a
```
* FIX Scoreboard with
  * cg_redTeamColor cg_blueTeamColor
```
## v0.087
```
+ cg_redTeamColor
+ cg_blueTeamColor
```

## v0.086a
```
SHUD:
  * Updated m1r, m1rqlstyle, AGENT hud configs
  * Obituaries style 2 - colored nicknames in team modes
  * bgcolor, bordercolor team colors support 'E' and 'T'
  + bgcolor2, bordercolor2 - set alpha when 'E' or 'T' setted (like color2)
  + docs/superhud inside the mod.pk3
```
## v0.085b
```
+ cg_healthColor
+ cg_healthLowColor
+ cg_healthMidColor
* r_fullbright 1 unlocked

* SHUD:
  * Predecorate/Postdecorate - added feature to use { text "string"; } to pring text on the HUD
  + player_name - new element
```
## v0.084e
```
+ cg_drawAmmoWarning 1 - new counting system + ammo low sound
  * 2 - default old
```
## v0.084d
```
+ cg_ignoreServerMessages - Hides the message from server. Bitmask
  * 1 - killer HP/AP (Q3MSK, FFA server)
  * 2 - chat fragged (not only q3msk)
  * 4 - server's chat messages
```
## v0.084c
```
+ cg_gibs 2 (more blood)
* various fixes
```
## v0.084
```
+ cg_itemFx - bitmask
 * 1 - bounce
 * 2 - rotating
 * 4 - scale
+ cg_bubbleTrail
* cg_altBattleSuit shader changed
* enemy cg_altLightning 3 changed
* changed some default values for cvars
- cg_itemsRespawnAnimation
```
## v0.083
```
+ cg_altGrenades 2
+ cg_altGrenadesColor (cg_altGrenades 2 only)
+ cg_enemyGrenadesColor (cg_altGrenades 2 only)
+ cg_altBattleSuit
```
## v0.081
```
+ cg_scoreboardBE 3 - old style team scores
+ outline optimisation
* hitbox shaders renamed
```
## v0.08
```
+ Fullbright player models support (use /fb. e.g. keel/fb)
+ cg_drawBrightWeapons
* rail chamber default color is white
* various fixes
```
## v0.07
```
+ Description for commands. Requires Q3E version dated April 14, 2025 or later
+ cg_railCustomChamber
  * 1 - reloading color = core color
  * 2 - standing color = rings color
* Rail chamber color in other player hands/in world have green color by default (QL-like), if cg_railCustomChamber = 2 - using this color 
* ScoreboardBE parse info fix
* updated diwoc's hud

* SHUD:
  + element 'localdate'
  + style 3 for StatusBar_HealthBar/StatusBar_ArmorBar 
  * StatusBar_AmmoCount/WeaponList -  shows ammo in red when it's 0
```
## v0.06d7
```
* cg_teamIndicator bitmask:
  * 8 show data for frozen player
  * 16 enable icons
* Replace OSP2 net.png image
* various fixes
* huds updated
```
## v0.06d
```
* cg_teamIndicator: 
  + 8 bitmask (show icons (only for dead now))
  + show all names while spectating
+ cg_teamIndicatorFont
+ cg_scoreboardBE - alt default scoreboard
+ cg_scoreboardFont - font for alt scoreboard
+ cg_centerMesagesFont
* cg_drawCrosshairNames:
  + 2 option - teammates
  + 3 option - enemies

* SHUD: StatusBar_ArmorCount fix
```
## v0.06c2
```
 + cg_teamIndicator;
 + cg_teamIndicatorAdjust;
 + cg_teamIndicatorColor;
 + cg_teamIndicatorOpaque;
 + cg_teamIndicatorBgColor;
 + cg_teamIndicatorBgOpaque;
 + cg_teamIndicatorOffset;
 + cg_teamIndicatorMaxLength;
 * cg_drawFriend
    + option 2 (draw only freeze foe)
```
## v0.06a
```
* SHUD:
  * Player/Weapon stats fix
  * StatusBar_ArmorCount 
    + visflags: showempty
    + color works for '0'
``` -->
## Extended features
```
Cvars:
+ cg_altBlood multiply shaders support
+ cg_altBloodColor
* cg_altPlasma 
  + option 2
+ cg_altShadow multiply shaders support
+ cg_altShadowColor
+ cg_damageDrawFrame
+ cg_damageFrameSize
+ cg_damageFrameOpaque
* cg_drawGun (reworked)
+ cg_drawGunForceAspect
* cg_drawHitBox enabled. Server permission required
* cg_drawRewards bitmask
  * 2 - disable icon, 4 - disable float sprite, 8 - disable sound
+ cg_gunPos
+ cg_hitBoxColor <color>
+ cg_enemyLightningColor
+ cg_uniqueColorTable
+ cg_noSlidingWindow
+ cg_underwaterFovWarp
+ cg_drawOutline
+ cg_enemyOutlineColor
+ cg_teamOutlineColor
+ cg_enemyOutlineColorUnique
+ cg_enemyOutlineSize
+ cg_teamOutlineSize
+ cg_railRingsRadius
+ cg_railRingsRotation
+ cg_railRingsSpacing
+ cg_shotGunTracer
+ cg_railStaticRings
+ cg_railRingsSize 
+ cg_scoreboardShowId
* cg_shadows
  + option -1

SHUD:
Elements:
+ tempAcc_current - temp acc for lg
+ weaponStats_## - accuracy
+ weaponStats_##_icon
+ playerStats_DG
+ playerStats_DR
+ playerStats_DG_icon
+ playerStats_DR_icon
+ playerStats_damage_ratio
+ grid
+ tempAcc_icon
Other:
+ ShadowColor for text
+ Border (border, `borderColor)
* Background for text elements (reworked)
+ Background for client events in obituaries
+ Temporal LG accuracy
+ Item pickup style 2
+ Color E/T transparency via color2
+ Armor bar over HP bar
+ tempAcc styles options
+ CurrentWeaponStats styles options
+ scoresScore_NME	style
+ Configs (m1rqlstyle, AGENT, diwoc_light, cybra, zoti, RTG, California)
+ Current Weapon Stats shows accuracy
  + CurrentWeaponStats
  + CurrentWeaponStats_icon
* weaponlist:
  + border for choosen weapon
* visflags: showempty for weapon/playerStats, enemyscore, tempAcc
+ StatusBar_AmmoCount shading when realoading
  * dark grey by default, or color2 is set
  * disable by style (any)


* +wstats additional data
  + Kill/death ratio (K/D)
  + Damage given/recieved Ratio
* crosshairNames
  + teammates names in fog
  + ignore fog on maps: Asylum, Quarantine, oxodm32, oxodm90, rjldm3, gen_q1dm1
* restricted names in scoreboard
* restricted strings have "." instead of "..." at the end
```
![shot-Mar-27-2025_00 30 21](https://github.com/user-attachments/assets/11e8a2cf-8ef6-4984-a427-5af8ac9de650)


## 📞 Contacts

Join the OSP2-BE community on Telegram for updates, discussions, and support:
- [Telegram Channel](https://t.me/q3osp2)
- [Telegram Chat](https://t.me/q3_osp2)

## 🛠 New Commands

See [docs/commands.md](docs/commands.md) for a comprehensive list of new commands and how to use them.

Example configuration file: `OSP2.cfg` located inside the mod directory.

## 📥 Installation

To install OSP2-BE:

1. **Install Quake 3** and the **OSP mod v1.03**.
2. Copy the mod files (pack or library) into the `osp` folder within your Quake 3 directory.


