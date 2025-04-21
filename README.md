# OSP2-BE: Extended OSP2 version

## 🔧 Contributions

OSP2-BE incorporates code from several notable Quake mods and projects:
- **OSP2 by snems**: [Github Link](https://github.com/snems/OSP2)
- **baseq3a by Cyrax**: [GitHub Link](https://github.com/ec-/baseq3a)
- **XQ3E Engine**: [GitHub Link](https://github.com/xq3e/engine)
- **Unlagged Mod by Nail Toronto**
- **Ratmod**: [GitHub Link](https://github.com/rdntcntrl/ratoa_gamecode)

## ✨ BE-Features

## v0.056t
```
  SHUD:
  + weaponStats_## - accuracy
  + weaponStats_##_icon
```
## v0.055t
```
  * +wstats additional data
    + Kill/death ratio (K/D)
    + Damage given/recieved Ratio
```
## 0.054t1
```
  + cg_gunPos
  + cg_shotGunTracer
  + cg_railStaticRings
  + cg_railRingsSize 

  * cg_railTrailRadius -> cg_railRingsRadius
  * cg_railTrailRotation -> cg_railRingsRotation
  * cg_railTrailSpacing -> cg_railRingsSpacing
```
## 0.053
```
  + cg_noSlidingWindow
```
## 0.051/0.052
```
  * cg_drawOutline team colors fixed
  * cg_drawRewards 2

  SHUD:
  + tempAcc styles variations
  + CurrentWeaponStats styles variations
  + scoresScore_NME	style
  + Configs (m1rqlstyle, AGENT, diwoc_light, cybra, zoti)
```
## Extended features
```
Cvars:
+ cg_altBlood
+ cg_altBloodColor
* cg_altPlasma 
  + variation 2
+ cg_damageDrawFrame
+ cg_damageFrameSize
+ cg_damageFrameOpaque
* cg_drawGun (reworked)
+ cg_drawGunForceAspect
* cg_drawHitBox enabled. Server permission required
+ cg_hitBoxColor <color>
+ cg_itemsRespawnAnimation
+ cg_enemyLightningColor
+ cg_uniqueColorTable
+ cg_noVoteBeep
+ cg_underwaterFovWarp
+ cg_drawOutline
+ cg_enemyOutlineColor
+ cg_teamOutlineColor
+ cg_enemyOutlineColorUnique
+ cg_enemyOutlineSize
+ cg_teamOutlineSize

SHUD:
+ Border (border, borderColor)
* Background for text elements (reworked)
+ Background for client events in obituaries
+ Temporal LG accuracy
+ Item pickup style 2
+ Color E/T transparency via color2
+ Armor bar over HP bar

+ Current Weapon Stats shows accuracy
  + CurrentWeaponStats
  + CurrentWeaponStats_icon

* weaponlist:
  + border for choosen weapon
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
