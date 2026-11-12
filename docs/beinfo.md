# beinfo.cfg

Опционально в pk3/моде: `beconfigs/beinfo.cfg`. UI читает блок `ui` при старте; `ttfFonts` читают и UI, и cgame.

```
AddVar
{
    ttfFonts
    {
        roboto
        {
            scale 1.15;
            offset 0.05;
        }
    }

    ui
    {
        be_statusServer1 = 127.0.0.1:27888;
        be_statusServer2 = 127.0.0.1:27889;
    }

    cgame
    {
        brightcolor_item_armor_shard 0.75 0.75 0.75 1;
    }
}
```

## ttfFonts (UI + cgame)

Per-font оптический `scale` / вертикальный `offset` для TTF (bare name из `fonts.cfg`).

| Ключ | По умолчанию | Clamp |
|------|----------------|-------|
| `scale` | `1` | `0.25`–`2` |
| `offset` | `0` | `-1`–`1` (доля высоты ячейки; `>0` вниз) |

Если шрифт есть в `ttfFonts` — значения из beInfo **перезаписывают** `be_ttfScale` / `be_ttfFontOffset`. Cvars действуют только для TTF без записи (глобальный дефолт / live-тест).

## ui

| Ключ (в `ui`) | По умолчанию |
|---------------|----------------|
| `be_statusServer1` | `127.0.0.1:27888` |
| `be_statusServer2` | `127.0.0.1:27889` |

Значение: `хост:порт`.

## cgame

Блок `cgame`: `brightcolor_<classname> …` — цвет предмета или `brightcolor_weapon_<name>` (как в `bg_misc.c`).

Значение цвета (один из форматов):

- `R G B A` — числа 0.0–1.0, через пробел (например `1 0 0 1`);
- hex: `RGB`, `RGBA`, `RRGGBB` или `RRGGBBAA`, можно с префиксом `#` или `0x` (например `F0F`, `#F0F8`, `FF00FF`, `#FF00FF80`, `0xFF00FF80`). Без альфы (`RGB` / `RRGGBB`) альфа = `FF` (1.0). Короткая форма удваивает цифру: `F0F8` = `FF00FF88`.

Hex из одних десятичных цифр без префикса (например `100`) считается hex, только если после него в той же строке нет других чисел; `100 0 0` читается как `R G B`. Чтобы не было неоднозначности, используйте `#`.

Предметы: `cg_drawBrightItems`. Оружие: `cg_drawBrightWeapons`.

Комментарии: `#` в начале строки (после пробелов) или пробел/`tab` перед `#` до конца строки. Исключение: ` #RGB`, ` #RGBA`, ` #RRGGBB`, ` #RRGGBBAA` в середине строки — это цвет, а не комментарий (строка, начинающаяся с `#`, всегда комментарий).

`brightcolor_team_CTF_*` — только при `cg_oldFlags` с битом **1** (старые модели флагов: значения 1 и 3). При 0 и 2 — dye-модели, цвета команд / без override из cfg.

`flashcolor_weapon_<name> …` — цвет вспышки выстрела в тех же форматах, например `flashcolor_weapon_machinegun FFFF00;`. Имена оружия те же, что у `brightcolor_weapon_*`. Работает для всех оружий, кроме `railgun` и `grapplinghook`. Применяется, только когда у оружия нет своего цвета:

| Оружие | Свой цвет |
|--------|-----------|
| `lightning` | `cg_altLightningColor` / `cg_teamLightningColor` / `cg_enemyLightningColor` |
| `plasmagun` | `cg_plasmaColor` / `cg_enemyPlasmaColor`, затем `cg_dlightPG` |
| остальные | `cg_dlight*` |

Если цвета нет нигде, рисуется стандартная вспышка. Цвет вспышки не зависит от того, что разрешает сервер.

`interface_killcam_font <имя|индекс>` — шрифт надписи `REPLAY` в killcam, например `interface_killcam_font roboto;` или `interface_killcam_font 3;`. Имя как в `fonts.cfg`, индекс — номер шрифта. Читается один раз при загрузке cgame (не квар). Нет ключа или шрифт не найден — шрифт по умолчанию.

Парсер: `code/qcommon/beinfo_cfg.c` (UI и cgame).
