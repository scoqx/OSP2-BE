# -*- coding: utf-8 -*-
"""Generate markdown blocks for cvars missing from osp2-q3-ClientReadme.md"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "assets" / "OSP2.cfg"
README = ROOT / "docs" / "osp2-q3-ClientReadme.md"
OUT = ROOT / "docs" / "_generated_missing_cvars.md"

SETA_RE = re.compile(
    r'^\s*seta\s+([A-Za-z0-9_]+)\s+"([^"]*)"\s*//\s*(.+)$'
)


def parse_cfg():
    items = []
    for line in CFG.read_text(encoding="utf-8").splitlines():
        m = SETA_RE.match(line)
        if m:
            items.append((m.group(1), m.group(2), m.group(3).strip()))
    return items


def in_readme(name: str, doc: str) -> bool:
    return bool(re.search(r"(?m)^\s{4}" + re.escape(name) + r"(\s|$)", doc))


def expand_value_lines(desc: str) -> tuple[str, list[str]]:
    """Split 'N - text' enumerations into backtick lines."""
    bullets: list[str] = []

    colon = re.search(
        r"(Битовая маска|Битмаска):\s*(.+)$",
        desc,
        re.I,
    )
    if colon:
        intro = desc[: colon.start()].strip()
        tail = colon.group(2)
        for n, txt in re.findall(r"(-?\d+)\s*-\s*([^,]+)", tail):
            bullets.append(f"`{n}` - {txt.strip().rstrip('.')}")
        if bullets:
            return f"{intro} {colon.group(1)}:", bullets

    if len(re.findall(r"-?\d+\s*-\s*", desc)) >= 2:
        parts = re.split(r"(?:,\s*|\.\s*)(?=-?\d+\s*-\s*)", desc)
        intro_chunks = []
        for p in parts:
            p = p.strip()
            m = re.match(r"(-?\d+)\s*-\s*(.+)", p)
            if m:
                bullets.append(f"`{m.group(1)}` - {m.group(2).strip().rstrip('.')}")
            else:
                intro_chunks.append(p)
        if bullets:
            intro = " ".join(intro_chunks).strip().rstrip(".,")
            return intro, bullets

    return desc, bullets


def block(name: str, desc: str) -> str:
    intro, bullets = expand_value_lines(desc)
    lines = [f"    {name}", ""]
    if intro:
        lines.append(intro)
        lines.append("")
    for b in bullets:
        lines.append(b)
        lines.append("")
    if not bullets and desc:
        if not intro:
            lines.append(desc)
            lines.append("")
    lines.append("---")
    lines.append("")
    return "\n".join(lines)


def main():
    doc = README.read_text(encoding="utf-8")
    missing = []
    seen = set()
    for name, val, desc in parse_cfg():
        if name in seen:
            continue
        seen.add(name)
        if not in_readme(name, doc):
            missing.append((name, val, desc))

    md = "".join(block(n, d) for n, v, d in missing)
    OUT.write_text(md, encoding="utf-8")
    print(f"Generated {len(missing)} blocks -> {OUT}")
    print(f"Size: {len(md)} chars")


if __name__ == "__main__":
    main()
