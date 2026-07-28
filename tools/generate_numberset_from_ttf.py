#!/usr/bin/env python3
"""
Generate cg_numbersSet digit sprites from a TTF font.

Renders 0-9 and minus into square RLE TGA (white + alpha), writes a
customNumbers_<name>.shader overlay, and packs a test .pk3.

Example:
  python tools/generate_numberset_from_ttf.py assets/fonts/Cookie.ttf --name trinity
"""

from __future__ import annotations

import argparse
import struct
import sys
import zipfile
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFilter, ImageFont
except ImportError:
    print("Pillow required: pip install Pillow", file=sys.stderr)
    sys.exit(1)

# Matches code/cgame/cg_main.c cg_numberSuffix[]
GLYPHS: list[tuple[str, str]] = [
    ("0", "zero_32b"),
    ("1", "one_32b"),
    ("2", "two_32b"),
    ("3", "three_32b"),
    ("4", "four_32b"),
    ("5", "five_32b"),
    ("6", "six_32b"),
    ("7", "seven_32b"),
    ("8", "eight_32b"),
    ("9", "nine_32b"),
    ("-", "minus_32b"),
]

KNOWN_SETS = frozenset({"elite", "eternal", "odyssey", "trinity", "cookie"})
ALPHA_THRESHOLD = 8
RENDER_POINT_SIZE = 512
DEFAULT_SIZE = 128
DEFAULT_PAD_Y = 0.05
DEFAULT_PAD_X = 0.05
DEFAULT_THICKNESS = 0.0  # fraction of canvas; alpha dilation after resize
DEFAULT_NAME = "trinity"


def rgba_to_bgra_rows(im: Image.Image) -> list[bytes]:
    if im.mode != "RGBA":
        im = im.convert("RGBA")
    px = im.load()
    rows = []
    for y in range(im.height - 1, -1, -1):
        row = bytearray()
        for x in range(im.width):
            r, g, b, a = px[x, y]
            row.extend((b, g, r, a))
        rows.append(bytes(row))
    return rows


