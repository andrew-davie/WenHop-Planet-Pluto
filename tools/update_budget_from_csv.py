#!/usr/bin/env python3
"""
update_budget_from_csv.py - Merge new debug[] timing exports into the
budget[128] table in main/board.c.

Looks for *.csv files sitting directly in the project root (Gopher2600
globals-dump exports, columns: Parent,Name,Type,Address,Value; rows of
interest look like `debug,debug[N],short unsigned int,ADDR,0xHEX`).

For each CSV not already recorded in tools/.budget_csv_manifest.txt:
  - parse debug[0..127] values
  - for each budget[] entry in main/board.c:
      - if it's currently the _untimed_ placeholder and the CSV value is
        nonzero, replace it with the CSV value
      - if it's already a real number and the CSV value is strictly
        larger, replace it with the CSV value
      - otherwise leave it alone
  - whenever an entry's value is actually changed, its own trailing
    comment gets a " -- updated YYYY-MM-DD HH:MM TZ" stamp appended (any
    previous per-line stamp is replaced, not stacked); untouched lines
    keep whatever stamp (or lack of one) they already had
  - the '// Last updated: ...' stamp above budget[128] is refreshed
    whenever a new CSV is processed, whether or not it actually changed
    any budget[] values -- it is not touched on runs with no new CSVs
  - git add + commit main/board.c and the manifest for each CSV processed
    (never a blanket `git add .`, since other unrelated files in this
    tree may have their own in-progress uncommitted changes)
  - record the CSV filename in the manifest either way, so it's never
    reprocessed

Run with --dry-run to see what would change without touching board.c or git.
"""

import csv
import os
import re
import subprocess
import sys
from datetime import datetime

try:
    from zoneinfo import ZoneInfo
    STAMP_TZ = ZoneInfo("Australia/Brisbane")  # AEST, no DST -- user is in Yeppoon, QLD
except Exception:
    STAMP_TZ = None

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOARD_C = os.path.join(ROOT, "main", "board.c")
MANIFEST = os.path.join(ROOT, "tools", ".budget_csv_manifest.txt")

BUDGET_START_RE = re.compile(r"^static const unsigned short budget\[128\] = \{")
BUDGET_END_RE = re.compile(r"^\};")
ENTRY_RE = re.compile(r"^(\s*)(\S+),(\s*)//\s*(\d+)\s+(.*)$")
STAMP_RE = re.compile(r"^// Last updated: .*$")
LINE_STAMP_RE = re.compile(r"\s*--\s*updated\s+.*$")
UNTIMED_TOKEN = "_untimed_"

DEBUG_NAME_RE = re.compile(r"^debug\[(\d+)\]$")


def parse_csv(path):
    """Return {index: value} for debug[0..127] rows in the CSV."""
    values = {}
    with open(path, newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames:
            reader.fieldnames = [n.strip() for n in reader.fieldnames]
        for row in reader:
            name = (row.get("Name") or "").strip()
            m = DEBUG_NAME_RE.match(name)
            if not m:
                continue
            idx = int(m.group(1))
            if idx > 127:
                continue
            raw = (row.get("Value") or "").strip().lower()
            if not raw:
                continue
            if raw.startswith("0x"):
                raw = raw[2:]
            try:
                # exporter always writes Value as plain hex (fixed-width,
                # zero-padded to the type size), never prefixed with "0x"
                # and never decimal -- confirmed against debug[4] (0x00b5
                # = 181 = CH_DOORCLOSED's known-correct budget[] value)
                val = int(raw, 16)
            except ValueError:
                continue
            values[idx] = val
    return values


def format_entry(token, idx, name):
    return "    " + (token + ",").ljust(14) + "// " + str(idx).rjust(3) + " " + name


def merge(board_lines, csv_values):
    """Return (new_lines, list_of_changes)."""
    out = []
    changes = []
    in_block = False
    for line in board_lines:
        if not in_block and BUDGET_START_RE.match(line):
            in_block = True
            out.append(line)
            continue
        if in_block and BUDGET_END_RE.match(line):
            in_block = False
            out.append(line)
            continue
        if in_block:
            m = ENTRY_RE.match(line.rstrip("\n"))
            if not m:
                out.append(line)
                continue
            indent, token, pad, idx_s, name = m.groups()
            idx = int(idx_s)
            csv_val = csv_values.get(idx)
            current = None if token == UNTIMED_TOKEN else int(token)

            new_token = None
            if csv_val is not None and csv_val != 0:
                if current is None:
                    new_token = str(csv_val)
                elif csv_val > current:
                    new_token = str(csv_val)

            if new_token is not None:
                base_name = LINE_STAMP_RE.sub("", name)
                stamped_name = f"{base_name} -- updated {stamp_now()}"
                changes.append((idx, base_name, token, new_token))
                out.append(format_entry(new_token, idx, stamped_name) + "\n")
            else:
                out.append(line)
            continue
        out.append(line)
    return out, changes


def stamp_now():
    now = datetime.now(STAMP_TZ) if STAMP_TZ else datetime.now()
    return now.strftime("%Y-%m-%d %H:%M %Z").strip()


def update_stamp(lines):
    """Update (or insert) the '// Last updated: ...' line immediately
    before the budget[] declaration."""
    for i, line in enumerate(lines):
        if BUDGET_START_RE.match(line):
            new_stamp = f"// Last updated: {stamp_now()}\n"
            if i > 0 and STAMP_RE.match(lines[i - 1].rstrip("\n")):
                lines[i - 1] = new_stamp
            else:
                lines.insert(i, new_stamp)
            return lines
    return lines


def load_manifest():
    if not os.path.exists(MANIFEST):
        return set()
    with open(MANIFEST) as f:
        return set(l.strip() for l in f if l.strip())


def append_manifest(fname):
    with open(MANIFEST, "a") as f:
        f.write(fname + "\n")


def git(args, cwd=ROOT):
    return subprocess.run(["git"] + args, cwd=cwd, capture_output=True, text=True)


def main():
    dry_run = "--dry-run" in sys.argv

    csvs = sorted(
        (f for f in os.listdir(ROOT) if f.lower().endswith(".csv")),
        key=lambda f: os.path.getmtime(os.path.join(ROOT, f)),
    )
    processed = load_manifest()
    todo = [f for f in csvs if f not in processed]

    if not todo:
        print("no new csv files")
        return

    for fname in todo:
        path = os.path.join(ROOT, fname)
        print(f"processing {fname}")
        csv_values = parse_csv(path)

        with open(BOARD_C) as f:
            board_lines = f.readlines()

        new_lines, changes = merge(board_lines, csv_values)
        new_lines = update_stamp(new_lines)  # always stamp the attempt, changed or not

        if changes:
            print(f"  {len(changes)} budget[] entries updated:")
            for idx, name, old, new in changes:
                print(f"    [{idx:3d}] {name}: {old} -> {new}")
        else:
            print("  no changes (all csv values already <= current budget[])")

        if dry_run:
            continue

        with open(BOARD_C, "w") as f:
            f.writelines(new_lines)

        append_manifest(fname)

        git(["add", "main/board.c", "tools/.budget_csv_manifest.txt"])
        if changes:
            msg = f"budget[]: merge {len(changes)} timing update(s) from {fname}"
        else:
            msg = f"budget[]: record {fname} as processed (no changes)"
        r = git(["commit", "-m", msg])
        print("  " + r.stdout.strip().splitlines()[0] if r.stdout.strip() else "  git commit produced no output")


if __name__ == "__main__":
    main()
