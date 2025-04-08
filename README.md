# OSP2-BE: Extended OSP2 version

## 🔧 Contributions

OSP2-BE incorporates code from several notable Quake mods and projects:
- **OSP2 by snems**: [Github Link](https://github.com/snems/OSP2)
- **baseq3a by Cyrax**: [GitHub Link](https://github.com/ec-/baseq3a)
- **XQ3E Engine**: [GitHub Link](https://github.com/xq3e/engine)
- **Unlagged Mod by Nail Toronto**
- **Ratmod**: [GitHub Link](https://github.com/rdntcntrl/ratoa_gamecode)

## ✨ BE-Features
<details>
  <summary>Player outline</summary>
  ![shot-Apr-09-2025_02 46 24](https://github.com/user-attachments/assets/ff247a28-303d-40b0-b194-25048af65231)
  ![shot-Apr-09-2025_02 47 01](https://github.com/user-attachments/assets/fe9a03e3-7f00-4fbb-9e96-014e3bd6d33d)
</details>

<details>
  <summary>DamageFrame</summary>
![shot-Apr-05-2025_17 53 25](https://github.com/user-attachments/assets/70d91ba9-b1ca-46b8-b8e1-aeb0a01b57bc)
![shot-Apr-09-2025_02 52 34](https://github.com/user-attachments/assets/12696685-b7da-427c-8c92-198cf831f6b7)
![shot-Apr-05-2025_17 44 15](https://github.com/user-attachments/assets/8cbe6487-b77d-435a-a392-fc9903923899)
</details>

<details>
  <summary>hitBoxes (server permission required) </summary>
![shot-Apr-05-2025_19 03 37](https://github.com/user-attachments/assets/ead8e97d-40b0-4be2-a6e5-89d4953d5c99)
![shot-Apr-05-2025_19 03 34](https://github.com/user-attachments/assets/dbed14fb-ee35-4047-bb9e-cef04327eef1)
![shot-Apr-07-2025_01 02 48](https://github.com/user-attachments/assets/f41201f4-2c30-4ba8-98a9-eae47b81c563)
</details>


## 0.041
```
+ belist
```
## 0.04
```
+ cg_drawOutline
+ cg_enemyOutlineColor
+ cg_teamOutlineColor
+ cg_enemyOutlineColorUnique
+ cg_enemyOutlineSize
+ cg_teamOutlineSize
```
## 0.032/0.033
```
+ cg_drawHitBox 2/3
```
## 0.031
```
+ cg_drawGunForceAspect
* clientconfig
  + Hitbox permission from server
```
## 0.03
```
* cg_drawHitBox enabled. Server permission required
  + cg_hitBoxColor <color>
  * HQ shader (lol)

* cg_damageDrawFrame variations
  + 1 gradient
  + 2 old solid border
```
## 0.02
```
* cg_drawDamage (3/4) reworked to:
  + cg_damageDrawFrame;
  + cg_damageFrameSize;
  + cg_damageFrameOpaque;

+ cg_shud_currentWeapons;

SHUD:
+ Current Weapon Stats shows accuracy
  + CurrentWeaponStats
  + CurrentWeaponStats_icon

* tempAcc reworked:
  * fixed calc
  - tempAcc_last
    

* weaponlist:
  + border for choosen weapon

+ various fixes
```
## 0.01
```
Cvars:
+ cg_itemsRespawnAnimation
+ cg_drawDamage (3/4)
+ cg_enemyLightning
+ cg_uniqueColorTable
+ cg_noVoteBeep
* cg_drawGun (reworked)

SHUD:
+ Border (border, borderColor)
* Background for text elements (reworked)
+ Background for client events in obituaries
+ Temporal LG accuracy
+ Item pickup style 2
+ Color E/T transparency via color2
+ Armor bar over HP bar
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
