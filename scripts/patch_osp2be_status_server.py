#!/usr/bin/env python3
"""
Copy assets/version_changelog.txt into osp2be-status-server and patch [status]
version/date in response.conf (flags and other fields stay manual).
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path


def normalize_version(value: str) -> str:
    s = value.strip().lower()
    if s.startswith("be-"):
        s = s[3:]
    elif s.startswith("be"):
        s = s[2:].lstrip("-")
    if s.startswith("v"):
        s = s[1:]
    return s.strip()


def version_from_version_txt(text: str) -> str:
    for line in text.splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if "define" in s.lower() and "osp_version" in s.lower():
            continue
        ver = normalize_version(s)
        if ver:
            return ver
    return ""


def date_from_changelog_header(header: str) -> str:
    m = re.match(r"^(\d{2})\.(\d{2})\.(\d{4})\b", header.strip())
    if not m:
        return ""
    day, month, year = m.groups()
    return f"{year}{month}{day}"


def changelog_version_from_text(text: str) -> str:
    lines = text.replace("\r\n", "\n").split("\n")
    if not lines:
        return ""
    header = lines[0].strip()
    match = re.search(r"\bv([0-9]+\.[0-9]+[a-zA-Z]*)\b", header, re.I)
    if match:
        return normalize_version(match.group(1))
    return ""


def set_status_field(content: str, key: str, value: str) -> str:
    line = f"{key} = {value}"
    pattern = rf"(?m)^{re.escape(key)}\s*=.*$"
    if re.search(pattern, content):
        return re.sub(pattern, line, content, count=1)
    if re.search(r"(?m)^\[status\]\s*$", content):
        return re.sub(r"(?m)^(\[status\]\s*)$", rf"\1\n{line}", content, count=1)
    raise ValueError(f"Cannot set {key}: [status] section not found in response.conf")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version-txt", type=Path, required=True)
    parser.add_argument("--changelog", type=Path, required=True)
    parser.add_argument(
        "--status-dir",
        type=Path,
        required=True,
        help="Checkout root of scoqx/osp2be-status-server",
    )
    args = parser.parse_args()

    for path in (args.version_txt, args.changelog):
        if not path.is_file():
            print(f"Error: missing file: {path}", file=sys.stderr)
            return 1

    status_dir = args.status_dir.resolve()
    response_conf = status_dir / "response.conf"
    if not response_conf.is_file():
        print(f"Error: missing {response_conf}", file=sys.stderr)
        return 1

    version = version_from_version_txt(args.version_txt.read_text(encoding="utf-8"))
    if not version:
        print("Error: could not read version from version.txt", file=sys.stderr)
        return 1

    changelog_text = args.changelog.read_text(encoding="utf-8")
    cl_ver = changelog_version_from_text(changelog_text)
    if cl_ver and cl_ver != version:
        print(
            f"Warning: version.txt -> {version}, changelog header -> {cl_ver}",
            file=sys.stderr,
        )

    lines = changelog_text.replace("\r\n", "\n").split("\n")
    header = lines[0].strip() if lines else ""
    date = date_from_changelog_header(header)
    if not date:
        print(
            f"Warning: no DD.MM.YYYY date in changelog header: {header!r}",
            file=sys.stderr,
        )

    dest_changelog = status_dir / "version_changelog.txt"
    shutil.copy2(args.changelog, dest_changelog)
    print(f"Copied changelog -> {dest_changelog}")

    text = response_conf.read_text(encoding="utf-8")
    text = set_status_field(text, "version", version)
    if date:
        text = set_status_field(text, "date", date)
    response_conf.write_text(text, encoding="utf-8", newline="\n")
    print(f"Patched {response_conf}: version={version}" + (f", date={date}" if date else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
