#!/usr/bin/env python3
"""
Generate loading progress bar TGAs (RLE, 32-bit BGRA, alpha).

Prompts for bar Width x Height and corner radius; canvas = bar + 2 px pad (max 1024).
Writes loading_bar_fill.tga and loading_bar_border.tga to the current working directory.
"""

from __future__ import annotations

import os
import re
import struct
import sys
import tkinter as tk
from dataclasses import dataclass
from tkinter import messagebox

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Pillow required: pip install Pillow", file=sys.stderr)
    sys.exit(1)

MAX_TEXTURE = 1024
CANVAS_PAD = 2
FILL_NAME = "loading_bar_fill.tga"
BORDER_NAME = "loading_bar_border.tga"


@dataclass(frozen=True)
class BarSpec:
    bar_w: int
    bar_h: int
    corner_radius: float
    img_w: int
    img_h: int
    pad_x: int
    pad_y: int
    border_width: float
    inner_gap: float

    @property
    def fill_inset(self) -> float:
        return self.border_width + self.inner_gap


def parse_dimensions(text: str) -> tuple[int, int]:
    text = text.strip().replace(",", " ")
    text = text.replace("x", " ").replace("X", " ")
    parts = [p for p in re.split(r"\s+", text) if p]
    if len(parts) != 2:
        raise ValueError("Need two numbers: Width Height (e.g. 880 32)")
    w, h = int(parts[0]), int(parts[1])
    if w < 1 or h < 1:
        raise ValueError("Width and height must be positive")
    return w, h


def parse_radius(text: str, bar_w: int, bar_h: int) -> float:
    text = text.strip()
    if not text:
        raise ValueError("Enter corner radius")
    radius = float(text.replace(",", "."))
    if radius < 0:
        raise ValueError("Radius cannot be negative")
    cap = min(bar_w, bar_h) * 0.5
    if radius > cap:
        raise ValueError(f"Radius cannot exceed {cap:g} for this bar")
    return radius


def make_bar_spec(bar_w: int, bar_h: int, corner_radius: float) -> BarSpec:
    img_w = bar_w + CANVAS_PAD * 2
    img_h = bar_h + CANVAS_PAD * 2
    if img_w > MAX_TEXTURE or img_h > MAX_TEXTURE:
        raise ValueError(
            f"Canvas {img_w}x{img_h} exceeds {MAX_TEXTURE}. "
            f"Reduce bar size (max {MAX_TEXTURE - CANVAS_PAD * 2} per side with pad)."
        )
    stroke = max(1.0, min(2.0, bar_h / 16.0))
    gap = max(1.0, min(2.0, bar_h / 16.0))
    return BarSpec(
        bar_w=bar_w,
        bar_h=bar_h,
        corner_radius=corner_radius,
        img_w=img_w,
        img_h=img_h,
        pad_x=CANVAS_PAD,
        pad_y=CANVAS_PAD,
        border_width=stroke,
        inner_gap=gap,
    )


def supersample_for(spec: BarSpec) -> int:
    m = max(spec.img_w, spec.img_h)
    if m <= 128:
        return 8
    if m <= 512:
        return 4
    return 2


def bar_bounds(spec: BarSpec, scale: int) -> tuple[float, float, float, float]:
    x0 = spec.pad_x * scale
    y0 = spec.pad_y * scale
    x1 = x0 + spec.bar_w * scale
    y1 = y0 + spec.bar_h * scale
    return x0, y0, x1, y1


def inset_bounds(
    x0: float, y0: float, x1: float, y1: float, inset: float
) -> tuple[float, float, float, float]:
    return x0 + inset, y0 + inset, x1 - inset, y1 - inset


