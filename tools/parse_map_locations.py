#!/usr/bin/env python3
"""
Quake 3 map locations for OSP2-BE (target_location / tinfo -> qloc64 / locs.q3lb).

Single script (Python 3): dev tools + pk3 folder scan for players.

Player / folder with pk3 (every maps/*.bsp in each pk3 -> path/locs/*.qloc64):

  cd D:/Quake3/baseq3
  python parse_map_locations.py

  python parse_map_locations.py D:/Quake3/baseq3

Development:

  python parse_map_locations.py map.bsp
  python parse_map_locations.py map.pk3 -o map.qloc64
  python parse_map_locations.py map.pk3 --locs
  python parse_map_locations.py --batch --pk3-dir .../baseq3 -o backup/locs
  python parse_map_locations.py --pack-q3lb backup/locs -o assets/locs/locs.q3lb

Ship in mod pk3: assets/locs/locs.q3lb and optional assets/locs/map.qloc64
"""

from __future__ import annotations

import argparse
import base64
import json
import struct
import sys
import zipfile
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_QLoc64_DIR = REPO_ROOT / "backup" / "locs"
DEFAULT_GAME_LOCS_DIR = REPO_ROOT / "assets" / "locs"
DEFAULT_BUNDLE_PATH = REPO_ROOT / "assets" / "locs" / "locs.q3lb"
DEFAULT_OUT_DIR = DEFAULT_QLoc64_DIR
DEFAULT_PK3_DIRS = [
    Path(r"c:\Program Files (x86)\Steam\steamapps\common\Quake 3 Arena\baseq3"),
    Path(r"c:\Program Files (x86)\Steam\steamapps\common\Quake 3 Arena\osp"),
]


import base64
import json
import struct
import sys
import zipfile
from pathlib import Path
from typing import Any

PACK_MAGIC = b"QLM1"

MAX_CLIENTS = 64
GT_FFA = 0
GT_TOURNAMENT = 1
GT_SINGLE_PLAYER = 2
GT_TEAM = 3
GT_CTF = 4

GAMETYPE_NAMES = {
    "ffa": GT_FFA,
    "tournament": GT_TOURNAMENT,
    "single": GT_SINGLE_PLAYER,
    "team": GT_TEAM,
    "ctf": GT_CTF,
}


def read_bsp_bytes(path: Path, bsp_in_pk3: str | None) -> tuple[bytes, str, str]:
    """Return (file bytes, source label, map name without extension)."""
    path = path.resolve()
    suffix = path.suffix.lower()

    if suffix == ".pk3":
        with zipfile.ZipFile(path) as zf:
            inner = _resolve_bsp_in_pk3(zf, path.stem.lower(), bsp_in_pk3)
            inner = inner.replace("\\", "/")
            data = zf.read(inner)
        map_name = Path(inner).stem.lower()
        return data, f"{path}!{inner}", map_name

    data = path.read_bytes()
    return data, str(path), path.stem.lower()


def entities_lump_from_bytes(data: bytes, source: str) -> str:
    if len(data) >= 4 and data[:4] != b"IBSP":
        return data.decode("latin-1", errors="replace")
    if len(data) < 8 + 15 * 8:
        raise ValueError(f"{source}: file too small for IBSP")
    magic, version = struct.unpack_from("<4si", data, 0)
    if magic != b"IBSP":
        raise ValueError(f"{source}: not IBSP (magic={magic!r})")
    if version not in (46, 47):
        print(f"warning: BSP version {version} (expected 46/47)", file=sys.stderr)

    off, length = struct.unpack_from("<ii", data, 8)
    if off < 0 or length < 0 or off + length > len(data):
        raise ValueError(f"{source}: invalid entities lump off={off} len={length}")
    raw = data[off : off + length]
    text = raw.decode("latin-1", errors="replace")
    return text.replace("\x00", "")


def _resolve_bsp_in_pk3(zf: zipfile.ZipFile, pk3_stem: str, bsp_in_pk3: str | None) -> str:
    if bsp_in_pk3:
        inner = bsp_in_pk3.replace("\\", "/")
        if inner not in zf.namelist():
            alt = [n for n in zf.namelist() if n.replace("\\", "/") == inner]
            if alt:
                return alt[0].replace("\\", "/")
        return inner

    index: dict[str, str] = {}
    for name in zf.namelist():
        norm = name.replace("\\", "/")
        low = norm.lower()
        if low.startswith("maps/") and low.endswith(".bsp"):
            index[low] = norm

    if not index:
        raise ValueError(f"{pk3_stem}.pk3: no maps/*.bsp inside")

    stem_hit = index.get(f"maps/{pk3_stem.lower()}.bsp")
    if stem_hit:
        return stem_hit

    if len(index) == 1:
        return next(iter(index.values()))

    hint = ", ".join(sorted(index.values())[:12])
    if len(index) > 12:
        hint += ", ..."
    raise ValueError(
        f"{pk3_stem}.pk3: multiple maps/*.bsp; use --bsp-in-pk3 PATH "
        f"(entries: {hint})"
    )


