# beinfo.cfg

Опционально в pk3/моде: `beconfigs/beinfo.cfg`. UI читает блок `ui` при старте.

```
AddVar
{
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

| Ключ (в `ui`) | По умолчанию |
|---------------|----------------|
| `be_statusServer1` | `127.0.0.1:27888` |
| `be_statusServer2` | `127.0.0.1:27889` |

Значение: `хост:порт`.

Блок `cgame` (cgame): `brightcolor_<classname> …` — цвет предмета или `brightcolor_weapon_<name>` (как в `bg_misc.c`).

Значение цвета (один из форматов):

- `R G B A` — числа 0.0–1.0, через пробел (например `1 0 0 1`);
- `RRGGBB` или `RRGGBBAA` — hex (например `FF00FF` или `FF00FF80`).

Предметы: `cg_drawBrightItems`. Оружие: `cg_drawBrightWeapons`.

Комментарии: `#` в начале строки (после пробелов) или пробел/`tab` перед `#` до конца строки. В hex-цвете символ `#` не используется (только `RRGGBB` / `RRGGBBAA`).

`brightcolor_team_CTF_*` — только при `cg_oldFlags` с битом **1** (старые модели флагов: значения 1 и 3). При 0 и 2 — dye-модели, цвета команд / без override из cfg.

Парсер: `code/qcommon/beinfo_cfg.c` (UI и cgame).
