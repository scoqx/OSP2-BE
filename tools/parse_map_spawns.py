#!/usr/bin/env python3
"""
Quake 3 player spawn points from BSP entities lump (OSP2-BE tooling).

Mirrors map entity classnames and gametype spawn filters used by the server
(info_player_*, team_CTF_*, flags). Outputs raw entities plus per-mode pools.

Examples:

  python parse_map_spawns.py ../testmaps/campgrounds.pk3
  python parse_map_spawns.py ../testmaps -o backup/spawns
  python parse_map_spawns.py --batch -o backup/spawns

  Q3LB mapdata: use tools/parse_map_bundle.py (--pack-q3lb → mapdata/mapdata.q3lb).
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
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
    collect_batch_jobs,
    collect_maps_from_pk3s,
    entity_would_spawn,
    entities_lump_from_bytes,
    parse_entity_lump,
    parse_origin,
    read_bsp_bytes,
    _spawn_int,
)

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUT_DIR = REPO_ROOT / "backup" / "spawns"
DEFAULT_SPAWN_DIRS = [*DEFAULT_MAP_SCAN_DIRS]

Q3LB_MAGIC = b"Q3LB"

TEAM_FREE = "FREE"
TEAM_RED = "RED"
TEAM_BLUE = "BLUE"
TEAM_INTERMISSION = "INTERMISSION"

SPAWN_PLAYER_CLASSNAMES = frozenset(
    {
        "info_player_deathmatch",
        "info_player_start",
        "info_player_intermission",
        "team_CTF_redspawn",
        "team_CTF_redplayer",
        "team_CTF_bluespawn",
        "team_CTF_blueplayer",
    }
)

SPAWN_FLAG_CLASSNAMES = frozenset(
    {
        "team_CTF_redflag",
        "team_CTF_blueflag",
    }
)

SPAWN_ALL_CLASSNAMES = SPAWN_PLAYER_CLASSNAMES | SPAWN_FLAG_CLASSNAMES

# Default count on indexed spawn slots (Q3A/TA entity defs); entity "count" overrides.
DEFAULT_COUNT_BY_CLASS: dict[str, int | None] = {
    "info_player_deathmatch": 1,
    "info_player_start": 1,
    "info_player_intermission": None,
    "team_CTF_redspawn": 1,
    "team_CTF_redplayer": 0,
    "team_CTF_bluespawn": 1,
    "team_CTF_blueplayer": 0,
    "team_CTF_redflag": None,
    "team_CTF_blueflag": None,
}

ROLE_BY_CLASS: dict[str, str] = {
    "info_player_deathmatch": "dm",
    "info_player_start": "dm_start",
    "info_player_intermission": "intermission",
    "team_CTF_redspawn": "ctf_respawn",
    "team_CTF_redplayer": "ctf_base",
    "team_CTF_bluespawn": "ctf_respawn",
    "team_CTF_blueplayer": "ctf_base",
    "team_CTF_redflag": "flag_red",
    "team_CTF_blueflag": "flag_blue",
}

TEAM_BY_CLASS: dict[str, str] = {
    "info_player_deathmatch": TEAM_FREE,
    "info_player_start": TEAM_FREE,
    "info_player_intermission": TEAM_INTERMISSION,
    "team_CTF_redspawn": TEAM_RED,
    "team_CTF_redplayer": TEAM_RED,
    "team_CTF_bluespawn": TEAM_BLUE,
    "team_CTF_blueplayer": TEAM_BLUE,
    "team_CTF_redflag": TEAM_RED,
    "team_CTF_blueflag": TEAM_BLUE,
}

GAMETYPE_POOL_NAMES = {
    GT_FFA: "ffa",
    GT_TOURNAMENT: "tournament",
    GT_SINGLE_PLAYER: "single",
    GT_TEAM: "team",
    GT_CTF: "ctf",
}

SPAWN_DOC_VERSION = 2

ROLE_CODE = {
    "dm": 0,
    "dm_start": 1,
    "intermission": 2,
    "ctf_base": 3,
    "ctf_respawn": 4,
    "flag_red": 5,
    "flag_blue": 6,
}

TEAM_CODE = {
    TEAM_FREE: 0,
    TEAM_RED: 1,
    TEAM_BLUE: 2,
    TEAM_INTERMISSION: 3,
}

QSP1_VERSION = 1
MODE_ORDER = ("ffa", "tournament", "single", "team", "ctf")


def _filter_mask(ent: dict[str, str]) -> int:
    mask = 0
    if _spawn_int(ent, "notfree"):
        mask |= 1
    if _spawn_int(ent, "notteam"):
        mask |= 2
    if _spawn_int(ent, "notsingle"):
        mask |= 4
    if _spawn_int(ent, "notq3a"):
        mask |= 8
    return mask


def _short_bsp_source(source: str) -> str:
    if "!" in source:
        pk3_path, inner = source.split("!", 1)
        return f"{Path(pk3_path).name}!{inner.replace(chr(92), '/')}"
    return Path(source).name


def _yaw_from_ent(ent: dict[str, str]) -> float:
    angles = parse_angles(ent)
    if angles:
        return angles[1]
    return 0.0


def _point_row(
    entity_index: int,
    ent: dict[str, str],
) -> dict[str, Any]:
    raw = ent.get("classname", "")
    origin = parse_origin(ent) or (0.0, 0.0, 0.0)
    count = spawn_count(ent, raw)
    yaw = _yaw_from_ent(ent)
    row: dict[str, Any] = {
        "e": entity_index,
        "c": raw,
        "r": ROLE_BY_CLASS.get(raw, "?"),
        "t": TEAM_BY_CLASS.get(raw, "?"),
        "o": [int(x) if float(x).is_integer() else x for x in origin],
        "y": int(yaw) if isinstance(yaw, float) and yaw.is_integer() else yaw,
        "sf": _spawn_int(ent, "spawnflags", 0),
    }
    if count is not None:
        row["n"] = count
    fl = _filter_mask(ent)
    if fl:
        row["fl"] = fl
    gt = ent.get("gametype", "")
    if gt:
        row["gt"] = gt
    return row


def _classified_for_gametype(
    points: list[dict[str, Any]],
    entities: list[dict[str, str]],
    gametype: int,
    freeze: bool,
) -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for pi, pt in enumerate(points):
        ent = entities[pt["e"]]
        raw = pt["c"]
        eff = effective_classname(raw, gametype)
        spawned = entity_would_spawn(ent, gametype, freeze)
        out.append(
            {
                "pi": pi,
                "entity_index": pt["e"],
                "classname": raw,
                "effective_classname": eff,
                "team": pt["t"],
                "role": pt["r"],
                "count": pt.get("n"),
                "origin": pt["o"],
                "spawnflags": pt["sf"],
                "spawned": spawned,
            }
        )
    return out


def parse_angles(ent: dict[str, str]) -> list[float] | None:
    if "angles" in ent:
        parts = ent["angles"].split()
        if len(parts) == 3:
            try:
                return [float(parts[0]), float(parts[1]), float(parts[2])]
            except ValueError:
                return None
    if "angle" in ent:
        try:
            yaw = float(ent["angle"])
            return [0.0, yaw, 0.0]
        except ValueError:
            return None
    return None


def effective_classname(raw: str, gametype: int) -> str:
    if gametype == GT_TEAM and raw in ("team_CTF_redplayer", "team_CTF_blueplayer"):
        return "info_player_deathmatch"
    if raw == "info_player_start":
        return "info_player_deathmatch"
    return raw


def spawn_count(ent: dict[str, str], classname: str) -> int | None:
    if "count" in ent:
        return _spawn_int(ent, "count", 0)
    return DEFAULT_COUNT_BY_CLASS.get(classname)


def classify_spawn_entity(
    ent: dict[str, str], entity_index: int, gametype: int, freeze: bool
) -> dict[str, Any] | None:
    raw = ent.get("classname", "")
    if raw not in SPAWN_ALL_CLASSNAMES:
        return None

    origin = parse_origin(ent)
    eff = effective_classname(raw, gametype)
    count = spawn_count(ent, raw)
    role = ROLE_BY_CLASS.get(raw, "unknown")
    team = TEAM_BY_CLASS.get(raw, "UNKNOWN")

    return {
        "entity_index": entity_index,
        "classname": raw,
        "effective_classname": eff,
        "team": team,
        "role": role,
        "count": count,
        "origin": list(origin) if origin else None,
        "angles": parse_angles(ent),
        "spawnflags": _spawn_int(ent, "spawnflags", 0),
        "spawned": entity_would_spawn(ent, gametype, freeze),
        "filters": {
            "notfree": _spawn_int(ent, "notfree", 0),
            "notteam": _spawn_int(ent, "notteam", 0),
            "notsingle": _spawn_int(ent, "notsingle", 0),
            "notq3a": _spawn_int(ent, "notq3a", 0),
            "gametype": ent.get("gametype", ""),
        },
    }


def _active_player_spawns(
    classified: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    return [
        s
        for s in classified
        if s["spawned"] and s["classname"] in SPAWN_PLAYER_CLASSNAMES
    ]


def _dm_pool(active: list[dict[str, Any]], gametype: int) -> list[dict[str, Any]]:
    """Non-team spawn selection (SelectSpawnPoint) after indexing rules."""
    free_dm = [
        s
        for s in active
        if s["effective_classname"] == "info_player_deathmatch"
        and s["team"] == TEAM_FREE
    ]
    if free_dm:
        return free_dm

    # CTF-only maps: fallback uses all indexed player spawns.
    return [s for s in active if s["role"] != "intermission"]


def _ctf_team_pool(
    active: list[dict[str, Any]], team: str, *, base: bool
) -> list[dict[str, Any]]:
    want_count = 0 if base else 1
    out: list[dict[str, Any]] = []
    for s in active:
        if s["team"] != team:
            continue
        if s["role"] not in ("ctf_base", "ctf_respawn"):
            continue
        if s["count"] != want_count:
            continue
        out.append(s)
    return out


def build_pools_for_gametype(
    classified: list[dict[str, Any]], gametype: int
) -> dict[str, list[int]]:
    """Pool name -> point indices (`pi`)."""

    def pis(items: list[dict[str, Any]]) -> list[int]:
        return [s["pi"] for s in items]

    active = _active_player_spawns(classified)
    flags = [s for s in classified if s["spawned"] and s["role"].startswith("flag_")]
    intermission = [s for s in active if s["role"] == "intermission"]
    dm_pool = _dm_pool(active, gametype)

    pools: dict[str, list[int]] = {
        "intermission": pis(intermission),
        "flags": pis(flags),
    }

    if gametype >= GT_CTF:
        pools["red_base"] = pis(_ctf_team_pool(active, TEAM_RED, base=True))
        pools["red_respawn"] = pis(_ctf_team_pool(active, TEAM_RED, base=False))
        pools["blue_base"] = pis(_ctf_team_pool(active, TEAM_BLUE, base=True))
        pools["blue_respawn"] = pis(_ctf_team_pool(active, TEAM_BLUE, base=False))
        pools["dm_fallback"] = pis(dm_pool)
    elif gametype == GT_TEAM:
        pools["dm"] = pis(dm_pool)
        pools["team_spawns_indexed"] = pis(
            [
                s
                for s in active
                if s["role"] in ("ctf_respawn",) and s["team"] in (TEAM_RED, TEAM_BLUE)
            ]
        )
    else:
        pools["dm"] = pis(dm_pool)
        initial = [s for s in dm_pool if s["spawnflags"] & 1]
        pools["dm_initial"] = pis(initial)

    return pools


def build_spawn_report_from_entities(
    entities: list[dict[str, str]],
    source: str,
    map_name: str,
    freeze: bool,
) -> dict[str, Any]:
    points: list[dict[str, Any]] = []
    raw_by_class: dict[str, int] = {}
    for idx, ent in enumerate(entities):
        if idx == 0:
            continue
        cn = ent.get("classname", "")
        if cn not in SPAWN_ALL_CLASSNAMES:
            continue
        points.append(_point_row(idx, ent))
        raw_by_class[cn] = raw_by_class.get(cn, 0) + 1

    modes: dict[str, Any] = {}
    for gt, name in GAMETYPE_POOL_NAMES.items():
        classified = _classified_for_gametype(points, entities, gt, freeze)
        spawned_pi = [s["pi"] for s in classified if s["spawned"]]
        modes[name] = {
            "spawned": spawned_pi,
            "pools": build_pools_for_gametype(classified, gt),
        }

    doc: dict[str, Any] = {
        "v": SPAWN_DOC_VERSION,
        "map": map_name,
        "src": _short_bsp_source(source),
        "points": points,
        "modes": modes,
    }
    if freeze:
        doc["freeze"] = 1
    if raw_by_class:
        doc["counts"] = raw_by_class
    return doc


def build_spawn_report(
    map_path: Path,
    freeze: bool,
    bsp_in_pk3: str | None = None,
) -> dict[str, Any]:
    bsp_bytes, source, map_name = read_bsp_bytes(map_path, bsp_in_pk3)
    lump = entities_lump_from_bytes(bsp_bytes, source)
    entities = parse_entity_lump(lump)
    return build_spawn_report_from_entities(entities, source, map_name, freeze)


def build_spawn_report_debug(
    map_path: Path,
    freeze: bool,
    bsp_in_pk3: str | None = None,
) -> dict[str, Any]:
    """Verbose JSON (entities duplicated per gametype) for inspection only."""
    bsp_bytes, source, map_name = read_bsp_bytes(map_path, bsp_in_pk3)
    lump = entities_lump_from_bytes(bsp_bytes, source)
    entities = parse_entity_lump(lump)

    raw_by_class: dict[str, int] = {}
    for ent in entities:
        cn = ent.get("classname", "")
        if cn in SPAWN_ALL_CLASSNAMES:
            raw_by_class[cn] = raw_by_class.get(cn, 0) + 1

    per_gt: dict[str, Any] = {}
    for gt, name in GAMETYPE_POOL_NAMES.items():
        classified: list[dict[str, Any]] = []
        pi = 0
        for idx, ent in enumerate(entities):
            if idx == 0:
                continue
            row = classify_spawn_entity(ent, idx, gt, freeze)
            if row is not None:
                row["pi"] = pi
                pi += 1
                classified.append(row)
        per_gt[name] = {
            "gametype": gt,
            "entities": classified,
            "pools": build_pools_for_gametype(classified, gt),
        }

    return {
        "map": map_name,
        "bsp": source,
        "freeze": freeze,
        "raw_class_counts": raw_by_class,
        "gametypes": per_gt,
    }


def _txt_num(value: float | int) -> str:
    if isinstance(value, float) and value.is_integer():
        return str(int(value))
    if isinstance(value, int):
        return str(value)
    return f"{value:g}"


def _point_suffix_from_row(pt: dict[str, Any]) -> str:
    o = pt.get("o") or [0, 0, 0]
    parts = [_txt_num(o[0]), _txt_num(o[1]), _txt_num(o[2])]
    parts.append(str(pt.get("sf", 0)))
    if "n" in pt:
        parts.append(str(pt["n"]))
    return " ".join(parts)


def format_report_text(report: dict[str, Any], gametype_name: str = "ctf") -> str:
    points = report["points"]
    mode = report["modes"][gametype_name]
    lines = [
        f"QSP1 {report['map']} {gametype_name} {len(mode['spawned'])}",
    ]
    counts = report.get("counts") or {}
    if counts:
        lines.append("# raw: " + " ".join(f"{k}={v}" for k, v in sorted(counts.items())))

    for pi in mode["spawned"]:
        pt = points[pi]
        o = pt["o"]
        lines.append(
            f"ENT {pt['c']} {pt['t']} {pt['r']} "
            f"{_txt_num(o[0])} {_txt_num(o[1])} {_txt_num(o[2])} "
            f"{pt.get('sf', 0)} {pt.get('n', '')}"
        )

    for pool_name, indices in mode["pools"].items():
        if not indices:
            continue
        for pi in indices:
            lines.append(f"POOL {pool_name} {_point_suffix_from_row(points[pi])}")

    return "\n".join(lines) + "\n"


def format_report_compact(report: dict[str, Any], gametype_name: str = "ctf") -> str:
    return format_report_text(report, gametype_name)


def serialize_report(
    report: dict[str, Any], fmt: str, pretty: bool, gametype_name: str
) -> str | bytes:
    if fmt in ("qspawn", "json"):
        indent = 2 if pretty else None
        return json.dumps(report, indent=indent, ensure_ascii=False, separators=(",", ":") if not pretty else (", ", ": ")) + "\n"
    if fmt == "text":
        return format_report_text(report, gametype_name)
    if fmt == "debug":
        indent = 2 if pretty else None
        return json.dumps(report, indent=indent, ensure_ascii=False) + "\n"
    raise ValueError(f"unknown format {fmt!r}")


SpawnJob = tuple[str, Path, str | None, str]  # label, path, bsp_in_pk3, map_name


def collect_spawn_jobs(path: Path) -> dict[str, SpawnJob]:
    """All maps/*.bsp from a file or game folder (every BSP inside each pk3)."""
    path = path.resolve()
    if path.is_file():
        if path.suffix.lower() == ".bsp":
            name = path.stem.lower()
            return {name: (str(path), path, None, name)}
        if path.suffix.lower() == ".pk3":
            raw = collect_maps_from_pk3s([path])
            return {
                map_name: (label, pk3, inner, map_name)
                for map_name, (pk3, inner, label) in raw.items()
            }
        raise ValueError(f"not a .bsp or .pk3: {path}")
    if path.is_dir():
        return collect_batch_jobs([path])
    raise ValueError(f"not a file or directory: {path}")


def batch_write_spawn_reports(
    quake_dirs: list[Path],
    out_dir: Path,
    freeze: bool,
    skip_existing: bool,
    verbose: bool,
    pretty: bool,
) -> int:
    out_dir = out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    jobs = collect_batch_jobs(quake_dirs)
    if not jobs:
        print(
            "error: no maps/*.bsp found (on disk or inside .pk3) in given directories",
            file=sys.stderr,
        )
        return 1

    written = 0
    skipped = 0
    skipped_exist = 0
    skipped_empty = 0

    print(f"scanning {len(jobs)} unique maps -> {out_dir}", file=sys.stderr)

    for map_name in sorted(jobs.keys()):
        label, path, inner, _ = jobs[map_name]
        out_path = out_dir / f"{map_name}.qspawn"
        if skip_existing and out_path.is_file():
            skipped_exist += 1
            continue
        try:
            report = build_spawn_report(path, freeze, inner)
        except (ValueError, OSError) as exc:
            skipped += 1
            if verbose:
                print(f"skip {map_name} ({label}): {exc}", file=sys.stderr)
            continue

        if not report.get("points"):
            skipped_empty += 1
            if verbose:
                print(f"skip {map_name}: no spawn entities ({label})", file=sys.stderr)
            continue

        out_path = out_dir / f"{map_name}.qspawn"
        out_path.write_text(
            json.dumps(report, ensure_ascii=False, separators=(",", ":")) + "\n",
            encoding="utf-8",
        )
        written += 1
        n = len(report["points"])
        print(f"wrote {out_path.name} ({n} spawn ents) <- {label}", file=sys.stderr)

    print(
        f"done: {written} written, {skipped_exist} already present, "
        f"{skipped_empty} no spawns, {skipped} errors",
        file=sys.stderr,
    )
    return 0 if written or skipped_exist else 1


def pack_qsp1_binary(report: dict[str, Any]) -> bytes:
    """Binary QSP1 payload for cgame (cg_spawns.c)."""
    points = report["points"]
    modes = report["modes"]
    if not points:
        raise ValueError("no points")

    chunks = [b"QSP1", struct.pack("<HH", QSP1_VERSION, len(points))]
    for pt in points:
        ox, oy, oz = pt["o"]
        yaw = int(pt.get("y", 0))
        sf = int(pt.get("sf", 0))
        cnt = pt.get("n")
        cnt_b = -1 if cnt is None else int(cnt)
        if cnt_b < -128 or cnt_b > 127:
            cnt_b = -1
        role = ROLE_CODE.get(pt.get("r", ""), 0)
        team = TEAM_CODE.get(pt.get("t", TEAM_FREE), 0)
        chunks.append(
            struct.pack(
                "<fffhbbBB",
                float(ox),
                float(oy),
                float(oz),
                yaw,
                sf & 0xFF,
                cnt_b,
                role & 0xFF,
                team & 0xFF,
            )
        )

    for mode_name in MODE_ORDER:
        block = modes.get(mode_name, {})
        spawned = block.get("spawned") or []
        chunks.append(struct.pack("<H", len(spawned)))
        for idx in spawned:
            chunks.append(struct.pack("<H", int(idx)))

    return b"".join(chunks)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Parse Q3 BSP player spawn entities and mode pools"
    )
    parser.add_argument(
        "map",
        type=Path,
        nargs="?",
        default=None,
        help="BSP, pk3, or folder of pk3/bsp (default: testmaps/)",
    )
    parser.add_argument(
        "--bsp-in-pk3",
        metavar="PATH",
        help="BSP path inside .pk3 when name does not match pk3 stem",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Output file or directory (per-map files when directory)",
    )
    parser.add_argument(
        "--format",
        choices=("qspawn", "json", "text", "debug"),
        help="qspawn=compact JSON (default batch), text=QSP1 lines, debug=verbose",
    )
    parser.add_argument(
        "--gametype",
        choices=sorted(GAMETYPE_POOL_NAMES.values()),
        default="ctf",
        help="Gametype section for compact output (default: ctf)",
    )
    parser.add_argument(
        "--freeze",
        action="store_true",
        help="Apply g_freeze entity spawn filters",
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Indented JSON",
    )
    parser.add_argument(
        "--batch",
        action="store_true",
        help="Scan baseq3 + osp + C:/maps (Steam); use --pk3-dir to override",
    )
    parser.add_argument(
        "--pk3-dir",
        type=Path,
        action="append",
        dest="pk3_dirs",
        metavar="DIR",
        help="Game folder with *.pk3 and maps/*.bsp (repeatable)",
    )
    parser.add_argument(
        "--skip-existing",
        action="store_true",
        help="With --batch: do not overwrite existing .qspawn in output dir",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Log skipped maps during --batch",
    )
    args = parser.parse_args()

    if args.batch:
        pk3_dirs = args.pk3_dirs if args.pk3_dirs else DEFAULT_SPAWN_DIRS
        out_dir = args.output if args.output else DEFAULT_OUT_DIR
        rc = batch_write_spawn_reports(
            pk3_dirs,
            out_dir,
            args.freeze,
            args.skip_existing,
            args.verbose,
            args.pretty,
        )
        return rc

    map_arg = args.map if args.map is not None else REPO_ROOT / "testmaps"
    if not map_arg.exists():
        print(f"error: path not found: {map_arg}", file=sys.stderr)
        return 1

    try:
        job_map = collect_spawn_jobs(map_arg)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if not job_map:
        print(f"error: no maps/*.bsp in {map_arg}", file=sys.stderr)
        return 1

    multi = len(job_map) > 1
    out_path = args.output.resolve() if args.output else None
    if out_path and multi and out_path.suffix.lower() in (".json", ".qspawn", ".txt"):
        print("error: multiple inputs require -o to be a directory", file=sys.stderr)
        return 1

    fmt = args.format
    if fmt is None:
        if multi:
            fmt = "qspawn"
        elif out_path and out_path.suffix.lower() in (".json", ".qspawn"):
            fmt = "qspawn"
        elif out_path and out_path.suffix.lower() == ".txt":
            fmt = "text"
        else:
            fmt = "text" if not multi else "qspawn"

    build = build_spawn_report_debug if fmt == "debug" else build_spawn_report

    reports: list[dict[str, Any]] = []
    for map_name, (label, path, inner, _mn) in sorted(job_map.items()):
        try:
            reports.append(build(path, args.freeze, inner or args.bsp_in_pk3))
        except (ValueError, OSError) as exc:
            print(f"error: {map_name} ({label}): {exc}", file=sys.stderr)
            return 1

    if multi and fmt == "text":
        print("error: text format supports one map at a time", file=sys.stderr)
        return 1

    if len(reports) == 1:
        payload = serialize_report(reports[0], fmt, args.pretty, args.gametype)
        if out_path:
            if out_path.suffix.lower() in (".json", ".qspawn", ".txt") or not multi:
                out_path.parent.mkdir(parents=True, exist_ok=True)
                if isinstance(payload, bytes):
                    out_path.write_bytes(payload)
                else:
                    out_path.write_text(payload, encoding="utf-8")
                print(f"wrote {out_path}", file=sys.stderr)
            else:
                out_path.mkdir(parents=True, exist_ok=True)
                ext = ".qspawn" if fmt in ("qspawn", "json", "debug") else ".txt"
                dest = out_path / f"{reports[0]['map']}{ext}"
                dest.write_text(payload if isinstance(payload, str) else payload.decode(), encoding="utf-8")
                print(f"wrote {dest}", file=sys.stderr)
        else:
            sys.stdout.write(payload if isinstance(payload, str) else payload.decode())
        return 0

    # Multiple maps -> JSON array or per-file in output dir
    if out_path:
        out_path.mkdir(parents=True, exist_ok=True)
        for rep in reports:
            dest = out_path / f"{rep['map']}.qspawn"
            dest.write_text(
                json.dumps(
                    rep,
                    indent=2 if args.pretty else None,
                    ensure_ascii=False,
                    separators=(",", ":") if not args.pretty else (", ", ": "),
                )
                + "\n",
                encoding="utf-8",
            )
            print(f"wrote {dest}", file=sys.stderr)
        return 0

    sys.stdout.write(json.dumps(reports, indent=2 if args.pretty else None, ensure_ascii=False) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