def _next_token(text: str, pos: int) -> tuple[str | None, int]:
    n = len(text)
    while pos < n and text[pos] in " \t\r\n":
        pos += 1
    if pos >= n:
        return None, pos
    if text[pos] == '"':
        pos += 1
        start = pos
        while pos < n and text[pos] != '"':
            if text[pos] == "\\" and pos + 1 < n:
                pos += 2
                continue
            pos += 1
        if pos >= n:
            raise ValueError("unterminated quoted string in entities lump")
        token = text[start:pos]
        pos += 1
        return token, pos
    start = pos
    while pos < n and text[pos] not in " \t\r\n":
        pos += 1
    return text[start:pos], pos


def parse_entity_lump(text: str) -> list[dict[str, str]]:
    entities: list[dict[str, str]] = []
    pos = 0
    n = len(text)

    while pos < n:
        while pos < n and text[pos] in " \t\r\n":
            pos += 1
        if pos >= n:
            break
        tok, pos = _next_token(text, pos)
        if tok is None or tok != "{":
            continue

        ent: dict[str, str] = {}
        while True:
            key, pos = _next_token(text, pos)
            if key is None:
                raise ValueError("unexpected EOF inside entity block")
            if key == "}":
                break
            val, pos = _next_token(text, pos)
            if val is None:
                raise ValueError(f"missing value for key {key!r}")
            ent[key] = val
        entities.append(ent)

    return entities


def _spawn_int(ent: dict[str, str], key: str, default: int = 0) -> int:
    if key not in ent:
        return default
    try:
        return int(ent[key])
    except ValueError:
        return default


def _gametype_matches(ent: dict[str, str], gametype: int) -> bool:
    value = ent.get("gametype")
    if not value:
        return True
    names = ("ffa", "tournament", "single", "team", "ctf")
    if gametype < 0 or gametype >= len(names):
        return True
    needle = names[gametype]
    return needle in value.lower()


def entity_would_spawn(ent: dict[str, str], gametype: int, freeze: bool) -> bool:
    classname = ent.get("classname", "")

    if gametype == GT_SINGLE_PLAYER and _spawn_int(ent, "notsingle"):
        return False

    if gametype >= GT_TEAM:
        if not freeze:
            if _spawn_int(ent, "notteam"):
                return False
        else:
            if _spawn_int(ent, "notteam") and classname != "info_player_deathmatch":
                return False
    else:
        if _spawn_int(ent, "notfree"):
            return False

    if _spawn_int(ent, "notq3a"):
        return False

    if not _gametype_matches(ent, gametype):
        return False

    return True


def parse_origin(ent: dict[str, str]) -> tuple[float, float, float] | None:
    origin = ent.get("origin")
    if not origin:
        return None
    parts = origin.split()
    if len(parts) != 3:
        return None
    try:
        return float(parts[0]), float(parts[1]), float(parts[2])
    except ValueError:
        return None


def simulate_gentity_slots(
    entities: list[dict[str, str]], gametype: int, freeze: bool
) -> list[dict[str, Any]]:
    """One G_Spawn() per entity block after worldspawn (fresh map load)."""
    if not entities:
        return []
    if entities[0].get("classname") != "worldspawn":
        print("warning: first entity is not worldspawn", file=sys.stderr)

    slots: list[dict[str, Any]] = []
    gentity_index = MAX_CLIENTS

    for ent in entities[1:]:
        slot: dict[str, Any] = {
            "gentity_index": gentity_index,
            "spawned": entity_would_spawn(ent, gametype, freeze),
            "keys": ent,
        }
        if slot["spawned"]:
            slot["classname"] = ent.get("classname", "")
        else:
            slot["classname"] = "freed"
        slots.append(slot)
        gentity_index += 1

    return slots


