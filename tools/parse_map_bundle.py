#!/usr/bin/env python3
r"""
parse_map_bundle.py — mapdata для OSP2-BE (спавны QSP1, shadow-пикапы QITM1, локации QLM1, tpexit QTX1).

Как клиент грузит данные (cgame)
--------------------------------
1) mapdata/mapdata.q3lb — общий архив, запись по имени карты (без .bsp).
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
  python parse_map_bundle.py --batch -o backup/mapdata
      JSON .qmap на карту (редактирование)

  python parse_map_bundle.py --batch --pack-q3lb -o assets/mapdata/mapdata.q3lb
      JSON в backup/mapdata, затем сборка mapdata.q3lb

  python parse_map_bundle.py --pack-q3lb backup/mapdata -o assets/mapdata/mapdata.q3lb
      Пересобрать q3lb из уже готовых .qmap/.json

Прочее
------
  --freeze          правила freeze для спавнов/локаций
  --loc-gametype    gametype для target_location (default: ctf)
  --pk3-dir DIR     доп. папки с pk3 (можно несколько; идут после стандартных)

Скан по умолчанию (--batch): baseq3 → osp → C:\\maps (позже перекрывает раньше).
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


# --- FinishSpawningItem drop (metarena code/game/g_items.c: trap_Trace, MASK_SOLID) ---

ITEM_RADIUS = 15.0
DROP_DEPTH = 4096.0
SURFACE_CLIP_EPSILON = 0.03125
CONTENTS_SOLID = 0x1
DROP_SPATIAL_CELL = 128.0
MAX_BRUSH_INDEX_SPAN = 2048.0

MST_PLANAR = 1
MST_PATCH = 2
MST_TRIANGLE_STRIP = 3
DSURFACE_SIZE = 104
DRAWVERT_SIZE = 44
DNODE_SIZE = 36
DLEAF_SIZE = 48

BSP_LUMP_SHADERS = 1
BSP_LUMP_PLANES = 2
BSP_LUMP_NODES = 3
BSP_LUMP_LEAFS = 4
BSP_LUMP_LEAFBRUSHES = 6
BSP_LUMP_DRAWVERTS = 10
BSP_LUMP_DRAWINDEXES = 11
BSP_LUMP_BRUSHES = 8
BSP_LUMP_BRUSHSIDES = 9
BSP_LUMP_SURFACES = 13


def _bsp_lump(data: bytes, lump_id: int) -> bytes:
    off = 8 + lump_id * 8
    fileofs, filelen = struct.unpack_from("<ii", data, off)
    if fileofs < 0 or filelen < 0 or fileofs + filelen > len(data):
        return b""
    return data[fileofs : fileofs + filelen]


def _plane_signbits(nx: float, ny: float, nz: float) -> int:
    bits = 0
    if nx < 0:
        bits |= 1
    if ny < 0:
        bits |= 2
    if nz < 0:
        bits |= 4
    return bits


def _build_trace_offsets(mins: tuple[float, float, float], maxs: tuple[float, float, float]) -> list[tuple[float, float, float]]:
    sx0, sy0, sz0 = mins
    sx1, sy1, sz1 = maxs
    return [
        (sx0, sy0, sz0),
        (sx1, sy0, sz0),
        (sx0, sy1, sz0),
        (sx1, sy1, sz0),
        (sx0, sy0, sz1),
        (sx1, sy0, sz1),
        (sx0, sy1, sz1),
        (sx1, sy1, sz1),
    ]


def _z_on_triangle_xy(
    px: float,
    py: float,
    a: tuple[float, float, float],
    b: tuple[float, float, float],
    c: tuple[float, float, float],
) -> float | None:
    v0, v1, v2 = a, b, c
    denom = (v1[1] - v2[1]) * (v0[0] - v2[0]) + (v2[0] - v1[0]) * (v0[1] - v2[1])
    if abs(denom) < 1e-8:
        return None
    w0 = ((v1[1] - v2[1]) * (px - v2[0]) + (v2[0] - v1[0]) * (py - v2[1])) / denom
    w1 = ((v2[1] - v0[1]) * (px - v2[0]) + (v0[0] - v2[0]) * (py - v2[1])) / denom
    w2 = 1.0 - w0 - w1
    if w0 < -0.001 or w1 < -0.001 or w2 < -0.001:
        return None
    return w0 * v0[2] + w1 * v1[2] + w2 * v2[2]


class _SpatialIndex:
    __slots__ = ("_cells", "_cell")

    def __init__(self, cell_size: float) -> None:
        self._cell = cell_size
        self._cells: dict[tuple[int, int], list[int]] = {}

    def _span(self, a: float, b: float) -> range:
        lo = a if a < b else b
        hi = b if b > a else a
        c = self._cell
        return range(int(math.floor(lo / c)), int(math.floor(hi / c)) + 1)

    def add_rect(self, x0: float, y0: float, x1: float, y1: float, item_id: int) -> None:
        cells = self._cells
        for ix in self._span(x0, x1):
            for iy in self._span(y0, y1):
                key = (ix, iy)
                bucket = cells.get(key)
                if bucket is None:
                    cells[key] = [item_id]
                elif bucket[-1] != item_id:
                    bucket.append(item_id)

    def query_into(self, x: float, y: float, radius: float, out: set[int]) -> None:
        cells = self._cells
        for ix in self._span(x - radius, x + radius):
            for iy in self._span(y - radius, y + radius):
                bucket = cells.get((ix, iy))
                if bucket:
                    out.update(bucket)


class _ItemDropCollision:
    __slots__ = (
        "brushes",
        "brush_solid",
        "brush_sides",
        "planes",
        "patch_tris",
        "_brush_index",
        "_patch_index",
        "_trace_offsets",
        "_brush_scratch",
        "_patch_scratch",
        "_origin_cache",
        "_all_solid_brushes",
        "_wide_brushes",
    )

    def __init__(self, bsp: bytes) -> None:
        self.brushes: list[tuple[int, int, int]] = []
        self.brush_solid: list[bool] = []
        self.brush_sides: list[tuple[int, int]] = []
        self.planes: list[tuple[float, float, float, float, int]] = []
        self.patch_tris: list[
            tuple[
                tuple[float, float, float],
                tuple[float, float, float],
                tuple[float, float, float],
                float,
                float,
                float,
                float,
            ]
        ] = []
        self._brush_index = _SpatialIndex(DROP_SPATIAL_CELL)
        self._patch_index = _SpatialIndex(DROP_SPATIAL_CELL)
        r = ITEM_RADIUS
        self._trace_offsets = _build_trace_offsets((-r, -r, -r), (r, r, r))
        self._brush_scratch: set[int] = set()
        self._patch_scratch: set[int] = set()
        self._origin_cache: dict[tuple[int, int, int], tuple[float, float, float]] = {}
        self._all_solid_brushes: list[int] = []
        self._wide_brushes: list[int] = []
        self._parse(bsp)

    def _push_solid_mesh_tri(
        self,
        t0: tuple[float, float, float],
        t1: tuple[float, float, float],
        t2: tuple[float, float, float],
    ) -> None:
        min_x = min(t0[0], t1[0], t2[0])
        max_x = max(t0[0], t1[0], t2[0])
        min_y = min(t0[1], t1[1], t2[1])
        max_y = max(t0[1], t1[1], t2[1])
        ti = len(self.patch_tris)
        self.patch_tris.append((t0, t1, t2, min_x, max_x, min_y, max_y))
        self._patch_index.add_rect(min_x, min_y, max_x, max_y, ti)

    def _parse(self, bsp: bytes) -> None:
        if len(bsp) < 68 or bsp[:4] != b"IBSP":
            return

        plane_blob = _bsp_lump(bsp, BSP_LUMP_PLANES)
        for i in range(0, len(plane_blob) - 15, 16):
            nx, ny, nz, dist = struct.unpack_from("<4f", plane_blob, i)
            self.planes.append((nx, ny, nz, dist, _plane_signbits(nx, ny, nz)))

        shader_blob = _bsp_lump(bsp, BSP_LUMP_SHADERS)
        shader_contents: list[int] = []
        for i in range(0, len(shader_blob) - 71, 72):
            shader_contents.append(struct.unpack_from("<i", shader_blob, i + 68)[0])

        brush_blob = _bsp_lump(bsp, BSP_LUMP_BRUSHES)
        for i in range(0, len(brush_blob) - 11, 12):
            first_side, num_sides, shader_num = struct.unpack_from("<iii", brush_blob, i)
            self.brushes.append((first_side, num_sides, shader_num))
            solid = False
            if num_sides > 0 and 0 <= shader_num < len(shader_contents):
                solid = bool(shader_contents[shader_num] & CONTENTS_SOLID)
            self.brush_solid.append(solid)
            if solid:
                self._all_solid_brushes.append(len(self.brushes) - 1)

        side_blob = _bsp_lump(bsp, BSP_LUMP_BRUSHSIDES)
        for i in range(0, len(side_blob) - 7, 8):
            self.brush_sides.append(struct.unpack_from("<ii", side_blob, i))

        leaf_blob = _bsp_lump(bsp, BSP_LUMP_LEAFS)
        leaf_brush_blob = _bsp_lump(bsp, BSP_LUMP_LEAFBRUSHES)
        brush_xy: dict[int, list[float]] = {}
        if leaf_blob and leaf_brush_blob and len(leaf_blob) % DLEAF_SIZE == 0:
            num_leafs = len(leaf_blob) // DLEAF_SIZE
            for li in range(num_leafs):
                base = li * DLEAF_SIZE
                leaf = struct.unpack_from("<ii6i4i", leaf_blob, base)
                mn0, mn1, mx0, mx1 = float(leaf[2]), float(leaf[3]), float(leaf[5]), float(leaf[6])
                first_lb = leaf[8]
                num_lb = leaf[9]
                if num_lb <= 0:
                    continue
                lb_end = (first_lb + num_lb) * 4
                if lb_end > len(leaf_brush_blob):
                    continue
                for off in range(first_lb * 4, lb_end, 4):
                    (bi,) = struct.unpack_from("<i", leaf_brush_blob, off)
                    if bi < 0 or bi >= len(self.brushes) or not self.brush_solid[bi]:
                        continue
                    box = brush_xy.get(bi)
                    if box is None:
                        brush_xy[bi] = [mn0, mn1, mx0, mx1]
                    else:
                        if mn0 < box[0]:
                            box[0] = mn0
                        if mn1 < box[1]:
                            box[1] = mn1
                        if mx0 > box[2]:
                            box[2] = mx0
                        if mx1 > box[3]:
                            box[3] = mx1

        max_span = MAX_BRUSH_INDEX_SPAN
        for bi, box in brush_xy.items():
            if box[2] - box[0] > max_span or box[3] - box[1] > max_span:
                self._wide_brushes.append(bi)
            else:
                self._brush_index.add_rect(box[0], box[1], box[2], box[3], bi)

        surf_blob = _bsp_lump(bsp, BSP_LUMP_SURFACES)
        vert_blob = _bsp_lump(bsp, BSP_LUMP_DRAWVERTS)
        index_blob = _bsp_lump(bsp, BSP_LUMP_DRAWINDEXES)
        if surf_blob and vert_blob and len(surf_blob) % DSURFACE_SIZE == 0:
            num_surfs = len(surf_blob) // DSURFACE_SIZE
            for si in range(num_surfs):
                base = si * DSURFACE_SIZE
                shader_num, _fog, surf_type, first_vert = struct.unpack_from("<4i", surf_blob, base)
                if shader_num < 0 or shader_num >= len(shader_contents):
                    continue
                if not (shader_contents[shader_num] & CONTENTS_SOLID):
                    continue

                if surf_type == MST_PATCH:
                    patch_w, patch_h = struct.unpack_from("<2i", surf_blob, base + 96)
                    if patch_w < 2 or patch_h < 2:
                        continue

                    pts: list[tuple[float, float, float]] = []
                    ok = True
                    for j in range(patch_h):
                        for i in range(patch_w):
                            vo = (first_vert + j * patch_w + i) * DRAWVERT_SIZE
                            if vo + 12 > len(vert_blob):
                                ok = False
                                break
                            pts.append(struct.unpack_from("<3f", vert_blob, vo))
                        if not ok:
                            break
                    if not ok or len(pts) != patch_w * patch_h:
                        continue

                    for j in range(patch_h - 1):
                        for i in range(patch_w - 1):
                            q = (
                                pts[j * patch_w + i],
                                pts[(j + 1) * patch_w + i],
                                pts[(j + 1) * patch_w + i + 1],
                                pts[j * patch_w + i + 1],
                            )
                            for a, b, c in ((0, 1, 2), (0, 2, 3)):
                                self._push_solid_mesh_tri(q[a], q[b], q[c])

                elif surf_type == MST_PLANAR:
                    (num_verts,) = struct.unpack_from("<i", surf_blob, base + 16)
                    if num_verts < 3:
                        continue
                    pts = []
                    ok = True
                    for vi in range(num_verts):
                        vo = (first_vert + vi) * DRAWVERT_SIZE
                        if vo + 12 > len(vert_blob):
                            ok = False
                            break
                        pts.append(struct.unpack_from("<3f", vert_blob, vo))
                    if not ok or len(pts) != num_verts:
                        continue
                    for vi in range(1, num_verts - 1):
                        self._push_solid_mesh_tri(pts[0], pts[vi], pts[vi + 1])

                elif surf_type == MST_TRIANGLE_STRIP:
                    first_index, num_indexes = struct.unpack_from("<2i", surf_blob, base + 20)
                    if num_indexes < 3 or not index_blob:
                        continue
                    idx_end = (first_index + num_indexes) * 4
                    if idx_end > len(index_blob):
                        continue
                    for ii in range(num_indexes - 2):
                        i0, i1, i2 = struct.unpack_from(
                            "<3i", index_blob, (first_index + ii) * 4
                        )
                        if i0 == i1 or i1 == i2 or i0 == i2:
                            continue
                        if ii & 1:
                            i1, i2 = i2, i1
                        if i0 < 0 or i1 < 0 or i2 < 0:
                            continue
                        v0o = (first_vert + i0) * DRAWVERT_SIZE
                        v1o = (first_vert + i1) * DRAWVERT_SIZE
                        v2o = (first_vert + i2) * DRAWVERT_SIZE
                        if v2o + 12 > len(vert_blob):
                            continue
                        t0 = struct.unpack_from("<3f", vert_blob, v0o)
                        t1 = struct.unpack_from("<3f", vert_blob, v1o)
                        t2 = struct.unpack_from("<3f", vert_blob, v2o)
                        self._push_solid_mesh_tri(t0, t1, t2)

    def _trace_one_brush(
        self,
        start: tuple[float, float, float],
        end: tuple[float, float, float],
        brush: tuple[int, int, int],
    ) -> float | None:
        first_side, num_sides, _shader = brush
        if num_sides <= 0:
            return None
        offsets = self._trace_offsets
        enter_frac = -1.0
        leave_frac = 1.0
        startout = False

        for i in range(first_side, first_side + num_sides):
            if i < 0 or i >= len(self.brush_sides):
                return None
            plane_num, _surf = self.brush_sides[i]
            if plane_num < 0 or plane_num >= len(self.planes):
                return None
            nx, ny, nz, pdist, sb = self.planes[plane_num]
            ox, oy, oz = offsets[sb]
            dist = pdist - (nx * ox + ny * oy + nz * oz)

            d1 = nx * start[0] + ny * start[1] + nz * start[2] - dist
            d2 = nx * end[0] + ny * end[1] + nz * end[2] - dist

            if d1 > 0:
                startout = True

            if d1 > 0 and (d2 >= SURFACE_CLIP_EPSILON or d2 >= d1):
                return None

            if d1 <= 0 and d2 <= 0:
                continue

            if d1 > d2:
                f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2)
                if f < 0:
                    f = 0.0
                if f > enter_frac:
                    enter_frac = f
            else:
                f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2)
                if f > 1:
                    f = 1.0
                if f < leave_frac:
                    leave_frac = f

        if not startout:
            return 0.0
        if enter_frac < leave_frac and enter_frac > -1:
            if enter_frac < 0:
                enter_frac = 0.0
            return enter_frac
        return None

    def _box_trace_brushes(
        self,
        start: tuple[float, float, float],
        end: tuple[float, float, float],
        brush_ids: list[int],
    ) -> tuple[float, float, float]:
        best_frac = 1.0
        brushes = self.brushes
        brush_solid = self.brush_solid
        for bi in brush_ids:
            if not brush_solid[bi]:
                continue
            hit = self._trace_one_brush(start, end, brushes[bi])
            if hit is not None and hit < best_frac:
                best_frac = hit
        if best_frac >= 1.0:
            return end
        return (
            start[0] + best_frac * (end[0] - start[0]),
            start[1] + best_frac * (end[1] - start[1]),
            start[2] + best_frac * (end[2] - start[2]),
        )

    def _brush_candidates(self, cx: float, cy: float) -> list[int]:
        scratch = self._brush_scratch
        scratch.clear()
        self._brush_index.query_into(cx, cy, ITEM_RADIUS, scratch)
        for bi in self._wide_brushes:
            scratch.add(bi)
        if scratch:
            return list(scratch)
        return self._all_solid_brushes

    def _patch_candidates(self, cx: float, cy: float) -> list[int]:
        scratch = self._patch_scratch
        scratch.clear()
        self._patch_index.query_into(cx, cy, ITEM_RADIUS, scratch)
        return list(scratch)

    def _floor_z_at(self, px: float, py: float, z_top: float, tri_ids: list[int]) -> float | None:
        best: float | None = None
        tris = self.patch_tris
        for ti in tri_ids:
            if ti < 0 or ti >= len(tris):
                continue
            t0, t1, t2, min_x, max_x, min_y, max_y = tris[ti]
            if px < min_x or px > max_x or py < min_y or py > max_y:
                continue
            z = _z_on_triangle_xy(px, py, t0, t1, t2)
            if z is None or z >= z_top:
                continue
            if best is None or z > best:
                best = z
        return best

    def _box_trace_patches(
        self,
        start: tuple[float, float, float],
        end: tuple[float, float, float],
        mins: tuple[float, float, float],
        maxs: tuple[float, float, float],
    ) -> tuple[float, float, float]:
        if not self.patch_tris:
            return end

        cx, cy, cz = start
        surface_ids = self._patch_candidates(cx, cy)
        if not surface_ids:
            return end

        samples = (
            (cx, cy),
            (cx + mins[0], cy + mins[1]),
            (cx + maxs[0], cy + mins[1]),
            (cx + mins[0], cy + maxs[1]),
            (cx + maxs[0], cy + maxs[1]),
        )

        best_frac = 1.0
        best_end = end
        drop = cz - end[2]
        if drop <= 0.0:
            return end

        for px, py in samples:
            fz = self._floor_z_at(px, py, cz, surface_ids)
            if fz is None:
                continue
            end_z = fz + ITEM_RADIUS
            frac = (cz - end_z) / drop
            if frac < 0.0:
                frac = 0.0
            if frac > 1.0:
                continue
            if frac < best_frac:
                best_frac = frac
                best_end = (cx, cy, end_z)

        return best_end

    def drop(
        self,
        x: float,
        y: float,
        z: float,
        *,
        suspended: bool = False,
    ) -> tuple[float, float, float]:
        if suspended:
            return x, y, z

        key = (int(x), int(y), int(z))
        cached = self._origin_cache.get(key)
        if cached is not None:
            return cached

        r = ITEM_RADIUS
        mins = (-r, -r, -r)
        maxs = (r, r, r)
        start = (x, y, z)
        end = (x, y, z - DROP_DEPTH)

        brush_hit = self._box_trace_brushes(start, end, self._brush_candidates(x, y))
        patch_hit = self._box_trace_patches(start, end, mins, maxs)

        drop_len = start[2] - end[2]
        if drop_len <= 0.0:
            result = start
        else:
            fb = (start[2] - brush_hit[2]) / drop_len
            fp = (start[2] - patch_hit[2]) / drop_len
            result = patch_hit if fp < fb else brush_hit

        self._origin_cache[key] = result
        return result


_bsp_drop_cache: dict[int, _ItemDropCollision | None] = {}


def _item_drop_collision(bsp: bytes) -> _ItemDropCollision | None:
    key = id(bsp)
    if key not in _bsp_drop_cache:
        col = _ItemDropCollision(bsp)
        if not col.planes and not col.patch_tris:
            _bsp_drop_cache[key] = None
        else:
            _bsp_drop_cache[key] = col
    return _bsp_drop_cache[key]


def drop_pickup_origin(
    bsp: bytes,
    x: float,
    y: float,
    z: float,
    *,
    suspended: bool = False,
) -> tuple[float, float, float]:
    if not bsp:
        return x, y, z
    col = _item_drop_collision(bsp)
    if col is None:
        return x, y, z
    return col.drop(x, y, z, suspended=suspended)


def apply_item_origins_after_spawn(
    bsp: bytes | None,
    points: list[dict[str, Any]],
    entities: list[dict[str, str]],
) -> None:
    if not bsp:
        return
    col = _item_drop_collision(bsp)
    if col is None:
        return
    for pt in points:
        ent = entities[pt["e"]]
        ox, oy, oz = pt["o"]
        ox, oy, oz = float(ox), float(oy), float(oz)
        suspended = bool(_spawn_int(ent, "spawnflags", 0) & 1)
        nx, ny, nz = col.drop(ox, oy, oz, suspended=suspended)
        pt["o"] = [
            int(v) if float(v).is_integer() else v for v in (nx, ny, nz)
        ]
    release_item_drop_collision(bsp)


def release_item_drop_collision(bsp: bytes | None) -> None:
    if bsp is not None:
        _bsp_drop_cache.pop(id(bsp), None)


def clear_item_drop_collision_cache() -> None:
    _bsp_drop_cache.clear()


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
QITM1_VERSION = 1

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
    bsp_bytes: bytes | None = None,
) -> dict[str, Any]:
    points: list[dict[str, Any]] = []
    for idx, ent in enumerate(entities):
        if idx == 0:
            continue
        row = _item_point_row(idx, ent, item_table)
        if row:
            points.append(row)

    if bsp_bytes:
        apply_item_origins_after_spawn(bsp_bytes, points, entities)

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

    items_doc = build_item_report(entities, item_table, freeze, bsp_bytes)
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
    for pt in points:
        ox, oy, oz = pt["o"]
        mi = int(pt["mi"])
        sc = int(pt.get("sc", 0)) & 0xFF
        chunks.append(struct.pack("<fffHBB", float(ox), float(oy), float(oz), mi, sc, 0))

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


def _read_qmap_payload(path: Path) -> bytes:
    raw = path.read_bytes()
    if len(raw) >= 4 and raw[:4] == QMAP_MAGIC:
        return raw
    text = raw.decode("utf-8", errors="replace").strip()
    if not text:
        raise ValueError("empty file")
    doc = json.loads(text)
    if doc.get("v") != MAP_DOC_VERSION:
        raise ValueError(f"unsupported v={doc.get('v')!r}")
    return pack_qmap1_binary(doc)


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
            print(f"  pack read {n}/{total}...", file=sys.stderr, flush=True)

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

    for map_name in sorted(jobs.keys()):
        label, path, inner, _ = jobs[map_name]
        out_path = out_dir / f"{map_name}.qmap"
        if skip_existing and out_path.is_file():
            skipped_exist += 1
            continue
        print(f"  {map_name}...", file=sys.stderr, flush=True)
        try:
            doc = build_map_bundle_report(path, freeze, loc_gametype, inner, item_table)
        except (ValueError, OSError, zipfile.BadZipFile) as exc:
            skipped += 1
            print(f"skip {map_name} ({label}): {exc}", file=sys.stderr)
            continue
        finally:
            clear_item_drop_collision_cache()

        n_spawn = len(doc.get("spawns", {}).get("points") or [])
        n_item = len(doc.get("items", {}).get("points") or [])
        n_loc = len((doc.get("locations") or {}).get("locations") or [])
        if n_spawn == 0 and n_item == 0 and n_loc == 0:
            skipped_empty += 1
            if verbose:
                print(f"skip {map_name}: empty bundle ({label})", file=sys.stderr)
            continue

        out_path.write_text(
            json.dumps(doc, ensure_ascii=False, separators=(",", ":")) + "\n",
            encoding="utf-8",
        )
        written += 1
        print(
            f"wrote {out_path.name} (spawns={n_spawn} items={n_item} locs={n_loc}) <- {label}",
            file=sys.stderr,
        )

    print(
        f"done: {written} written, {skipped_exist} already present, "
        f"{skipped_empty} empty, {skipped} errors",
        file=sys.stderr,
    )
    return 0 if written or skipped_exist else 1


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
    parser = argparse.ArgumentParser(
        description="Сбор mapdata (spawns, shadow items, locations, tpexit) для OSP2-BE.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Примеры:\n"
            "  %(prog)s mymap.bsp\n"
            "  %(prog)s mymap.bsp -o assets/mapdata/mymap.qmap\n"
            "  %(prog)s --batch -o backup/mapdata\n"
            "  %(prog)s --batch --pack-q3lb -o assets/mapdata/mapdata.q3lb\n"
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
        if args.output and args.output.suffix.lower() == ".q3lb":
            out_dir = DEFAULT_OUT_DIR.resolve()
            pack_out = args.output.resolve()
        else:
            out_dir = args.output.resolve() if args.output else DEFAULT_OUT_DIR.resolve()
            pack_out = DEFAULT_BUNDLE_PATH.resolve()
        rc = batch_write_map_reports(
            dirs, out_dir, args.freeze, loc_gt, args.skip_existing, args.verbose, item_table
        )
        if rc != 0 or args.pack_q3lb is None:
            return rc
        return pack_q3lb_from_mapdata_dir(out_dir, pack_out)

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

    if args.verbose:
        n_spawn = len(doc.get("spawns", {}).get("points") or [])
        n_item = len(doc.get("items", {}).get("points") or [])
        n_loc = len((doc.get("locations") or {}).get("locations") or [])
        print(f"  map={map_name} spawns={n_spawn} items={n_item} locs={n_loc} <- {label}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
