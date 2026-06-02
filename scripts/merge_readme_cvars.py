# -*- coding: utf-8 -*-
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "docs" / "osp2-q3-ClientReadme.md"
GEN = ROOT / "docs" / "_generated_missing_cvars.md"

doc = README.read_text(encoding="utf-8")
gen = GEN.read_text(encoding="utf-8")

stub_start = "    Missing cvars from OSP2.cfg"
if stub_start in doc:
    i = doc.index(stub_start)
    j = doc.index("---", i)
    j = doc.index("\n", j) + 1  # after --- line
    # skip blank line
    if doc[j:j+2] == "\n\n":
        j += 1
    doc = doc[:i] + doc[j:]

marker = "Выводит список команд добавленных в BE. +Зелёные новые, *желтые изменённые\n\n---\n\n"
insert = "Выводит список команд добавленных в BE. +Зелёные новые, *желтые изменённые\n\n---\n\n" + gen
if marker in doc:
    doc = doc.replace(marker, insert, 1)
else:
    raise SystemExit("marker not found")

README.write_text(doc, encoding="utf-8")
print("Merged into", README)
