#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Reverse changelog_be.txt by version blocks (newest first).
Each block starts with DD.MM.YYYY be v X.XX
"""

import os
import re
import sys


def reverse_changelog(input_file, output_file):
    with open(input_file, "r", encoding="utf-8") as f:
        content = f.read()

    version_pattern = re.compile(r"^\d{2}\.\d{2}\.\d{4}\s+be\s+v\s*", re.MULTILINE)
    matches = list(version_pattern.finditer(content))

    if not matches:
        print("No version blocks found in changelog!", file=sys.stderr)
        return 1

    blocks = []
    for i, match in enumerate(matches):
        start_pos = match.start()
        if i + 1 < len(matches):
            end_pos = matches[i + 1].start()
        else:
            end_pos = len(content)
        block = content[start_pos:end_pos].rstrip()
        if block:
            blocks.append(block)

    blocks.reverse()
    reversed_content = "\n\n".join(blocks)

    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    with open(output_file, "w", encoding="utf-8", newline="\n") as f:
        f.write(reversed_content)
        if not reversed_content.endswith("\n"):
            f.write("\n")

    print(f"Blocks: {len(blocks)} -> {output_file}")
    return 0


def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    input_file = os.path.join(project_root, "changelog_be.txt")
    output_file = os.path.join(project_root, "assets", "docs", "changelog_be_r.txt")

    if not os.path.exists(input_file):
        print(f"Missing {input_file}", file=sys.stderr)
        return 1

    return reverse_changelog(input_file, output_file)


if __name__ == "__main__":
    sys.exit(main())