def assign_location_ids(slots: list[dict[str, Any]], freeze: bool) -> list[dict[str, Any]]:
    """Mirror target_location_linkup (g_target.c)."""
    modified = False
    if freeze:
        for slot in slots:
            if slot.get("classname") == "target_location":
                count = _spawn_int(slot["keys"], "count", 0)
                if count != 255:
                    modified = True
                    break

    locations: list[dict[str, Any]] = []
    n = 1
    for slot in slots:
        if slot.get("classname") != "target_location":
            continue
        ent = slot["keys"]
        count = _spawn_int(ent, "count", 0)
        if freeze and count == 255:
            if modified:
                continue
            count = 0

        origin = parse_origin(ent)
        loc = {
            "id": n,
            "cs_index": n,
            "name": ent.get("message", ""),
            "count_color": count,
            "origin": list(origin) if origin else None,
            "gentity_index": slot["gentity_index"],
        }
        locations.append(loc)
        n += 1

    return locations


def location_at_point(
    locations: list[dict[str, Any]], x: float, y: float, z: float
) -> dict[str, Any] | None:
    """Nearest target_location by distance (no PVS — offline approximation)."""
    best: dict[str, Any] | None = None
    best_len = 3 * 8192.0 * 8192.0
    for loc in locations:
        o = loc.get("origin")
        if not o:
            continue
        dx = x - o[0]
        dy = y - o[1]
        dz = z - o[2]
        dist_sq = dx * dx + dy * dy + dz * dz
        if dist_sq <= best_len:
            best_len = dist_sq
            best = loc
    return best


def build_report(
    map_path: Path,
    gametype: int,
    freeze: bool,
    at: tuple[float, float, float] | None,
    bsp_in_pk3: str | None = None,
) -> dict[str, Any]:
    bsp_bytes, source, map_name = read_bsp_bytes(map_path, bsp_in_pk3)
    lump = entities_lump_from_bytes(bsp_bytes, source)
    entities = parse_entity_lump(lump)
    slots = simulate_gentity_slots(entities, gametype, freeze)
    locations = assign_location_ids(slots, freeze)

    report: dict[str, Any] = {
        "map": map_name,
        "bsp": source,
        "gametype": gametype,
        "freeze": freeze,
        "location_count": len(locations),
        "cs_locations_unknown_index": 0,
        "locations": locations,
    }

    if at is not None:
        hit = location_at_point(locations, at[0], at[1], at[2])
        report["query"] = {
            "at": list(at),
            "location_id": hit["id"] if hit else 0,
            "name": hit["name"] if hit else "unknown",
        }

    return report


def _output_is_explicit_file(path: Path) -> bool:
    ext = path.suffix.lower()
    if ext in (".qloc64", ".qloc", ".json", ".b64"):
        return True
    return path.name.lower().endswith(".verbose.txt")


def resolve_single_map_output_path(
    report: dict[str, Any],
    *,
    output: Path | None,
    locs_dir: Path | None,
) -> Path | None:
    """File to write, or None for stdout."""
    map_name = str(report["map"]).lower()

    if output is not None and _output_is_explicit_file(output):
        return output.resolve()

    if locs_dir is not None:
        base = locs_dir.resolve()
        base.mkdir(parents=True, exist_ok=True)
        return base / f"{map_name}.qloc64"

    if output is None:
        return None

    out = output.resolve()
    if out.is_dir() or not out.suffix:
        out.mkdir(parents=True, exist_ok=True)
        return out / f"{map_name}.qloc64"
    return out


def _txt_number(value: float | int) -> str:
    if isinstance(value, float) and value.is_integer():
        return str(int(value))
    if isinstance(value, int):
        return str(value)
    text = f"{value:g}"
    return text


def _txt_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def format_report_compact(report: dict[str, Any]) -> str:
    """One header line + one row per zone: id x y z color name…"""
    lines = [f"QLM1 {report['map']} {report['location_count']}"]
    for loc in report["locations"]:
        o = loc.get("origin") or [0, 0, 0]
        name = loc["name"].replace("\n", " ").replace("\r", " ")
        lines.append(
            f"{loc['id']} {_txt_number(o[0])} {_txt_number(o[1])} {_txt_number(o[2])} "
            f"{loc['count_color']} {name}"
        )
    if "query" in report:
        q = report["query"]
        at = q["at"]
        lines.append(
            f"? {_txt_number(at[0])} {_txt_number(at[1])} {_txt_number(at[2])} "
            f"{q['location_id']} {q['name']}"
        )
    return "\n".join(lines) + "\n"


