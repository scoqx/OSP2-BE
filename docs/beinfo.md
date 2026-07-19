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
- `RRGGBB` или `RRGGBBAA` — hex (например `FF00FF` или `FF00FF80`).

Предметы: `cg_drawBrightItems`. Оружие: `cg_drawBrightWeapons`.

Комментарии: `#` в начале строки (после пробелов) или пробел/`tab` перед `#` до конца строки. В hex-цвете символ `#` не используется (только `RRGGBB` / `RRGGBBAA`).

`brightcolor_team_CTF_*` — только при `cg_oldFlags` с битом **1** (старые модели флагов: значения 1 и 3). При 0 и 2 — dye-модели, цвета команд / без override из cfg.

Парсер: `code/qcommon/beinfo_cfg.c` (UI и cgame).
