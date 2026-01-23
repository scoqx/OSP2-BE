#!/usr/bin/env python3
"""
Fail the build if any cvar description exceeds the engine limit.

Quake3e rejects:
  strlen(description) >= MAX_CVAR_VALUE_STRING

So the maximum allowed length is MAX_CVAR_VALUE_STRING - 1 bytes.
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Entry:
    cvar: str
    desc_raw: str
    desc_line: int


def _read_max_cvar_value_string(repo_root: Path) -> int:
    header = repo_root / "code" / "qcommon" / "q_shared.h"
    text = header.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"^\s*#\s*define\s+MAX_CVAR_VALUE_STRING\s+(\d+)\s*$", text, re.MULTILINE)
    if not m:
        raise RuntimeError(f"Can't find MAX_CVAR_VALUE_STRING in {header}")
    return int(m.group(1))


def _strip_c_comments_keep_strings(src: str) -> str:
    out = []
    i = 0
    n = len(src)
    in_str = False
    in_char = False
    while i < n:
        ch = src[i]

        if in_str:
            out.append(ch)
            if ch == "\\" and i + 1 < n:
                out.append(src[i + 1])
                i += 2
                continue
            if ch == '"':
                in_str = False
            i += 1
            continue

        if in_char:
            out.append(ch)
            if ch == "\\" and i + 1 < n:
                out.append(src[i + 1])
                i += 2
                continue
            if ch == "'":
                in_char = False
            i += 1
            continue

        if ch == '"':
            in_str = True
            out.append(ch)
            i += 1
            continue

        if ch == "'":
            in_char = True
            out.append(ch)
            i += 1
            continue

        # line comment
        if ch == "/" and i + 1 < n and src[i + 1] == "/":
            i += 2
            while i < n and src[i] != "\n":
                i += 1
            # keep newline if present
            if i < n and src[i] == "\n":
                out.append("\n")
                i += 1
            continue

        # block comment
        if ch == "/" and i + 1 < n and src[i + 1] == "*":
            i += 2
            while i + 1 < n and not (src[i] == "*" and src[i + 1] == "/"):
                if src[i] == "\n":
                    out.append("\n")
                i += 1
            i += 2 if i + 1 < n else 0
            continue

        out.append(ch)
        i += 1

    return "".join(out)


def _decode_c_string_to_bytes(raw: str) -> bytes:
    """
    raw: content between quotes, with C escapes (no surrounding ").
    Returns the byte sequence as best as we can model for strlen checks.
    """
    b = bytearray()
    i = 0
    n = len(raw)
    while i < n:
        ch = raw[i]
        if ch != "\\":
            # non-ascii characters: approximate by UTF-8 bytes
            code = ord(ch)
            if code <= 0xFF:
                b.append(code)
            else:
                b.extend(ch.encode("utf-8"))
            i += 1
            continue

        # escape
        i += 1
        if i >= n:
            # trailing backslash - treat literally
            b.append(ord("\\"))
            break

        esc = raw[i]
        i += 1

        if esc == "n":
            b.append(0x0A)
        elif esc == "r":
            b.append(0x0D)
        elif esc == "t":
            b.append(0x09)
        elif esc == "v":
            b.append(0x0B)
        elif esc == "b":
            b.append(0x08)
        elif esc == "f":
            b.append(0x0C)
        elif esc == "a":
            b.append(0x07)
        elif esc in ('"', "\\", "'"):
            b.append(ord(esc))
        elif esc == "?":
            b.append(ord("?"))
        elif esc == "x":
            # hex escape: \xHH...
            start = i
            while i < n and raw[i] in "0123456789abcdefABCDEF":
                i += 1
            if i == start:
                # invalid, keep as 'x'
                b.append(ord("x"))
            else:
                b.append(int(raw[start:i], 16) & 0xFF)
        elif esc in "01234567":
            # octal escape: up to 3 digits (we already consumed first digit)
            digits = esc
            count = 1
            while count < 3 and i < n and raw[i] in "01234567":
                digits += raw[i]
                i += 1
                count += 1
            b.append(int(digits, 8) & 0xFF)
        else:
            # unknown escape, treat as the escaped char itself
            b.append(ord(esc))

    return bytes(b)


def _parse_entries(repo_root: Path) -> list[Entry]:
    path = repo_root / "code" / "cgame" / "cg_cvardescriptions.c"
    src = path.read_text(encoding="utf-8", errors="replace")
    src = _strip_c_comments_keep_strings(src)

    # Narrow to the table to avoid false positives.
    m = re.search(r"static\s+cvarDescriptionTable_t\s+cvarDescriptionTable\[\]\s*=\s*\{", src)
    if not m:
        raise RuntimeError("Can't find cvarDescriptionTable[] initializer")
    start = m.end()  # position after '{'

    end = src.find("};", start)
    if end == -1:
        raise RuntimeError("Can't find end of cvarDescriptionTable[] (missing '};')")

    table = src[start:end]

    # Precompute line numbers within the file for error reporting.
    prefix = src[:start]
    base_line = prefix.count("\n") + 1

    # Match entries: { "name", "desc" }
    # (We intentionally do not support arbitrary expressions here.)
    entry_re = re.compile(
        r"""\{\s*"(?P<name>(?:\\.|[^"\\])*)"\s*,\s*"(?P<desc>(?:\\.|[^"\\])*)"\s*\}""",
        re.DOTALL,
    )

    entries: list[Entry] = []
    for em in entry_re.finditer(table):
        before = table[: em.start()]
        line = base_line + before.count("\n")
        cvar = bytes(em.group("name"), "utf-8").decode("unicode_escape", errors="replace")
        desc_raw = em.group("desc")
        entries.append(Entry(cvar=cvar, desc_raw=desc_raw, desc_line=line))

    return entries


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    limit = _read_max_cvar_value_string(repo_root)

    entries = _parse_entries(repo_root)
    if not entries:
        print("ERROR: no cvar descriptions found to validate", file=sys.stderr)
        return 2

    bad: list[tuple[Entry, int]] = []
    for e in entries:
        desc_bytes = _decode_c_string_to_bytes(e.desc_raw)
        # strlen() stops at NUL if present
        desc_len = desc_bytes.split(b"\x00", 1)[0].__len__()
        if desc_len >= limit:
            bad.append((e, desc_len))

    if bad:
        cfile = repo_root / "code" / "cgame" / "cg_cvardescriptions.c"
        print(
            f"ERROR: cvar description(s) exceed engine limit: strlen(desc) must be < {limit} bytes",
            file=sys.stderr,
        )
        for e, ln in bad:
            print(
                f"  {cfile}:{e.desc_line}: {e.cvar}: {ln} bytes (max {limit-1})",
                file=sys.stderr,
            )
        return 1

    print(f"OK: validated {len(entries)} cvar descriptions (< {limit} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