def render_supersampled(spec: BarSpec, mode: str) -> Image.Image:
    s = supersample_for(spec)
    im = Image.new("RGBA", (spec.img_w * s, spec.img_h * s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(im)

    x0, y0, x1, y1 = bar_bounds(spec, s)
    radius = spec.corner_radius * s
    stroke = max(1, int(round(spec.border_width * s)))
    fill_inset = spec.fill_inset * s

    inner_h = (y1 - y0) - 2 * fill_inset
    max_fill_r = max(0.0, inner_h * 0.5)
    if mode == "fill":
        fx0, fy0, fx1, fy1 = inset_bounds(x0, y0, x1, y1, fill_inset)
        fr = min(max(0.0, radius - fill_inset), max_fill_r)
        draw.rounded_rectangle(
            [fx0, fy0, fx1, fy1],
            radius=fr,
            fill=(255, 255, 255, 255),
        )
    elif mode == "border":
        br = min(radius, (y1 - y0) * 0.5)
        draw.rounded_rectangle(
            [x0, y0, x1, y1],
            radius=br,
            outline=(255, 255, 255, 255),
            width=stroke,
        )
    else:
        raise ValueError(mode)

    return im.resize((spec.img_w, spec.img_h), Image.Resampling.LANCZOS)


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


def write_rle_tga(path: str, im: Image.Image) -> None:
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
    out_dir = os.path.dirname(os.path.abspath(path))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(path, "wb") as f:
        f.write(header)
        f.write(body)


def generate_tgas(spec: BarSpec, out_dir: str) -> tuple[str, str]:
    fill_path = os.path.join(out_dir, FILL_NAME)
    border_path = os.path.join(out_dir, BORDER_NAME)
    write_rle_tga(fill_path, render_supersampled(spec, "fill"))
    write_rle_tga(border_path, render_supersampled(spec, "border"))
    return fill_path, border_path


def run_dialog() -> None:
    root = tk.Tk()
    root.title("Loading bar TGA")
    root.resizable(False, False)

    pad = {"padx": 10, "pady": 6}

    tk.Label(
        root,
        text="Размеры бара — Width Height (одна строка, напр. 880 32 или 220x8):",
        anchor="w",
    ).grid(row=0, column=0, columnspan=2, sticky="w", **pad)

    size_var = tk.StringVar(value="880 32")
    tk.Entry(root, textvariable=size_var, width=28).grid(row=1, column=0, columnspan=2, sticky="we", **pad)

    tk.Label(root, text="Степень скругления (radius, px):", anchor="w").grid(
        row=2, column=0, columnspan=2, sticky="w", **pad
    )
    radius_var = tk.StringVar(value="16")
    tk.Entry(root, textvariable=radius_var, width=12).grid(row=3, column=0, sticky="w", **pad)

    tk.Label(
        root,
        text=f"Холст: бар + {CANVAS_PAD * 2} px по краям, макс. {MAX_TEXTURE}. "
        f"Файлы — в папке запуска скрипта.",
        fg="gray",
        anchor="w",
        wraplength=360,
    ).grid(row=4, column=0, columnspan=2, sticky="w", **pad)

    def on_generate() -> None:
        try:
            bar_w, bar_h = parse_dimensions(size_var.get())
            radius = parse_radius(radius_var.get(), bar_w, bar_h)
            spec = make_bar_spec(bar_w, bar_h, radius)
            out_dir = os.getcwd()
            fill_path, border_path = generate_tgas(spec, out_dir)
        except ValueError as exc:
            messagebox.showerror("Ошибка", str(exc), parent=root)
            return
        except OSError as exc:
            messagebox.showerror("Ошибка", f"Не удалось записать файлы:\n{exc}", parent=root)
            return

        messagebox.showinfo(
            "Готово",
            f"Бар {spec.bar_w}x{spec.bar_h}, radius {spec.corner_radius:g}\n"
            f"Холст {spec.img_w}x{spec.img_h}\n\n"
            f"{fill_path}\n{border_path}",
            parent=root,
        )

    btn = tk.Frame(root)
    btn.grid(row=5, column=0, columnspan=2, **pad)
    tk.Button(btn, text="Создать", width=12, command=on_generate).pack(side="left", padx=4)
    tk.Button(btn, text="Закрыть", width=12, command=root.destroy).pack(side="left", padx=4)

    root.mainloop()


def main() -> None:
    run_dialog()


if __name__ == "__main__":
    main()