def parse_report_compact(text: str) -> dict[str, Any]:
    """Parse QLM1 compact text (same as format_report_compact output)."""
    lines = [ln.strip() for ln in text.splitlines() if ln.strip() and not ln.strip().startswith("#")]
    if not lines or not lines[0].startswith("QLM1 "):
        raise ValueError("expected QLM1 compact header")
    parts = lines[0].split()
    if len(parts) < 3:
        raise ValueError("bad QLM1 header")
    map_name = parts[1]
    try:
        expect = int(parts[2])
    except ValueError as exc:
        raise ValueError("bad location_count in QLM1 header") from exc

    locations: list[dict[str, Any]] = []
    for line in lines[1:]:
        if line.startswith("?"):
            continue
        fields = line.split(None, 5)
        if len(fields) < 6:
            continue
        loc_id = int(fields[0])
        ox, oy, oz = float(fields[1]), float(fields[2]), float(fields[3])
        color = int(fields[4])
        name = fields[5]
        locations.append(
            {
                "id": loc_id,
                "cs_index": loc_id,
                "name": name,
                "count_color": color,
                "origin": [ox, oy, oz],
            }
        )

    if expect != len(locations):
        raise ValueError(f"QLM1 header count {expect} != parsed {len(locations)}")

    return {
        "map": map_name,
        "location_count": len(locations),
        "locations": locations,
    }


def pack_locations_binary(report: dict[str, Any]) -> bytes:
    map_name = report["map"].encode("utf-8")
    if len(map_name) > 255:
        raise ValueError("map name too long for packed format")
    chunks = [PACK_MAGIC, struct.pack("B", len(map_name)), map_name]
    locs = report["locations"]
    chunks.append(struct.pack("B", len(locs)))
    for loc in locs:
        o = loc.get("origin") or [0.0, 0.0, 0.0]
        name = loc["name"].encode("utf-8")
        if len(name) > 255:
            raise ValueError(f"location name too long: {loc['id']}")
        chunks.append(
            struct.pack(
                "<BfffBB",
                loc["id"],
                float(o[0]),
                float(o[1]),
                float(o[2]),
                loc["count_color"] & 0xFF,
                len(name),
            )
        )
        chunks.append(name)
    return b"".join(chunks)


def unpack_locations_binary(data: bytes) -> dict[str, Any]:
    raw = data
    if len(raw) < 6 or raw[:4] != PACK_MAGIC:
        raise ValueError("not a QLM1 packed location file")
    pos = 4
    (map_len,) = struct.unpack_from("B", raw, pos)
    pos += 1
    map_name = raw[pos : pos + map_len].decode("utf-8")
    pos += map_len
    (count,) = struct.unpack_from("B", raw, pos)
    pos += 1
    locations: list[dict[str, Any]] = []
    for _ in range(count):
        loc_id, ox, oy, oz, color, name_len = struct.unpack_from("<BfffBB", raw, pos)
        pos += struct.calcsize("<BfffBB")
        name = raw[pos : pos + name_len].decode("utf-8")
        pos += name_len
        locations.append(
            {
                "id": loc_id,
                "cs_index": loc_id,
                "name": name,
                "count_color": color,
                "origin": [ox, oy, oz],
            }
        )
    return {
        "map": map_name,
        "location_count": len(locations),
        "locations": locations,
    }


def format_qloc64(data: bytes) -> str:
    return "QLZ1 " + base64.b64encode(data, altchars=b"-_").decode("ascii") + "\n"


def parse_qloc64(text: str) -> bytes:
    line = text.strip().splitlines()[0]
    if not line.startswith("QLZ1 "):
        raise ValueError("expected QLZ1 base64 line")
    return base64.b64decode(line[5:], altchars=b"-_")


def iter_maps_in_pk3(pk3_path: Path) -> list[tuple[str, str]]:
    """Return (bsp_stem, inner_path) for each maps/*.bsp (stem may differ from pk3 name)."""
    out: list[tuple[str, str]] = []
    with zipfile.ZipFile(pk3_path) as zf:
        for name in zf.namelist():
            norm = name.replace("\\", "/")
            low = norm.lower()
            if "__macosx" in low or not low.startswith("maps/") or not low.endswith(".bsp"):
                continue
            map_name = Path(norm).stem.lower()
            if not map_name:
                continue
            out.append((map_name, norm))
    out.sort(key=lambda item: item[0])
    return out


def expand_pk3_sources(path: Path) -> list[Path]:
    path = path.resolve()
    if path.is_file():
        if path.suffix.lower() != ".pk3":
            raise ValueError(f"not a .pk3 file: {path}")
        return [path]
    if path.is_dir():
        pk3s = sorted(path.glob("*.pk3"))
        if not pk3s:
            raise ValueError(f"no *.pk3 in directory: {path}")
        return pk3s
    raise ValueError(f"not a file or directory: {path}")


