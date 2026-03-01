#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OSP2-BE config filter.

1) Parse cg_main.c and extract OSP2-BE cvar names from cvarTable[] (excluding
   CVAR_ROM, CVAR_TEMP, CVAR_CHEAT, and CVAR_ARCHIVE_ND), then save the list to a file.

2) Filter a config file: keep only lines that set one of those cvars
   (seta/set/bare form), preserving order. Write result to a new .cfg file.

Usage (run from scripts/):
  # 1. Extract list of allowed cvars from cg_main.c (run after code changes)
  python osp2_cfg_filter.py --parse [--out osp2_be_cvars.txt]
  # Without path, uses ../code/cgame/cg_main.c

  # 2. Filter config: input.cfg -> output.cfg using saved list
  python osp2_cfg_filter.py input.cfg output.cfg [--cvars osp2_be_cvars.txt]
"""

import re
import argparse
import os


# When run from scripts/, path to cg_main.c
DEFAULT_CG_MAIN = os.path.join("..", "code", "cgame", "cg_main.c")
DEFAULT_CVARS_FILE = "osp2_be_cvars.txt"

# Cvars to never copy into filtered config (engine/game, not OSP2-BE settings)
OSP2_CFG_IGNORE = frozenset([
    "com_buildScript", "com_cameraMode", "com_maxfps", "g_synchronousClients",
    "handicap", "m_pitch", "osp_debug", "osp_print_issues", "pmove_fixed", "pmove_msec",
    "r_fullbright", "r_lodCurveError", "r_shownormals", "r_showtris", "r_subdivisions", "r_znear",
    "snaps", "timescale", "uc_sensitivity",
])
OSP2_CFG_IGNORE_LOWER = {c.lower() for c in OSP2_CFG_IGNORE}


def parse_cvar_table(content):
    """
    Find cvarTable[] = { ... }; (main table only, not cvarTable_temp).
    For each entry, extract cvar name and exclude if CVAR_ROM, CVAR_TEMP, CVAR_CHEAT, or CVAR_ARCHIVE_ND.
    Returns set of allowed cvar names (lowercase for matching).
    """
    start_marker = "cvarTable_t cvarTable[]"
    start = content.find(start_marker)
    if start == -1:
        return set()

    brace = content.find("{", start)
    if brace == -1:
        return set()

    temp_marker = "cvarTable_temp"
    temp_pos = content.find(temp_marker, brace)
    if temp_pos == -1:
        temp_pos = len(content)

    block = content[brace:temp_pos]

    # Entry line: { &var, "name", "default", FLAGS }, — extract "name", skip if ROM/TEMP
    entry_re = re.compile(r'^\s*\{\s*&\w+,\s*"([^"]+)"\s*,')
    allowed = set()
    for line in block.splitlines():
        line = line.strip()
        if not line.startswith("{"):
            continue
        if "CVAR_ROM" in line or "CVAR_TEMP" in line or "CVAR_CHEAT" in line or "CVAR_ARCHIVE_ND" in line:
            continue
        m = entry_re.match(line)
        if not m:
            continue
        name = m.group(1).strip()
        if name:
            allowed.add(name)
    return allowed


def parse_cg_main(path):
    """Read cg_main.c and return set of allowed cvar names."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        content = f.read()
    return parse_cvar_table(content)


def extract_cvar_from_line(line):
    """
    Parse a config line. Supports:
      seta cvar value
      set cvar value
      cvar value
    Returns (cvar_name, None) if not a cvar set line; (cvar_name, full_line) if it is.
    """
    raw = line
    if "//" in line:
        line = line[: line.index("//")].rstrip()
    line = line.strip()
    if not line:
        return (None, None)
    parts = line.split(None, 2)
    if len(parts) == 0:
        return (None, None)
    if parts[0].lower() in ("seta", "set"):
        if len(parts) < 2:
            return (None, None)
        return (parts[1], raw.rstrip())
    return (parts[0], raw.rstrip())


def filter_config(input_path, output_path, allowed_cvars):
    """Keep only lines that set an allowed cvar. Preserve order. Returns count."""
    allowed_lower = {c.lower() for c in allowed_cvars}
    kept = []
    with open(input_path, "r", encoding="utf-8", errors="replace") as f:
        for raw_line in f:
            cvar, full_line = extract_cvar_from_line(raw_line)
            if full_line is None:
                continue
            if cvar and cvar.lower() in allowed_lower:
                kept.append(full_line)
    with open(output_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(kept))
        if kept:
            f.write("\n")
    return len(kept)


def load_cvars_list(path):
    """Load one cvar per line from file. Return set (lowercase)."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return {line.strip().lower() for line in f if line.strip()}


def main():
    ap = argparse.ArgumentParser(
        description="OSP2-BE: parse cvar list from cg_main.c or filter config by that list."
    )
    ap.add_argument(
        "--parse",
        nargs="?",
        const="<default>",
        metavar="CG_MAIN",
        help="Parse cg_main.c and write allowed cvars (excluding CVAR_ROM, CVAR_TEMP, CVAR_CHEAT, CVAR_ARCHIVE_ND). If path omitted, uses %s (run from scripts/)." % DEFAULT_CG_MAIN,
    )
    ap.add_argument(
        "--out",
        metavar="FILE",
        default=DEFAULT_CVARS_FILE,
        help="Output file for --parse (default: %s)." % DEFAULT_CVARS_FILE,
    )
    ap.add_argument(
        "--cvars",
        metavar="FILE",
        default=DEFAULT_CVARS_FILE,
        help="Cvars list file for filter mode (default: %s)." % DEFAULT_CVARS_FILE,
    )
    ap.add_argument(
        "input",
        nargs="?",
        help="Input config file (filter mode).",
    )
    ap.add_argument(
        "output",
        nargs="?",
        help="Output config file (filter mode).",
    )
    args = ap.parse_args()

    if args.parse is not None:
        cg_main_path = args.parse if args.parse != "<default>" else DEFAULT_CG_MAIN
        if not os.path.isfile(cg_main_path):
            print("Error: file not found: %s" % os.path.abspath(cg_main_path))
            return 1
        allowed = parse_cg_main(cg_main_path)
        allowed = {n for n in allowed if n.lower() not in OSP2_CFG_IGNORE_LOWER}
        out_path = args.out
        with open(out_path, "w", encoding="utf-8", newline="\n") as f:
            for name in sorted(allowed):
                f.write(name + "\n")
        print("Parsed %d OSP2-BE cvars (excluding CVAR_ROM, CVAR_TEMP, CVAR_CHEAT, CVAR_ARCHIVE_ND, and ignore list)." % len(allowed))
        print("Saved to: %s" % os.path.abspath(out_path))
        return 0

    if not args.input or not args.output:
        ap.error("For filter mode provide input and output files (or use --parse for parse mode).")
    if not os.path.isfile(args.input):
        print("Error: input file not found: %s" % args.input)
        return 1
    if not os.path.isfile(args.cvars):
        print("Error: cvars list not found: %s (run with --parse first)" % args.cvars)
        return 1
    allowed = load_cvars_list(args.cvars) - OSP2_CFG_IGNORE_LOWER
    n = filter_config(args.input, args.output, allowed)
    print("Written %d OSP2-BE commands to: %s" % (n, os.path.abspath(args.output)))
    print("Done.")
    return 0


if __name__ == "__main__":
    exit(main())
