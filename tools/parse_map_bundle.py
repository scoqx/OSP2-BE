#!/usr/bin/env python3
r"""
parse_map_bundle.py — mapdata для OSP2-BE (спавны QSP1, shadow-пикапы QITM1, локации QLM1, tpexit QTX1).

Как клиент грузит данные (cgame)
--------------------------------
1) mapdata/<имя_карты>.q3lb — приоритет (локальный тест), затем mapdata/mapdata.q3lb.
2) Если карты нет в архиве или blob пустой:
   - mapdata/<имя_карты>.qmap  (бинарный QMAP)
   - mapdata/<имя_карты>.q3lb  (мини-архив на одну карту)

Файлы кладут в pk3 мода или в отдельный pk3: mapdata/...

Одна карта (доп. mapdata для игрока / маппер)
---------------------------------------------
  python parse_map_bundle.py C:\maps\mymap.bsp
      → assets/mapdata/mymap.qmap (бинарь, имя = имя карты)

  python parse_map_bundle.py mymap.pk3 -o D:\Quake3\osp\mapdata\mymap.qmap

  python parse_map_bundle.py mymap.bsp -o mapdata\mymap.q3lb

Расширения -o:
  .qmap  — бинарный QMAP (для игры)
  .q3lb  — один map в Q3LB
  .json  — только для правки; в игре не читается
  каталог — пишет <имя_карты>.qmap внутрь

Пакет mod / все карты
---------------------
  python parse_map_bundle.py
      Полный пересбор с нуля: все карты → backup/mapdata/*.qmap (JSON),
      затем assets/mapdata/mapdata.q3lb (один файл для pk3).

  python parse_map_bundle.py --batch -o backup/mapdata
      Только JSON .qmap на карту (без q3lb).

  python parse_map_bundle.py --batch --pack-q3lb
      JSON в backup/mapdata, затем assets/mapdata/mapdata.q3lb

  python parse_map_bundle.py --pack-q3lb backup/mapdata -o assets/mapdata/mapdata.q3lb
      Пересобрать q3lb из уже готовых .qmap/.json

Прочее
------
  --freeze          правила freeze для спавнов/локаций
  --loc-gametype    gametype для target_location (default: ctf)
  --pk3-dir DIR     доп. папки с pk3 (можно несколько; идут после стандартных)

Скан по умолчанию (--batch): baseq3 → osp → C:\\maps (позже перекрывает раньше).
  QITM1 v2 only — spawn origin + flags; drop в cgame (CM). Пересобери mapdata.q3lb текущим скриптом.
  MAP_ITEM_ENTITY_EXCLUDE — точки QITM, выкидываемые по индексу entity на карте.
  Локации без target_location в BSP: assets/locs/<map>.cfg (как locs/<map>.cfg в игре).
  -v                подробный лог

Запуск из корня репозитория OSP2-PBE:  python tools/parse_map_bundle.py ...
"""

from __future__ import annotations

import argparse
import json
import math
import re
import struct
import sys
import time
from pathlib import Path
from typing import Any

from parse_map_locations import (
    DEFAULT_MAP_SCAN_DIRS,
    DEFAULT_PK3_DIRS,
    GT_CTF,
    GT_FFA,
    GT_SINGLE_PLAYER,
    GT_TEAM,
    GT_TOURNAMENT,
    assign_location_ids,
    collect_batch_jobs,
    entity_would_spawn,
    entities_lump_from_bytes,
    pack_locations_binary,
    load_custom_locations_for_map,
    parse_entity_lump,
    parse_origin,
    read_bsp_bytes,
    simulate_gentity_slots,
    _spawn_int,
)
from parse_map_spawns import (
    MODE_ORDER,
    Q3LB_MAGIC,
    SPAWN_DOC_VERSION,
    GAMETYPE_POOL_NAMES,
    build_spawn_report,
    collect_spawn_jobs,
    pack_qsp1_binary,
    parse_angles,
    _filter_mask,
)

TPEXIT_DOC_VERSION = 1
QTX1_VERSION = 1

DEST_CLASSNAMES = frozenset(
    {
        "target_position",
        "misc_teleporter_dest",
        "info_notnull",
    }
)