def default_locs_dir(input_path: Path) -> Path:
    """<folder>/locs or <pk3-parent>/locs."""
    input_path = input_path.resolve()
    if input_path.is_file():
        return input_path.parent / "locs"
    return input_path / "locs"


def collect_maps_from_pk3s(pk3_paths: list[Path]) -> dict[str, tuple[Path, str, str]]:
    """
    map_name -> (pk3_path, inner_bsp_path, label).
    Later pk3 wins if the same map name appears twice.
    """
    jobs: dict[str, tuple[Path, str, str]] = {}
    for pk3_path in pk3_paths:
        try:
            entries = iter_maps_in_pk3(pk3_path)
        except (OSError, zipfile.BadZipFile) as exc:
            print(f"warn: skip {pk3_path.name}: {exc}", file=sys.stderr)
            continue
        if not entries:
            print(f"warn: {pk3_path.name}: no maps/*.bsp inside", file=sys.stderr)
            continue
        for map_name, inner in entries:
            label = f"{pk3_path.name}!{inner}"
            jobs[map_name] = (pk3_path, inner, label)
    return jobs


def run_batch(
    sources: list[Path],
    out_dir: Path,
    gametype: int,
    freeze: bool,
    skip_existing: bool,
) -> int:
    out_dir = out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    jobs = collect_maps_from_pk3s(sources)
    if not jobs:
        print("error: no maps/*.bsp found in given .pk3 file(s)", file=sys.stderr)
        return 1

    written = 0
    skipped = 0
    skipped_exist = 0

    print(f"{len(jobs)} map(s) from {len(sources)} pk3 -> {out_dir}", file=sys.stderr)

    for map_name in sorted(jobs.keys()):
        pk3_path, inner, label = jobs[map_name]
        out_path = out_dir / f"{map_name}.qloc64"
        if skip_existing and out_path.is_file():
            skipped_exist += 1
            continue
        try:
            report = build_report(pk3_path, gametype, freeze, None, inner)
        except (ValueError, OSError) as exc:
            skipped += 1
            print(f"skip {map_name} ({label}): {exc}", file=sys.stderr)
            continue

        if report["location_count"] <= 0:
            skipped += 1
            print(f"skip {map_name}: no target_location in BSP ({label})", file=sys.stderr)
            continue

        out_path.write_text(
            format_qloc64(pack_locations_binary(report)),
            encoding="ascii",
        )
        written += 1
        print(
            f"wrote {out_path.name} ({report['location_count']} zones) <- {label}",
            file=sys.stderr,
        )

    print(
        f"done: {written} written, {skipped_exist} already present, {skipped} skipped",
        file=sys.stderr,
    )
    if written:
        print("install: locs/<map>.qloc64 in mod pk3 or game locs/ folder", file=sys.stderr)
    return 0 if written or skipped_exist else 1


def pack_q3lb_from_qloc64_dir(src_dir: Path, out_path: Path) -> int:
    """Pack all *.qloc64 in src_dir into one Q3LB binary (cgame locs/locs.q3lb)."""
    entries: list[tuple[str, bytes]] = []
    for path in sorted(src_dir.glob("*.qloc64")):
        map_name = path.stem.lower()
        if not map_name:
            continue
        try:
            qlm = parse_qloc64(path.read_text(encoding="ascii"))
        except (ValueError, OSError) as exc:
            print(f"warn: skip {path.name}: {exc}", file=sys.stderr)
            continue
        if len(qlm) < 6 or qlm[:4] != PACK_MAGIC:
            print(f"warn: skip {path.name}: not QLM1", file=sys.stderr)
            continue
        entries.append((map_name, qlm))

    if not entries:
        print("error: no valid qloc64 files", file=sys.stderr)
        return 1

    chunks = [b"Q3LB", struct.pack("<H", len(entries))]
    for map_name, qlm in entries:
        name_b = map_name.encode("utf-8")
        if len(name_b) > 255:
            print(f"warn: skip {map_name}: name too long", file=sys.stderr)
            continue
        chunks.append(struct.pack("<B", len(name_b)))
        chunks.append(name_b)
        chunks.append(struct.pack("<I", len(qlm)))
        chunks.append(qlm)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(b"".join(chunks))
    print(
        f"wrote {out_path} ({len(entries)} maps, {out_path.stat().st_size} bytes)",
        file=sys.stderr,
    )
    return 0


