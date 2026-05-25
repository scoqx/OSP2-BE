#!/usr/bin/env python3
"""
Copy mod version_changelog.txt into osp2be-status-server deploy/ and patch
deploy/response.conf [status] from version.txt, version_changelog.txt, and
be_status_server.conf in repo root (flags, hfv).
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


def parse_status_config(path: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith("//"):
            continue
        if "=" not in line:
            continue
        key, _, val = line.partition("=")
        key = key.strip().lower()
        val = val.strip()
        if key in ("flags", "hfv"):
            fields[key] = val
    return fields


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
    parser.add_argument("--version-txt", type=Path, default=None)
    parser.add_argument("--changelog", type=Path, default=None)
    parser.add_argument(
        "--status-dir",
        type=Path,
        required=True,
        help="Checkout root of scoqx/osp2be-status-server",
    )
    parser.add_argument(
        "--status-config",
        type=Path,
        default=None,
        help="flags/hfv source (default: be_status_server.conf in mod repo root)",
    )
    args = parser.parse_args()

    status_dir = args.status_dir.resolve()

    if not args.version_txt or not args.changelog:
        print("Error: --version-txt and --changelog are required", file=sys.stderr)
        return 1

    for path in (args.version_txt, args.changelog):
        if not path.is_file():
            print(f"Error: missing file: {path}", file=sys.stderr)
            return 1

    deploy_dir = status_dir / "deploy"
    response_conf = deploy_dir / "response.conf"
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

    deploy_dir.mkdir(parents=True, exist_ok=True)
    dest_changelog = deploy_dir / "version_changelog.txt"
    shutil.copy2(args.changelog, dest_changelog)
    print(f"Copied changelog -> {dest_changelog}")

    status_config = args.status_config
    if status_config is None:
        status_config = Path(__file__).resolve().parent.parent / "be_status_server.conf"
    status_fields: dict[str, str] = {}
    if status_config.is_file():
        status_fields = parse_status_config(status_config)
    elif args.status_config is not None:
        print(f"Error: missing status config: {status_config}", file=sys.stderr)
        return 1

    text = response_conf.read_text(encoding="utf-8")
    text = set_status_field(text, "version", version)
    if date:
        text = set_status_field(text, "date", date)
    for key in ("flags", "hfv"):
        if key in status_fields:
            text = set_status_field(text, key, status_fields[key])
    response_conf.write_text(text, encoding="utf-8", newline="\n")

    extra = []
    if "flags" in status_fields:
        extra.append(f"flags={status_fields['flags']}")
    if "hfv" in status_fields:
        extra.append(f"hfv={status_fields['hfv']}")
    suffix = (", " + ", ".join(extra)) if extra else ""
    print(f"Patched {response_conf}: version={version}" + (f", date={date}" if date else "") + suffix)
    if not status_fields and status_config.is_file():
        print(f"Warning: no flags/hfv in {status_config}", file=sys.stderr)
    elif not status_config.is_file():
        print(f"Note: no {status_config}, flags/hfv unchanged in response.conf", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
