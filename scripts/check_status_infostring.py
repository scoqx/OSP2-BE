#!/usr/bin/env python3
"""
Fail the build if BE status UDP infostrings would exceed MAX_INFO_STRING.

Mirrors scoqx/osp2be-status-server q3_status_emulator.py --check-config using:
  assets/version.txt, assets/version_changelog.txt, be_status_server.conf
and fixed [info] fields from deploy/response.conf (protocol, hostname).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

CHANGELOG_LINE_SEP = "|"
CHANGELOG_KEYS = ("be_cl",) + tuple(f"be_cl{i}" for i in range(6))
INFO_SLOT_KEYS = tuple(f"Info{i}" for i in range(6))
SAMPLE_CHALLENGE = "1234567890"

# deploy/response.conf [info] — not patched by OSP2-PBE CI
DEFAULT_INFO = {
    "protocol": "68",
    "hostname": "OSP2BE",
}

# deploy/response.conf [status] — CI patches version, date, flags, hfv only
DEFAULT_STATUS_EXTRA = {
    "sivj": "1",
    "TitleText": "0",
    # MD5 hex length (real mod_sig from gen_status_sig.py)
    "mod_sig": "715727b5a9a81bf8623b9effc1dac694",
}


def _read_max_info_string(repo_root: Path) -> int:
    header = repo_root / "code" / "qcommon" / "q_shared.h"
    text = header.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"^\s*#\s*define\s+MAX_INFO_STRING\s+(\d+)\s*$", text, re.MULTILINE)
    if not m:
        raise RuntimeError(f"Can't find MAX_INFO_STRING in {header}")
    return int(m.group(1))


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


def parse_version_changelog_file(text: str) -> tuple[str, str]:
    lines = text.replace("\r\n", "\n").split("\n")
    if not lines:
        return "", ""
    header = lines[0].strip()
    match = re.search(r"\bv([0-9]+\.[0-9]+[a-zA-Z0-9-]*)", header, re.I)
    file_ver = normalize_version(match.group(1)) if match else ""
    body_lines = [ln.rstrip() for ln in lines[1:]]
    while body_lines and not body_lines[-1].strip():
        body_lines.pop()
    body = "\n".join(body_lines).strip()
    return file_ver, body


def encode_changelog_body(body: str) -> str:
    lines: list[str] = []
    for raw in body.splitlines():
        line = raw.strip()
        if not line:
            continue
        if "\\" in line:
            line = line.replace("\\", "/")
        lines.append(line)
    return CHANGELOG_LINE_SEP.join(lines)


def build_infostring(pairs: dict[str, str], limit: int) -> str:
    parts: list[str] = []
    for key, value in pairs.items():
        if not key or key.startswith("#"):
            continue
        parts.append(f"\\{key}\\{value}")
    s = "".join(parts)
    if len(s) >= limit:
        raise ValueError(
            f"Infostring length {len(s)} exceeds MAX_INFO_STRING ({limit})"
        )
    return s


def infostring_length(pairs: dict[str, str]) -> int:
    parts: list[str] = []
    for key, value in pairs.items():
        if not key or key.startswith("#"):
            continue
        parts.append(f"\\{key}\\{value}")
    return len("".join(parts))


def pack_changelog_into_pairs(
    file_ver: str, encoded: str, merged: dict[str, str], limit: int
) -> dict[str, str]:
    if not encoded:
        return {}

    for key in CHANGELOG_KEYS + INFO_SLOT_KEYS:
        merged.pop(key, None)
    merged.pop("be_cl_ver", None)
    merged.pop("be_cl_parts", None)

    base = {"be_cl_ver": file_ver, "be_cl_parts": "1"}

    single = dict(merged)
    single.update(base)
    single["Info0"] = encoded
    if infostring_length(single) < limit:
        return {**base, "Info0": encoded}

    single = dict(merged)
    single.update(base)
    single["be_cl"] = encoded
    if infostring_length(single) < limit:
        return {**base, "be_cl": encoded}

    parts: list[str] = []
    buf = ""
    for piece in encoded.split(CHANGELOG_LINE_SEP):
        candidate = piece if not buf else f"{buf}{CHANGELOG_LINE_SEP}{piece}"
        trial = dict(merged)
        trial["be_cl_parts"] = "?"
        for i, p in enumerate(parts + [candidate]):
            trial[f"be_cl{i}"] = p
        if infostring_length(trial) >= limit and buf:
            parts.append(buf)
            buf = piece
        else:
            buf = candidate
    if buf:
        parts.append(buf)

    if len(parts) > 6:
        raise ValueError(
            f"Changelog needs {len(parts)} parts; max 6 (be_cl0..be_cl5 / Info0..Info5)"
        )

    out: dict[str, str] = {"be_cl_ver": file_ver, "be_cl_parts": str(len(parts))}
    for i, part in enumerate(parts):
        key = INFO_SLOT_KEYS[i] if len(parts) <= 6 else f"be_cl{i}"
        out[key] = part
    trial = dict(merged)
    trial.update(out)
    if infostring_length(trial) >= limit:
        raise ValueError(
            f"Changelog still too large after split ({len(encoded)} chars encoded)"
        )
    return out


def is_getinfo_only_key(key: str) -> bool:
    if len(key) < 5 or not key.startswith("Info"):
        return False
    return key[4:].isdigit()


def merge_getinfo(
    info: dict[str, str],
    status: dict[str, str],
    changelog_path: Path,
    limit: int,
) -> tuple[dict[str, str], str, str]:
    merged = dict(info)
    if status.get("version"):
        merged["version"] = status["version"]
    merged["challenge"] = SAMPLE_CHALLENGE
    merged["mod_data"] = status.get("mod_data", "")

    raw = changelog_path.read_text(encoding="utf-8")
    file_ver, body = parse_version_changelog_file(raw)
    expected = normalize_version(status.get("version", ""))
    if not file_ver:
        raise ValueError(
            "version_changelog.txt: no version in first line (expected e.g. '19.06.2026 be v1.05e')"
        )
    if expected and file_ver != expected:
        raise ValueError(
            f"Changelog version mismatch: version.txt -> {expected!r}, "
            f"changelog header -> {file_ver!r}"
        )
    encoded = encode_changelog_body(body)
    if not encoded:
        raise ValueError("version_changelog.txt: changelog body is empty")

    merged.update(pack_changelog_into_pairs(file_ver, encoded, merged, limit))
    merged["be_cl_ok"] = "1"
    merged["be_cl_err"] = ""
    return merged, file_ver, encoded


def merge_getstatus(info: dict[str, str], status: dict[str, str]) -> dict[str, str]:
    merged = {k: v for k, v in info.items() if not is_getinfo_only_key(k)}
    merged.update(status)
    merged["challenge"] = SAMPLE_CHALLENGE
    mod_data = status.get("mod_data")
    if mod_data is not None:
        merged["mod_data"] = mod_data
    return merged


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    limit = _read_max_info_string(repo_root)

    version_path = repo_root / "assets" / "version.txt"
    changelog_path = repo_root / "assets" / "version_changelog.txt"
    status_config_path = repo_root / "be_status_server.conf"

    for path in (version_path, changelog_path):
        if not path.is_file():
            print(f"ERROR: missing {path}", file=sys.stderr)
            return 2

    version = version_from_version_txt(version_path.read_text(encoding="utf-8"))
    if not version:
        print("ERROR: could not read version from assets/version.txt", file=sys.stderr)
        return 2

    cl_lines = changelog_path.read_text(encoding="utf-8").splitlines()
    header = cl_lines[0].strip() if cl_lines else ""
    date = date_from_changelog_header(header)
    status_fields = (
        parse_status_config(status_config_path)
        if status_config_path.is_file()
        else {}
    )

    status = dict(DEFAULT_STATUS_EXTRA)
    status["version"] = version
    if date:
        status["date"] = date
    status["flags"] = status_fields.get("flags", "0")
    status["hfv"] = status_fields.get("hfv", "0")
    status.setdefault("mod_data", "")

    info = dict(DEFAULT_INFO)

    errors: list[str] = []
    details: list[str] = []

    try:
        getinfo_pairs, file_ver, encoded = merge_getinfo(
            info, status, changelog_path, limit
        )
        getinfo_s = build_infostring(getinfo_pairs, limit)
        margin = limit - len(getinfo_s) - 1
        details.append(
            f"getinfo: {len(getinfo_s)} / {limit} bytes (margin {margin})"
        )
        details.append(
            f"changelog: version={file_ver!r}, raw {changelog_path.stat().st_size} bytes, "
            f"{len(encoded)} encoded (lines joined with {CHANGELOG_LINE_SEP!r})"
        )
    except ValueError as exc:
        errors.append(f"getinfo: {exc}")

    try:
        getstatus_pairs = merge_getstatus(info, status)
        getstatus_s = build_infostring(getstatus_pairs, limit)
        margin = limit - len(getstatus_s) - 1
        details.append(
            f"getstatus: {len(getstatus_s)} / {limit} bytes (margin {margin})"
        )
    except ValueError as exc:
        errors.append(f"getstatus: {exc}")

    if errors:
        print(
            f"ERROR: BE status infostring check failed (MAX_INFO_STRING = {limit})",
            file=sys.stderr,
        )
        for line in details:
            print(f"  {line}", file=sys.stderr)
        for line in errors:
            print(f"  {line}", file=sys.stderr)
        print(
            "  Shorten assets/version_changelog.txt and/or be_status_server.conf hfv; "
            "see osp2be-status-server q3_status_emulator.py --check-config",
            file=sys.stderr,
        )
        return 1

    for line in details:
        print(line)
    print(f"OK: BE status infostrings fit in MAX_INFO_STRING ({limit})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