def rle_encode_row(raw: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(raw)
    while i < n:
        raw_chunk = bytearray()
        while i < n:
            run = 1
            px = raw[i : i + 4]
            while i + run * 4 < n and raw[i + run * 4 : i + run * 4 + 4] == px and run < 128:
                run += 1
            if run >= 4:
                break
            raw_chunk.extend(px)
            i += 4
            if len(raw_chunk) // 4 >= 128:
                break
        if raw_chunk:
            count = len(raw_chunk) // 4
            out.append(count - 1)
            out.extend(raw_chunk)
        if i >= n:
            break
        px = raw[i : i + 4]
        run = 1
        while i + run * 4 < n and raw[i + run * 4 : i + run * 4 + 4] == px and run < 128:
            run += 1
        out.append(0x80 | (run - 1))
        out.extend(px)
        i += run * 4
    return bytes(out)


def write_rle_tga(path: Path, im: Image.Image) -> None:
    width, height = im.size
    rows = rgba_to_bgra_rows(im)
    header = struct.pack(
        "<BBBHHBHHHHBB",
        0,
        0,
        10,
        0,
        0,
        0,
        0,
        0,
        width,
        height,
        32,
        0x08,
    )
    body = bytearray()
    for row in rows:
        body.extend(rle_encode_row(row))
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as f:
        f.write(header)
        f.write(body)


def alpha_bbox(im: Image.Image, threshold: int = ALPHA_THRESHOLD) -> tuple[int, int, int, int] | None:
    if im.mode != "RGBA":
        im = im.convert("RGBA")
    alpha = im.getchannel("A")
    # getbbox uses non-zero; threshold via point
    mask = alpha.point(lambda a: 255 if a > threshold else 0)
    return mask.getbbox()


def thicken_alpha(im: Image.Image, radius_px: int) -> Image.Image:
    """Expand glyph coverage slightly (bolder strokes)."""
    if radius_px <= 0:
        return im
    _r, _g, _b, a = im.split()
    for _ in range(radius_px):
        a = a.filter(ImageFilter.MaxFilter(3))
    white = Image.new("L", im.size, 255)
    return Image.merge("RGBA", (white, white, white, a))


def force_white_rgb(im: Image.Image) -> Image.Image:
    _r, _g, _b, a = im.split()
    white = Image.new("L", im.size, 255)
    return Image.merge("RGBA", (white, white, white, a))


def render_glyph(
    font_path: Path,
    char: str,
    size: int,
    pad_x: float,
    pad_y: float,
    thickness: float,
) -> Image.Image:
    font = ImageFont.truetype(str(font_path), RENDER_POINT_SIZE)
    canvas_w = RENDER_POINT_SIZE * 3
    canvas_h = RENDER_POINT_SIZE * 3
    tmp = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(tmp)
    cx, cy = canvas_w // 2, canvas_h // 2
    draw.text((cx, cy), char, font=font, fill=(255, 255, 255, 255), anchor="mm")

    bbox = alpha_bbox(tmp)
    if bbox is None:
        raise ValueError(f"glyph {char!r} produced no visible pixels in {font_path}")

    glyph = tmp.crop(bbox)
    max_w = size * (1.0 - 2.0 * pad_x)
    max_h = size * (1.0 - 2.0 * pad_y)
    if max_w < 1 or max_h < 1:
        raise ValueError("padding too large for canvas size")

    # Room for post-resize dilation so ink stays inside the pad box
    thick_px = max(0, int(round(size * thickness)))
    fit_w = max(1.0, max_w - 2 * thick_px)
    fit_h = max(1.0, max_h - 2 * thick_px)

    gw, gh = glyph.size
    scale = min(fit_w / gw, fit_h / gh)
    new_w = max(1, int(round(gw * scale)))
    new_h = max(1, int(round(gh * scale)))
    glyph = glyph.resize((new_w, new_h), Image.Resampling.LANCZOS)
    glyph = force_white_rgb(glyph)
    glyph = thicken_alpha(glyph, thick_px)

    bb = alpha_bbox(glyph)
    if bb is not None:
        glyph = glyph.crop(bb)
    gw, gh = glyph.size
    if gw > size or gh > size:
        clamp = min(size / gw, size / gh)
        glyph = glyph.resize(
            (max(1, int(round(gw * clamp))), max(1, int(round(gh * clamp)))),
            Image.Resampling.LANCZOS,
        )
        glyph = force_white_rgb(glyph)
        gw, gh = glyph.size

    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    x = (size - gw) // 2
    y = (size - gh) // 2
    out.paste(glyph, (x, y), glyph)
    return out


def shader_block(set_name: str, stem: str) -> str:
    path = f"gfx/2d/customnumbers/{set_name}/{stem}"
    return (
        f"{path}\n"
        "{\n"
        "\tnopicmip\n"
        "\tnomipmaps\n"
        "\t{\n"
        f"\t\tmap {path}.tga\n"
        "\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n"
        "\t\trgbgen vertex\n"
        "\t}\n"
        "}\n"
    )


def write_shader(path: Path, set_name: str) -> None:
    parts = [shader_block(set_name, stem) for _, stem in GLYPHS]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(parts), encoding="utf-8", newline="\n")