def _yaw_from_ent(ent: dict[str, str]) -> float:
    angles = parse_angles(ent)
    if angles:
        return angles[1]
    return 0.0


def _resolve_target_chain(
    entities: list[dict[str, str]], target_name: str, depth: int = 0
) -> list[dict[str, str]]:
    if depth > 8 or not target_name:
        return []

    out: list[dict[str, str]] = []
    for ent in entities:
        if ent.get("targetname") != target_name:
            continue
        cn = ent.get("classname", "")
        if cn == "target_teleporter":
            next_target = ent.get("target")
            if next_target:
                out.extend(_resolve_target_chain(entities, next_target, depth + 1))
            elif ent.get("origin"):
                out.append(ent)
        elif cn in DEST_CLASSNAMES and ent.get("origin"):
            out.append(ent)
        elif cn == "target_relay" and ent.get("target"):
            out.extend(_resolve_target_chain(entities, ent["target"], depth + 1))
    return out


def _dedupe_tpexit_points(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    seen: set[tuple[float, float, float]] = set()
    out: list[dict[str, Any]] = []
    for row in rows:
        o = row["o"]
        key = (float(o[0]), float(o[1]), float(o[2]))
        if key in seen:
            continue
        seen.add(key)
        out.append(row)
    return out


def collect_tpexit_points(entities: list[dict[str, str]]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for ent in entities:
        if ent.get("classname") != "trigger_teleport":
            continue
        target = ent.get("target")
        if not target:
            continue
        for dest in _resolve_target_chain(entities, target):
            origin = parse_origin(dest)
            if not origin:
                continue
            yaw = _yaw_from_ent(dest)
            rows.append(
                {
                    "c": dest.get("classname", "?"),
                    "o": [
                        int(x) if float(x).is_integer() else x for x in origin
                    ],
                    "y": int(yaw) if yaw.is_integer() else yaw,
                }
            )
    return _dedupe_tpexit_points(rows)


def pack_qtx1_binary(report: dict[str, Any]) -> bytes:
    points = report.get("points") or []
    if not points:
        raise ValueError("no points")

    chunks = [b"QTX1", struct.pack("<HH", QTX1_VERSION, len(points))]
    for pt in points:
        ox, oy, oz = pt["o"]
        yaw = int(pt.get("y", 0))
        chunks.append(struct.pack("<fffh", float(ox), float(oy), float(oz), yaw))
    return b"".join(chunks)


def _fmt_origin_xyz(ox: float, oy: float, oz: float) -> list[float | int]:
    return [int(v) if float(v).is_integer() else v for v in (ox, oy, oz)]


def apply_item_spawn_metadata(
    points: list[dict[str, Any]],
    entities: list[dict[str, str]],
) -> None:
    """QITM v2: spawn origin + suspended; drop is done in cgame (CM trace)."""
    for pt in points:
        ent = entities[pt["e"]]
        ox, oy, oz = pt["o"]
        ox, oy, oz = float(ox), float(oy), float(oz)
        pt["so"] = _fmt_origin_xyz(ox, oy, oz)
        if bool(_spawn_int(ent, "spawnflags", 0) & 1):
            pt["sf"] = 1
        elif "sf" in pt:
            del pt["sf"]
        pt["o"] = list(pt["so"])


def _progress_index_tag(index: int) -> str:
    return f"[{index:>4}]"


REPO_ROOT = Path(__file__).resolve().parent.parent
BG_MISC = REPO_ROOT / "code" / "game" / "bg_misc.c"
DEFAULT_OUT_DIR = REPO_ROOT / "backup" / "mapdata"
DEFAULT_BUNDLE_PATH = REPO_ROOT / "assets" / "mapdata" / "mapdata.q3lb"
DEFAULT_MAPDATA_DIR = REPO_ROOT / "assets" / "mapdata"
DEFAULT_MAP_DIRS = [
    *DEFAULT_MAP_SCAN_DIRS,
    REPO_ROOT / "testmaps",
]

# Entity lump index ("e" in item points) dropped from shadow mapdata on specific maps.
MAP_ITEM_ENTITY_EXCLUDE: dict[str, frozenset[int]] = {
    # pak0 q3dm10: лишние entity (дубль жёлтой брони, битый drop у small health / shard).
    "q3dm10": frozenset({29, 58, 72}),
}

MAP_DOC_VERSION = 1
QMAP_MAGIC = b"QMAP"
QMAP_VERSION = 2
QITM1_VERSION = 2
QITM1_FLAG_SUSPENDED = 1

IT_HEALTH = 1
IT_ARMOR = 2
IT_WEAPON = 4
IT_AMMO = 8
IT_POWERUP = 16
IT_HOLDABLE = 32
IT_TEAM = 64

SHADOW_CAT_HEALTH = 1
SHADOW_CAT_ARMOR = 2
SHADOW_CAT_WEAPON = 4
SHADOW_CAT_AMMO = 8
SHADOW_CAT_POWERUP = 16
SHADOW_CAT_FLAG = 32

GTYPE_TO_SHADOW = {
    IT_HEALTH: SHADOW_CAT_HEALTH,
    IT_ARMOR: SHADOW_CAT_ARMOR,
    IT_WEAPON: SHADOW_CAT_WEAPON,
    IT_AMMO: SHADOW_CAT_AMMO,
    IT_POWERUP: SHADOW_CAT_POWERUP,
    IT_HOLDABLE: SHADOW_CAT_POWERUP,
    IT_TEAM: SHADOW_CAT_FLAG,
}

PICKUP_CLASSNAMES = frozenset(
    {
        "team_CTF_redflag",
        "team_CTF_blueflag",
    }
)


def _load_item_table(bg_misc_path: Path) -> dict[str, tuple[int, int, int]]:
    """classname -> (bg_itemlist index, giType constant, shadow category bit)."""
    text = bg_misc_path.read_text(encoding="utf-8", errors="replace")
    start = text.find("gitem_t bg_itemlist[]")
    if start < 0:
        raise ValueError(f"{bg_misc_path}: bg_itemlist not found")
    section = text[start:]
    end_mark = section.find("};\n\nint     bg_numItems")
    if end_mark > 0:
        section = section[: end_mark + 2]
    table: dict[str, tuple[int, int, int]] = {}
    idx = 0
    for m in re.finditer(r'\n\t\{\n\t\t"([^"]+)"', section):
        classname = m.group(1)
        if classname == "NULL":
            break
        block_start = m.start()
        block_end = section.find("\n\t},", block_start)
        if block_end < 0:
            break
        block = section[block_start:block_end]
        it_m = re.search(r"\b(IT_[A-Z]+)\b", block)
        if not it_m:
            continue
        it_name = it_m.group(1)
        idx += 1
        gi = {
            "IT_HEALTH": IT_HEALTH,
            "IT_ARMOR": IT_ARMOR,
            "IT_WEAPON": IT_WEAPON,
            "IT_AMMO": IT_AMMO,
            "IT_POWERUP": IT_POWERUP,
            "IT_HOLDABLE": IT_HOLDABLE,
            "IT_TEAM": IT_TEAM,
        }.get(it_name, 0)
        cat = GTYPE_TO_SHADOW.get(gi, 0)
        table[classname] = (idx, gi, cat)
    return table


def _is_pickup_classname(cn: str) -> bool:
    if cn in PICKUP_CLASSNAMES:
        return True
    return (
        cn.startswith("item_")
        or cn.startswith("weapon_")
        or cn.startswith("ammo_")
        or cn.startswith("holdable_")
    )


def _item_point_row(entity_index: int, ent: dict[str, str], item_table: dict[str, tuple[int, int, int]]) -> dict[str, Any] | None:
    raw = ent.get("classname", "")
    if not _is_pickup_classname(raw):
        return None
    info = item_table.get(raw)
    if not info:
        return None
    modelindex, _gi, cat = info
    origin = parse_origin(ent) or (0.0, 0.0, 0.0)
    row: dict[str, Any] = {
        "e": entity_index,
        "c": raw,
        "mi": modelindex,
        "sc": cat,
        "o": [int(x) if float(x).is_integer() else x for x in origin],
    }
    fl = _filter_mask(ent)
    if fl:
        row["fl"] = fl
    gt = ent.get("gametype", "")
    if gt:
        row["gt"] = gt
    return row


def _classified_items_for_gametype(
    points: list[dict[str, Any]],
    entities: list[dict[str, str]],
    gametype: int,
    freeze: bool,
) -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for pi, pt in enumerate(points):
        ent = entities[pt["e"]]
        spawned = entity_would_spawn(ent, gametype, freeze)
        out.append(
            {
                "pi": pi,
                "spawned": spawned,
            }
        )
    return out


def build_item_report(
    entities: list[dict[str, str]],
    item_table: dict[str, tuple[int, int, int]],
    freeze: bool,
) -> dict[str, Any]:
    points: list[dict[str, Any]] = []
    for idx, ent in enumerate(entities):
        if idx == 0:
            continue
        row = _item_point_row(idx, ent, item_table)
        if row:
            points.append(row)

    apply_item_spawn_metadata(points, entities)

    modes: dict[str, Any] = {}
    for gt, name in GAMETYPE_POOL_NAMES.items():
        classified = _classified_items_for_gametype(points, entities, gt, freeze)
        spawned_pi = [s["pi"] for s in classified if s["spawned"]]
        modes[name] = {"spawned": spawned_pi}

    return {"points": points, "modes": modes}


def _apply_map_item_entity_excludes(map_name: str, items_doc: dict[str, Any]) -> None:
    exclude = MAP_ITEM_ENTITY_EXCLUDE.get(map_name.lower())
    if not exclude:
        return
    points: list[dict[str, Any]] = items_doc.get("points") or []
    if not points:
        return
    old_to_new: dict[int, int] = {}
    new_points: list[dict[str, Any]] = []
    for old_pi, pt in enumerate(points):
        if pt.get("e") in exclude:
            continue
        old_to_new[old_pi] = len(new_points)
        new_points.append(pt)
    if len(new_points) == len(points):
        return
    items_doc["points"] = new_points
    modes = items_doc.get("modes") or {}
    for mode_data in modes.values():
        spawned = mode_data.get("spawned") or []
        mode_data["spawned"] = [old_to_new[pi] for pi in spawned if pi in old_to_new]


def build_locations_report(
    entities: list[dict[str, str]],
    gametype: int,
    freeze: bool,
) -> dict[str, Any] | None:
    slots = simulate_gentity_slots(entities, gametype, freeze)
    locations = assign_location_ids(slots, freeze)
    if not locations:
        return None
    return {
        "map": "",
        "location_count": len(locations),
        "locations": locations,
    }


def build_map_bundle_report(
    map_path: Path,
    freeze: bool,
    loc_gametype: int,
    bsp_in_pk3: str | None = None,
    item_table: dict[str, tuple[int, int, int]] | None = None,
) -> dict[str, Any]:
    if item_table is None:
        item_table = _load_item_table(BG_MISC)

    spawn_doc = build_spawn_report(map_path, freeze, bsp_in_pk3)
    bsp_bytes, source, map_name = read_bsp_bytes(map_path, bsp_in_pk3)
    lump = entities_lump_from_bytes(bsp_bytes, source)
    entities = parse_entity_lump(lump)

    items_doc = build_item_report(entities, item_table, freeze)
    _apply_map_item_entity_excludes(map_name, items_doc)
    loc_doc = build_locations_report(entities, loc_gametype, freeze)
    if not loc_doc:
        custom_locs = load_custom_locations_for_map(map_name)
        if custom_locs:
            loc_doc = {
                "map": map_name,
                "location_count": len(custom_locs),
                "locations": custom_locs,
            }
    tpexit_points = collect_tpexit_points(entities)

    doc: dict[str, Any] = {
        "v": MAP_DOC_VERSION,
        "map": map_name,
        "src": spawn_doc.get("src", ""),
        "spawns": {
            "v": spawn_doc.get("v", SPAWN_DOC_VERSION),
            "points": spawn_doc["points"],
            "modes": spawn_doc["modes"],
        },
        "items": items_doc,
    }
    if tpexit_points:
        doc["tpexit"] = {
            "v": TPEXIT_DOC_VERSION,
            "points": tpexit_points,
        }
    if loc_doc:
        loc_doc["map"] = map_name
        doc["locations"] = loc_doc
    if freeze:
        doc["freeze"] = 1
    return doc


def pack_qitm1_binary(items_doc: dict[str, Any]) -> bytes:
    points = items_doc.get("points") or []
    modes = items_doc.get("modes") or {}
    if not points:
        raise ValueError("no item points")

    chunks = [b"QITM1", struct.pack("<HH", QITM1_VERSION, len(points))]
    for pi, pt in enumerate(points):
        if "so" not in pt:
            if "o" not in pt:
                raise ValueError(f"item point {pi} missing 'so'/'o' (reparse with --batch)")
            pt["so"] = list(pt["o"])
        ox, oy, oz = pt["so"]
        mi = int(pt["mi"])
        sc = int(pt.get("sc", 0)) & 0xFF
        flags = QITM1_FLAG_SUSPENDED if pt.get("sf") else 0
        chunks.append(
            struct.pack("<fffHBB", float(ox), float(oy), float(oz), mi, sc, flags)
        )

    for mode_name in MODE_ORDER:
        block = modes.get(mode_name, {})
        spawned = block.get("spawned") or []
        chunks.append(struct.pack("<H", len(spawned)))
        for idx in spawned:
            chunks.append(struct.pack("<H", int(idx)))

    return b"".join(chunks)


def pack_qmap1_binary(doc: dict[str, Any]) -> bytes:
    spawns = doc.get("spawns") or {}
    items = doc.get("items") or {}
    locations = doc.get("locations")
    tpexit = doc.get("tpexit")

    qsp = pack_qsp1_binary(
        {
            "v": spawns.get("v", SPAWN_DOC_VERSION),
            "points": spawns.get("points") or [],
            "modes": spawns.get("modes") or {},
        }
    )
    qitm = b""
    if items.get("points"):
        qitm = pack_qitm1_binary(items)
    qlm = b""
    if locations and locations.get("locations"):
        qlm = pack_locations_binary(locations)
    qtx = b""
    if tpexit and tpexit.get("points"):
        qtx = pack_qtx1_binary(
            {
                "v": tpexit.get("v", 1),
                "map": doc.get("map", ""),
                "points": tpexit["points"],
            }
        )

    return (
        QMAP_MAGIC
        + struct.pack("<HIIII", QMAP_VERSION, len(qsp), len(qitm), len(qlm), len(qtx))
        + qsp
        + qitm
        + qlm
        + qtx
    )


def _qmap_qitm_version(qmap_blob: bytes) -> int:
    if len(qmap_blob) < 18 or qmap_blob[:4] != QMAP_MAGIC:
        return 0
    qmap_ver = qmap_blob[4] | (qmap_blob[5] << 8)
    qsp_len = struct.unpack_from("<I", qmap_blob, 6)[0]
    qitm_len = struct.unpack_from("<I", qmap_blob, 10)[0]
    hdr = 22 if qmap_ver >= 2 else 18
    if qitm_len <= 0 or hdr + qsp_len + qitm_len > len(qmap_blob):
        return 0
    sec = qmap_blob[hdr + qsp_len : hdr + qsp_len + qitm_len]
    if len(sec) < 9 or sec[:5] != b"QITM1":
        return 0
    return struct.unpack_from("<H", sec, 5)[0]


def _read_qmap_payload(path: Path) -> bytes:
    raw = path.read_bytes()
    stripped = raw.lstrip()
    if stripped.startswith(b"{"):
        text = raw.decode("utf-8", errors="replace").strip()
        if not text:
            raise ValueError("empty file")
        doc = json.loads(text)
        if doc.get("v") != MAP_DOC_VERSION:
            raise ValueError(f"unsupported v={doc.get('v')!r}")
        return pack_qmap1_binary(doc)

    if len(raw) >= 4 and raw[:4] == QMAP_MAGIC:
        qitm_ver = _qmap_qitm_version(raw)
        if qitm_ver == 1:
            raise ValueError(
                f"{path.name}: stale binary QITM v1 — run --batch on this map, then pack JSON .qmap"
            )
        return raw

    raise ValueError(f"{path.name}: not JSON mapdata and not QMAP binary")


def pack_q3lb_from_mapdata_dir(src_dir: Path, out_path: Path) -> int:
    src_dir = src_dir.resolve()
    if src_dir.is_file():
        if src_dir.suffix.lower() not in (".qmap", ".json"):
            raise ValueError(f"not a .qmap file: {src_dir}")
        paths = [src_dir]
        src_dir = src_dir.parent
    else:
        paths = sorted(list(src_dir.glob("*.qmap")) + list(src_dir.glob("*.json")))

    entries: list[tuple[str, bytes]] = []
    total = len(paths)
    for n, path in enumerate(paths, 1):
        map_name = path.stem.lower()
        if not map_name:
            continue
        try:
            blob = _read_qmap_payload(path)
        except (ValueError, OSError, json.JSONDecodeError) as exc:
            print(f"warn: skip {path.name}: {exc}", file=sys.stderr)
            continue
        entries.append((map_name, blob))
        if n % 50 == 0 or n == total:
            tag = _progress_index_tag(n)
            print(f"\r{tag} pack read {n}/{total}...", end="", file=sys.stderr, flush=True)

    if total:
        print(file=sys.stderr)

    if not entries:
        print("error: no valid .qmap files", file=sys.stderr)
        return 1

    chunks = [Q3LB_MAGIC, struct.pack("<H", len(entries))]
    for map_name, blob in entries:
        name_b = map_name.encode("utf-8")
        if len(name_b) > 255:
            print(f"warn: skip {map_name}: name too long", file=sys.stderr)
            continue
        chunks.append(struct.pack("<B", len(name_b)))
        chunks.append(name_b)
        chunks.append(struct.pack("<I", len(blob)))
        chunks.append(blob)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(b"".join(chunks))
    print(f"packed {len(entries)} maps -> {out_path}", file=sys.stderr)
    print("install: mapdata/mapdata.q3lb in mod pk3 (assets/mapdata/)", file=sys.stderr)
    return 0


def batch_write_map_reports(
    quake_dirs: list[Path],
    out_dir: Path,
    freeze: bool,
    loc_gametype: int,
    skip_existing: bool,
    verbose: bool,
    item_table: dict[str, tuple[int, int, int]],
) -> int:
    out_dir = out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    jobs = collect_batch_jobs(quake_dirs)
    if not jobs:
        print("error: no maps/*.bsp found", file=sys.stderr)
        return 1

    written = 0
    skipped = 0
    skipped_exist = 0
    skipped_empty = 0

    print(f"scanning {len(jobs)} unique maps -> {out_dir}", file=sys.stderr)

    sorted_maps = sorted(jobs.keys())

    for index, map_name in enumerate(sorted_maps, 1):
        tag = _progress_index_tag(index)
        label, path, inner, _ = jobs[map_name]
        out_path = out_dir / f"{map_name}.qmap"
        if skip_existing and out_path.is_file():
            skipped_exist += 1
            continue
        try:
            doc = build_map_bundle_report(path, freeze, loc_gametype, inner, item_table)
        except (ValueError, OSError, zipfile.BadZipFile) as exc:
            skipped += 1
            print(f"{tag} skip {map_name} ({label}): {exc}", file=sys.stderr)
            continue

        n_spawn = len(doc.get("spawns", {}).get("points") or [])
        n_item = len(doc.get("items", {}).get("points") or [])
        n_loc = len((doc.get("locations") or {}).get("locations") or [])
        if n_spawn == 0 and n_item == 0 and n_loc == 0:
            skipped_empty += 1
            if verbose:
                print(f"{tag} skip {map_name}: empty bundle ({label})", file=sys.stderr)
            continue

        out_path.write_text(
            json.dumps(doc, ensure_ascii=False, separators=(",", ":")) + "\n",
            encoding="utf-8",
        )
        written += 1
        print(
            f"{tag} {map_name}: wrote {out_path.name} "
            f"(spawns={n_spawn} items={n_item} locs={n_loc}) <- {label}",
            file=sys.stderr,
        )

    print(
        f"done: {written} written, {skipped_exist} already present, "
        f"{skipped_empty} empty, {skipped} errors",
        file=sys.stderr,
    )
    return 0 if written or skipped_exist else 1


def run_default_full_rebuild(
    item_table: dict[str, tuple[int, int, int]],
    loc_gametype: int,
    *,
    freeze: bool = False,
    verbose: bool = False,
    pk3_dirs: list[Path] | None = None,
) -> int:
    out_dir = DEFAULT_OUT_DIR.resolve()
    pack_out = DEFAULT_BUNDLE_PATH.resolve()
    scan_dirs = pk3_dirs if pk3_dirs else DEFAULT_MAP_DIRS

    print("full mapdata rebuild (default, no CLI args)", file=sys.stderr)
    print(f"  1/2 parse maps -> {out_dir}", file=sys.stderr)
    print(f"  2/2 pack Q3LB -> {pack_out}", file=sys.stderr)

    rc = batch_write_map_reports(
        scan_dirs,
        out_dir,
        freeze,
        loc_gametype,
        skip_existing=False,
        verbose=verbose,
        item_table=item_table,
    )
    if rc != 0:
        return rc
    return pack_q3lb_from_mapdata_dir(out_dir, pack_out)


def write_single_map_qmap(out_path: Path, map_name: str, doc: dict[str, Any]) -> None:
    out_path = out_path.resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(pack_qmap1_binary(doc))
    print(
        f"wrote {out_path} (install as mapdata/{map_name}.qmap in pk3 if not in mapdata.q3lb)",
        file=sys.stderr,
    )


def write_single_map_q3lb(out_path: Path, map_name: str, doc: dict[str, Any]) -> None:
    out_path = out_path.resolve()
    blob = pack_qmap1_binary(doc)
    name_b = map_name.lower().encode("utf-8")
    if len(name_b) > 255:
        raise ValueError("map name too long for Q3LB")
    chunks = [
        Q3LB_MAGIC,
        struct.pack("<H", 1),
        struct.pack("<B", len(name_b)),
        name_b,
        struct.pack("<I", len(blob)),
        blob,
    ]
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(b"".join(chunks))
    print(f"wrote {out_path} (install as mapdata/{map_name}.q3lb in pk3 if not in mapdata.q3lb)", file=sys.stderr)


def main() -> int:
    if len(sys.argv) == 1:
        item_table = _load_item_table(BG_MISC)
        return run_default_full_rebuild(item_table, GT_CTF)

    parser = argparse.ArgumentParser(
        description="Сбор mapdata (spawns, shadow items, locations, tpexit) для OSP2-BE.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Примеры:\n"
            "  %(prog)s\n"
            "      полный пересбор → backup/mapdata + assets/mapdata/mapdata.q3lb\n"
            "  %(prog)s mymap.bsp\n"
            "  %(prog)s mymap.bsp -o assets/mapdata/mymap.qmap\n"
            "  %(prog)s --batch -o backup/mapdata\n"
            "  %(prog)s --batch --pack-q3lb\n"
            "\n"
            "Подробная инструкция — в docstring в начале файла parse_map_bundle.py"
        ),
    )
    parser.add_argument("input", nargs="?", type=Path, help=".bsp, .pk3 или один pk3/архив с одной картой")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Файл (.qmap/.q3lb/.json) или каталог; для одной карты без -o → assets/mapdata/<map>.qmap",
    )
    parser.add_argument("--batch", action="store_true", help="Обход pk3/Quake dirs, много карт → JSON .qmap")
    parser.add_argument(
        "--pk3-dir",
        type=Path,
        action="append",
        dest="pk3_dirs",
        metavar="DIR",
        help="Доп. каталоги с pk3 (повторяемо). По умолчанию: baseq3, osp, C:\\maps, testmaps",
    )
    parser.add_argument("--freeze", action="store_true", help="Freeze-режим для спавнов и локаций")
    parser.add_argument(
        "--loc-gametype",
        choices=["ffa", "tournament", "single", "team", "ctf"],
        default="ctf",
        help="Gametype для фильтров target_location в locs (по умолчанию: ctf)",
    )
    parser.add_argument("--skip-existing", action="store_true", help="В --batch не перезаписывать существующие .qmap")
    parser.add_argument("-v", "--verbose", action="store_true", help="Подробный вывод")
    parser.add_argument(
        "--pack-q3lb",
        type=Path,
        nargs="?",
        const=DEFAULT_OUT_DIR,
        metavar="SRC_DIR",
        help="Собрать SRC_DIR/*.qmap|.json в mapdata.q3lb (-o задаёт выходной q3lb)",
    )
    args = parser.parse_args()

    loc_gt = {
        "ffa": GT_FFA,
        "tournament": GT_TOURNAMENT,
        "single": GT_SINGLE_PLAYER,
        "team": GT_TEAM,
        "ctf": GT_CTF,
    }[args.loc_gametype]

    item_table = _load_item_table(BG_MISC)

    if args.pack_q3lb is not None and not args.batch:
        src = args.pack_q3lb.resolve()
        out = args.output.resolve() if args.output else DEFAULT_BUNDLE_PATH.resolve()
        return pack_q3lb_from_mapdata_dir(src, out)

    if args.batch:
        dirs = args.pk3_dirs if args.pk3_dirs else DEFAULT_MAP_DIRS
        if args.pack_q3lb is not None:
            out_dir = args.pack_q3lb.resolve()
            pack_out = (
                args.output.resolve()
                if args.output
                else DEFAULT_BUNDLE_PATH.resolve()
            )
        elif args.output and args.output.suffix.lower() == ".q3lb":
            out_dir = DEFAULT_OUT_DIR.resolve()
            pack_out = args.output.resolve()
        else:
            out_dir = args.output.resolve() if args.output else DEFAULT_OUT_DIR.resolve()
            pack_out = DEFAULT_BUNDLE_PATH.resolve()
        rc = batch_write_map_reports(
            dirs, out_dir, args.freeze, loc_gt, args.skip_existing, args.verbose, item_table
        )
        if rc != 0:
            return rc
        if args.pack_q3lb is not None or (args.output and args.output.suffix.lower() == ".q3lb"):
            return pack_q3lb_from_mapdata_dir(out_dir, pack_out)
        return rc

    if not args.input:
        parser.error("input required unless --batch")

    input_path = args.input.resolve()
    jobs = collect_spawn_jobs(input_path)
    if len(jobs) != 1:
        parser.error("single map input required (use --batch for folders)")

    map_name = next(iter(jobs.keys()))
    label, path, inner, _ = jobs[map_name]
    doc = build_map_bundle_report(path, args.freeze, loc_gt, inner, item_table)
    doc["map"] = map_name

    if args.output:
        out = args.output.resolve()
        if out.suffix.lower() == ".q3lb":
            write_single_map_q3lb(out, map_name, doc)
        elif out.suffix.lower() in (".qmap", ".json"):
            if out.suffix.lower() == ".qmap":
                write_single_map_qmap(out, map_name, doc)
            else:
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_text(json.dumps(doc, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
                print(f"wrote {out} (JSON; use .qmap for in-game mapdata override)", file=sys.stderr)
        elif out.is_dir():
            write_single_map_qmap(out / f"{map_name}.qmap", map_name, doc)
        else:
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_text(json.dumps(doc, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            print(f"wrote {out} (JSON)", file=sys.stderr)
    else:
        write_single_map_qmap(DEFAULT_MAPDATA_DIR / f"{map_name}.qmap", map_name, doc)

    return 0


def _print_elapsed_seconds(start: float) -> None:
    elapsed = time.perf_counter() - start
    if elapsed >= 60.0:
        print(f"elapsed: {elapsed / 60.0:.1f} min ({elapsed:.1f} s)", file=sys.stderr)
    else:
        print(f"elapsed: {elapsed:.2f} s", file=sys.stderr)


if __name__ == "__main__":
    _t0 = time.perf_counter()
    _rc = main()
    _print_elapsed_seconds(_t0)
    raise SystemExit(_rc)