def verify_q3lb_lookup(bundle_path: Path, map_name: str) -> dict[str, Any] | None:
    raw = bundle_path.read_bytes()
    if len(raw) < 7 or raw[:4] != b"Q3LB":
        raise ValueError("not Q3LB")
    pos = 4
    (n_maps,) = struct.unpack_from("<H", raw, pos)
    pos += 2
    needle = map_name.lower()
    for _ in range(n_maps):
        (name_len,) = struct.unpack_from("<B", raw, pos)
        pos += 1
        name = raw[pos : pos + name_len].decode("utf-8")
        pos += name_len
        (qlm_len,) = struct.unpack_from("<I", raw, pos)
        pos += 4
        blob = raw[pos : pos + qlm_len]
        pos += qlm_len
        if name == needle:
            return unpack_locations_binary(blob)
    return None


def format_report_verbose(report: dict[str, Any]) -> str:
    lines: list[str] = []
    lines.append(f"map {_txt_string(report['map'])}")
    lines.append(f"source {_txt_string(report['bsp'])}")
    lines.append(f"gametype {_txt_number(report['gametype'])}")
    lines.append(f"freeze {1 if report['freeze'] else 0}")
    lines.append(f"location_count {_txt_number(report['location_count'])}")
    lines.append(f"cs_unknown_index {_txt_number(report['cs_locations_unknown_index'])}")
    lines.append("")

    for loc in report["locations"]:
        lines.append("location {")
        lines.append(f"  id {_txt_number(loc['id'])}")
        lines.append(f"  cs_index {_txt_number(loc['cs_index'])}")
        lines.append(f"  name {_txt_string(loc['name'])}")
        lines.append(f"  count_color {_txt_number(loc['count_color'])}")
        o = loc.get("origin")
        if o:
            lines.append(
                f"  origin {_txt_number(o[0])} {_txt_number(o[1])} {_txt_number(o[2])}"
            )
        else:
            lines.append("  origin 0 0 0")
        lines.append(f"  gentity_index {_txt_number(loc['gentity_index'])}")
        lines.append("}")
        lines.append("")

    if "query" in report:
        q = report["query"]
        lines.append("query {")
        at = q["at"]
        lines.append(
            f"  at {_txt_number(at[0])} {_txt_number(at[1])} {_txt_number(at[2])}"
        )
        lines.append(f"  location_id {_txt_number(q['location_id'])}")
        lines.append(f"  name {_txt_string(q['name'])}")
        lines.append("}")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def serialize_report(
    report: dict[str, Any], fmt: str, pretty_json: bool
) -> str | bytes:
    if fmt == "compact":
        return format_report_compact(report)
    if fmt == "verbose":
        return format_report_verbose(report)
    if fmt == "qloc":
        return pack_locations_binary(report)
    if fmt == "qloc64":
        return format_qloc64(pack_locations_binary(report))
    indent = 2 if pretty_json else None
    return json.dumps(report, indent=indent, ensure_ascii=False) + "\n"




def collect_batch_jobs(
    quake_dirs: list[Path],
) -> dict[str, tuple[str, Path, str | None, str]]:
    """
    Collect BSP sources per map name. Later directories and pk3 beat earlier loose maps/.
    Value: (source_label, path, bsp_in_pk3 or None, map_name).
    """
    by_map: dict[str, tuple[str, Path, str | None, str]] = {}

    for directory in quake_dirs:
        directory = directory.resolve()
        if not directory.is_dir():
            print(f"warn: not a directory: {directory}", file=sys.stderr)
            continue

        maps_dir = directory / "maps"
        if maps_dir.is_dir():
            for bsp_path in sorted(maps_dir.glob("*.bsp")):
                map_name = bsp_path.stem.lower()
                if not map_name:
                    continue
                label = f"{directory.name}/maps/{bsp_path.name}"
                by_map[map_name] = (label, bsp_path.resolve(), None, map_name)

        for pk3_path in sorted(directory.glob("*.pk3")):
            pk3_path = pk3_path.resolve()
            try:
                entries = iter_maps_in_pk3(pk3_path)
            except (OSError, zipfile.BadZipFile) as exc:
                print(f"warn: {pk3_path.name}: {exc}", file=sys.stderr)
                continue
            for map_name, inner in entries:
                label = f"{pk3_path.name}!{inner}"
                by_map[map_name] = (label, pk3_path, inner, map_name)

    return by_map