def pack_pk3(staging_root: Path, pk3_path: Path) -> None:
    pk3_path.parent.mkdir(parents=True, exist_ok=True)
    tmp_path = pk3_path.with_suffix(pk3_path.suffix + ".tmp")
    if tmp_path.exists():
        tmp_path.unlink()
    with zipfile.ZipFile(tmp_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for file_path in sorted(staging_root.rglob("*")):
            if not file_path.is_file():
                continue
            arcname = file_path.relative_to(staging_root).as_posix()
            zf.write(file_path, arcname)
    try:
        if pk3_path.exists():
            pk3_path.unlink()
        tmp_path.replace(pk3_path)
    except PermissionError:
        # File locked (e.g. open in game) — leave .tmp and also write a sibling
        alt = pk3_path.with_name(pk3_path.stem + "-new" + pk3_path.suffix)
        if alt.exists():
            try:
                alt.unlink()
            except PermissionError:
                pass
        tmp_path.replace(alt)
        raise PermissionError(
            f"cannot overwrite {pk3_path} (file in use); wrote {alt} instead"
        ) from None


def default_out_dir(repo_tools: Path, set_name: str) -> Path:
    return repo_tools / "out" / f"numberset_{set_name}"


def default_pk3_path(repo_tools: Path, set_name: str) -> Path:
    return repo_tools / "out" / f"zz-numberset-{set_name}.pk3"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    tools_dir = Path(__file__).resolve().parent
    p = argparse.ArgumentParser(
        description="Generate cg_numbersSet TGA sprites + shader + test pk3 from a TTF."
    )
    p.add_argument("font", type=Path, help="Path to .ttf (e.g. assets/fonts/Cookie.ttf)")
    p.add_argument(
        "--name",
        default=DEFAULT_NAME,
        help=f"Set folder name under gfx/2d/customnumbers/ (default: {DEFAULT_NAME})",
    )
    p.add_argument(
        "--size",
        type=int,
        default=DEFAULT_SIZE,
        help=f"Square canvas size in pixels (default: {DEFAULT_SIZE})",
    )
    p.add_argument(
        "--padding",
        type=float,
        default=None,
        help="Uniform margin fraction (sets both --pad-x and --pad-y)",
    )
    p.add_argument(
        "--pad-x",
        type=float,
        default=DEFAULT_PAD_X,
        help=f"Left/right margin fraction (default: {DEFAULT_PAD_X})",
    )
    p.add_argument(
        "--pad-y",
        type=float,
        default=DEFAULT_PAD_Y,
        help=f"Top/bottom margin fraction (default: {DEFAULT_PAD_Y})",
    )
    p.add_argument(
        "--thickness",
        type=float,
        default=DEFAULT_THICKNESS,
        help=f"Stroke thicken as fraction of canvas (default: {DEFAULT_THICKNESS})",
    )
    p.add_argument(
        "--out-dir",
        type=Path,
        default=None,
        help="Staging directory (default: tools/out/numberset_<name>/)",
    )
    p.add_argument(
        "--pk3",
        type=Path,
        default=None,
        help="Output .pk3 path (default: tools/out/zz-numberset-<name>.pk3)",
    )
    p.add_argument(
        "--no-pk3",
        action="store_true",
        help="Skip packing the .pk3",
    )
    args = p.parse_args(argv)
    if args.padding is not None:
        args.pad_x = args.padding
        args.pad_y = args.padding
    if args.out_dir is None:
        args.out_dir = default_out_dir(tools_dir, args.name)
    if args.pk3 is None:
        args.pk3 = default_pk3_path(tools_dir, args.name)
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    font_path: Path = args.font
    if not font_path.is_file():
        print(f"error: font not found: {font_path}", file=sys.stderr)
        return 1
    if args.size < 8:
        print("error: --size must be >= 8", file=sys.stderr)
        return 1
    for label, value in (("pad-x", args.pad_x), ("pad-y", args.pad_y)):
        if not (0.0 <= value < 0.5):
            print(f"error: --{label} must be in [0, 0.5)", file=sys.stderr)
            return 1
    if args.thickness < 0:
        print("error: --thickness must be >= 0", file=sys.stderr)
        return 1

    set_name = args.name.strip().replace("\\", "/").strip("/")
    if not set_name or "/" in set_name or ".." in set_name:
        print("error: --name must be a single folder name", file=sys.stderr)
        return 1
    if set_name not in KNOWN_SETS:
        print(
			f"warn: '{set_name}' is not elite/eternal/odyssey/trinity/cookie; "
			"pk3 alone will not work until cg_numbersSet is wired in C.",
            file=sys.stderr,
        )

    staging: Path = args.out_dir
    gfx_dir = staging / "gfx" / "2d" / "customnumbers" / set_name
    scripts_dir = staging / "scripts"

    # Clean previous glyph outputs in this staging tree
    if gfx_dir.exists():
        for old in gfx_dir.glob("*_32b.tga"):
            old.unlink()
    gfx_dir.mkdir(parents=True, exist_ok=True)

    print(f"font: {font_path}")
    print(
        f"set:  {set_name}  size={args.size}  "
        f"pad-x={args.pad_x}  pad-y={args.pad_y}  thickness={args.thickness}"
    )
    print(f"out:  {staging}")

    for char, stem in GLYPHS:
        im = render_glyph(
            font_path, char, args.size, args.pad_x, args.pad_y, args.thickness
        )
        tga_path = gfx_dir / f"{stem}.tga"
        write_rle_tga(tga_path, im)
        print(f"  wrote {tga_path.relative_to(staging)}")

    shader_path = scripts_dir / f"customNumbers_{set_name}.shader"
    write_shader(shader_path, set_name)
    print(f"  wrote {shader_path.relative_to(staging)}")

    if not args.no_pk3:
        pack_pk3(staging, args.pk3)
        print(f"pk3: {args.pk3}")
        print(f"Drop into mod folder, set cg_numbersSet, then vid_restart.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
