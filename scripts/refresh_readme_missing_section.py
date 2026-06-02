# -*- coding: utf-8 -*-
"""Replace inserted missing-cvar section with freshly generated blocks."""
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "docs" / "osp2-q3-ClientReadme.md"
GEN = ROOT / "docs" / "_generated_missing_cvars.md"

subprocess.check_call(["python", str(ROOT / "scripts" / "gen_readme_missing_cvars.py")])

doc = README.read_text(encoding="utf-8")
gen = GEN.read_text(encoding="utf-8")

start = doc.index("    ch_TeamCrosshairHealth")
end = doc.index("    cg_drawBrightWeapons    ")
doc = doc[:start] + gen + doc[end:]

README.write_text(doc, encoding="utf-8")
print("Refreshed missing cvar section in readme")