def batch_pk3_to_qloc64(
    pk3_dirs: list[Path],
    out_dir: Path,
    gametype: int,
    freeze: bool,
    verbose: bool,
    skip_existing: bool,
) -> int:
    out_dir = out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    jobs = collect_batch_jobs(pk3_dirs)
    if not jobs:
        print(
            "error: no maps/*.bsp found (on disk or inside .pk3) in given directories",
            file=sys.stderr,
        )
        return 1

    written = 0
    skipped = 0
    skipped_exist = 0

    print(f"scanning {len(jobs)} unique maps -> {out_dir}", file=sys.stderr)

    for map_name in sorted(jobs.keys()):
        label, path, inner, _ = jobs[map_name]
        out_path = out_dir / f"{map_name}.qloc64"
        if skip_existing and out_path.is_file():
            skipped_exist += 1
            continue
        try:
            report = build_report(path, gametype, freeze, None, inner)
        except (ValueError, OSError) as exc:
            skipped += 1
            if verbose:
                print(f"skip {map_name} ({label}): {exc}", file=sys.stderr)
            continue

        if report["location_count"] <= 0:
            skipped += 1
            if verbose:
                print(f"skip {map_name}: no target_location ({label})", file=sys.stderr)
            continue

        payload = format_qloc64(pack_locations_binary(report))
        out_path.write_text(payload, encoding="ascii")
        written += 1
        print(
            f"wrote {out_path.name} ({report['location_count']} locs) <- {label}",
            file=sys.stderr,
        )

    print(
        f"done: {written} qloc64 written, {skipped_exist} already present, {skipped} skipped",
        file=sys.stderr,
    )
    return 0



