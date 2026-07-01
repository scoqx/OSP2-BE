#!/usr/bin/env python3
"""
Walk code/ and fix common text junk in C sources:
  - leading UTF-8 BOM (EF BB BF), including repeated
  - exactly one trailing newline (LF)
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

DEFAULT_EXTENSIONS = frozenset(
    {
        ".c",
        ".h",
        ".cpp",
        ".hpp",
        ".asm",
        ".inc",
    }
)


def fix_bytes(data: bytes) -> bytes:
    while data.startswith(b"\xef\xbb\xbf"):
        data = data[3:]
    return data.rstrip(b"\r\n") + b"\n"


def process_file(path: Path, dry_run: bool) -> bool:
    raw = path.read_bytes()
    fixed = fix_bytes(raw)
    if fixed == raw:
        return False
    if not dry_run:
        path.write_bytes(fixed)
    return True


def main() -> int:
    repo = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--code-dir",
        type=Path,
        default=repo / "code",
        help="Root directory to scan (default: <repo>/code)",
    )
    parser.add_argument(
        "--ext",
        action="append",
        dest="extensions",
        metavar="EXT",
        help="File extension to include (repeatable). Default: .c .h .cpp .hpp .asm .inc",
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="List files that would change without writing",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Only print changed paths (or nothing if none)",
    )
    args = parser.parse_args()

    code_dir = args.code_dir.resolve()
    if not code_dir.is_dir():
        print(f"error: not a directory: {code_dir}", file=sys.stderr)
        return 1

    extensions = set(DEFAULT_EXTENSIONS)
    if args.extensions:
        extensions = {e if e.startswith(".") else f".{e}" for e in args.extensions}

    changed: list[Path] = []
    for path in sorted(code_dir.rglob("*")):
        if not path.is_file():
            continue
        if path.suffix.lower() not in extensions:
            continue
        if process_file(path, args.dry_run):
            changed.append(path)

    if not args.quiet and not changed:
        print(f"No changes under {code_dir}")
    for path in changed:
        prefix = "would fix" if args.dry_run else "fixed"
        try:
            rel = path.relative_to(repo)
        except ValueError:
            rel = path
        print(f"{prefix}: {rel}")

    if changed and not args.quiet:
        print(f"{'would change' if args.dry_run else 'changed'} {len(changed)} file(s)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