def single_map_mode(args: argparse.Namespace) -> bool:
    """One BSP/pk3 via build_report; otherwise scan all pk3 in folder."""
    if args.map is None:
        return False
    if args.map.is_dir():
        return False
    suffix = args.map.suffix.lower()
    if suffix in (".bsp", ".txt"):
        return True
    if args.bsp_in_pk3 or args.at or args.locs is not None or args.format:
        return True
    if args.output is not None and _output_is_explicit_file(args.output):
        return True
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Parse Q3 BSP target_location / tinfo IDs")
    parser.add_argument(
        "map",
        type=Path,
        nargs="?",
        default=None,
        help="BSP/pk3, folder to scan, or omit for pk3 scan in current directory",
    )
    parser.add_argument(
        "--decode",
        type=Path,
        metavar="FILE",
        help="Unpack .qloc / .qloc64 to compact text on stdout",
    )
    parser.add_argument(
        "--bsp-in-pk3",
        metavar="PATH",
        help="Override BSP path inside .pk3 (default: maps/<name>.bsp)",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Write output (.txt=compact, .qloc=QLM1 binary, .qloc64=one-line b64, .json)",
    )
    parser.add_argument(
        "--format",
        choices=("compact", "verbose", "qloc", "qloc64", "json"),
        help="Output format (default: from -o extension, else compact)",
    )
    parser.add_argument(
        "--gametype",
        choices=sorted(GAMETYPE_NAMES.keys()),
        default="ctf",
        help="Spawn filters like G_SpawnGEntityFromSpawnVars (default: ctf)",
    )
    parser.add_argument(
        "--freeze",
        action="store_true",
        help="Apply g_freeze target_location rules (count 255 markers)",
    )
    parser.add_argument(
        "--at",
        nargs=3,
        type=float,
        metavar=("X", "Y", "Z"),
        help="Test which location ID wins at world coordinates",
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Indented JSON (only with --format json)",
    )
    parser.add_argument(
        "--batch",
        action="store_true",
        help="Scan Quake dirs: maps/*.bsp + every maps/*.bsp inside *.pk3 (see --pk3-dir)",
    )
    parser.add_argument(
        "--pk3-dir",
        type=Path,
        action="append",
        dest="pk3_dirs",
        metavar="DIR",
        help="Folder with .pk3 (repeatable). With --batch and no dirs, uses Steam baseq3 + osp",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Log skipped maps during --batch",
    )
    parser.add_argument(
        "--skip-existing",
        action="store_true",
        help="With --batch: do not overwrite existing .qloc64 in output dir",
    )
    parser.add_argument(
        "--locs",
        nargs="?",
        const="",
        metavar="DIR",
        help=(
            "Write <map>.qloc64 for per-map mod override (default dir: assets/locs). "
            "Cgame loads locs/<map>.qloc64 before locs.q3lb."
        ),
    )
    parser.add_argument(
        "--pack-q3lb",
        type=Path,
        nargs="?",
        const=DEFAULT_QLoc64_DIR,
        metavar="SRC_DIR",
        help=f"Pack SRC_DIR/*.qloc64 into locs.q3lb (Q3 locs bundle, default src: {DEFAULT_QLoc64_DIR.name})",
    )
    parser.add_argument(
        "--verify-map",
        metavar="NAME",
        help="With --pack-q3lb output: print compact locs for map from bundle",
    )
    args = parser.parse_args()

    gt = GAMETYPE_NAMES[args.gametype]

    if args.pack_q3lb is not None:
        src = args.pack_q3lb.resolve()
        if src.is_file() and src.suffix.lower() == ".qloc64":
            src = src.parent
        out = (
            args.output.resolve()
            if args.output
            else DEFAULT_BUNDLE_PATH.resolve()
        )
        rc = pack_q3lb_from_qloc64_dir(src, out)
        if rc != 0:
            return rc
        if args.verify_map:
            report = verify_q3lb_lookup(out, args.verify_map)
            if not report:
                print(f"error: map {args.verify_map!r} not in bundle", file=sys.stderr)
                return 1
            sys.stdout.write(format_report_compact(report))
        return 0

    if args.batch:
        pk3_dirs = args.pk3_dirs if args.pk3_dirs else DEFAULT_PK3_DIRS
        out_dir = args.output if args.output else DEFAULT_OUT_DIR
        return batch_pk3_to_qloc64(
            pk3_dirs, out_dir, gt, args.freeze, args.verbose, args.skip_existing
        )

    if args.decode:
        if not args.decode.is_file():
            print(f"error: not a file: {args.decode}", file=sys.stderr)
            return 1
        raw = args.decode.read_bytes()
        ext = args.decode.suffix.lower()
        try:
            if ext == ".qloc64" or raw[:4] == b"QLZ1":
                blob = parse_qloc64(raw.decode("ascii", errors="strict"))
            else:
                blob = raw
            report = unpack_locations_binary(blob)
        except (ValueError, OSError, struct.error) as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        sys.stdout.write(format_report_compact(report))
        return 0

    if not single_map_mode(args):
        input_path = (args.map if args.map is not None else Path.cwd()).resolve()
        try:
            pk3_list = expand_pk3_sources(input_path)
        except ValueError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        out_dir = args.output if args.output else default_locs_dir(input_path)
        return run_batch(pk3_list, out_dir, gt, args.freeze, args.skip_existing)

    if not args.map or not args.map.is_file():
        print(
            "error: map file required (or omit path / pass a folder to scan .pk3)",
            file=sys.stderr,
        )
        return 1

    at = tuple(args.at) if args.at else None

    try:
        if args.map.suffix.lower() == ".txt":
            txt = args.map.read_text(encoding="utf-8", errors="replace")
            if txt.lstrip().startswith("QLM1 "):
                report = parse_report_compact(txt)
            else:
                report = build_report(args.map, gt, args.freeze, at, args.bsp_in_pk3)
        else:
            report = build_report(args.map, gt, args.freeze, at, args.bsp_in_pk3)
    except (ValueError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    locs_dir: Path | None = None
    if args.locs is not None:
        locs_dir = DEFAULT_GAME_LOCS_DIR if args.locs == "" else Path(args.locs)

    out_path = resolve_single_map_output_path(
        report, output=args.output, locs_dir=locs_dir
    )

    fmt = args.format
    if fmt is None and out_path:
        ext = out_path.suffix.lower()
        if ext == ".json":
            fmt = "json"
        elif ext == ".qloc":
            fmt = "qloc"
        elif ext in (".qloc64", ".b64"):
            fmt = "qloc64"
        elif out_path.name.lower().endswith(".verbose.txt"):
            fmt = "verbose"
        else:
            fmt = "compact"
    if fmt is None:
        fmt = "compact"

    payload = serialize_report(report, fmt, args.pretty)

    if out_path:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(payload, bytes):
            out_path.write_bytes(payload)
        else:
            out_path.write_text(payload, encoding="utf-8")
        size = out_path.stat().st_size
        print(
            f"wrote {out_path} ({report['location_count']} locations, {size} bytes)",
            file=sys.stderr,
        )
        if locs_dir is not None or out_path.parent == DEFAULT_GAME_LOCS_DIR.resolve():
            print(
                f"mod: locs/{report['map'].lower()}.qloc64 (pk3: assets/locs/)",
                file=sys.stderr,
            )
    else:
        if isinstance(payload, bytes):
            sys.stdout.buffer.write(payload)
        else:
            sys.stdout.write(payload)

    if at and "query" in report:
        q = report["query"]
        print(
            f"at {q['at']}: location_id={q['location_id']} ({q['name']})",
            file=sys.stderr,
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
