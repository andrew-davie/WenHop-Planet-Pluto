#!/usr/bin/env python3
"""
update_budget_from_csv.py - Merge new debug[]/debugOvertime[] timing exports
into the budget[128] table in main/board.c.

Looks for *.csv files sitting directly in the project root (Gopher2600
globals-dump exports, columns: Parent,Name,Type,Address,Value; rows of
interest look like `debug,debug[N],unsigned int,ADDR,0xHEX` and
`debugOvertime,debugOvertime[N],unsigned short,ADDR,0xHEX`).

debug[] (main.h/main.c) records a running MAX and a running AVERAGE for every
raw character number n (0-127), unconditionally, every visit -- three fixed
128-wide blocks:

    debug[n]         MAX   -- worst single-visit T1TC tick count for character n.
    debug[128 + n]   SUM   -- running sum of ticks across every visit to character n.
    debug[256 + n]   COUNT -- visit count for character n (debug[128+n] / debug[256+n]
                              is that character's average cost).

debugOvertime[n] is a separate, smaller array (unsigned short, not part of
debug[] -- kept apart purely for RAM: the ARM side's `ram` region had almost
no slack left, and overshoot amounts don't need unsigned int range) -- the
worst T1TC overshoot seen while character n was the one being processed when
a whole processBoardSquares() pass ran past availableIdleTime.

This script merges all three, per character, independently:

  - MAX  -> budget[]'s own value/token (the `_B + N` / `_untimed_` / `_nop_`
            part), replacing it only when the CSV's value is strictly larger
            than what's already there. `_nop_` entries are never touched --
            that's a hand-verified "no handler exists" fact, not a timing
            measurement.
  - AVERAGE -> a "(avg: N)" annotation inside budget[]'s trailing comment,
            independent of the token to its left, same replace-only-if-
            larger rule -- "(avg: N)" always shows the worst AVERAGE seen
            across every capture, not just the latest one. Skipped for a
            character whose visit count is 0 in this capture (no data).
  - OVERTIME -> a "(overtime by N)" PREFIX right after the "//" (before the
            index), e.g. "_B + 2000, // (overtime by 252) 10 CH_ROCK ...",
            same replace-only-if-larger rule. Skipped for a character that
            was never caught mid-processing during an overrun in this
            capture (debugOvertime[n] == 0, or the CSV predates
            debugOvertime[] existing at all).

For each CSV not already recorded in tools/.budget_csv_manifest.txt:
  - if it doesn't have the MAX+SUM+COUNT debug[384] layout (main.h -- e.g. a
    capture from before this scheme existed, when debug[] was a single
    smaller block with different semantics), skip merging it entirely -- an
    older layout's numbers don't mean the same thing and merging them in
    unchecked is exactly how budget[] got corrupted once already (see git
    history around 2026-08-07) -- but still record it as processed so it's
    never retried
  - otherwise merge MAX and AVERAGE for every budget[] entry as above, plus
    OVERTIME wherever the CSV actually has debugOvertime[] rows (older
    debug[]-only captures just merge MAX/AVERAGE and leave any existing
    "(overtime by N)" alone)
  - whenever a line's token and/or "(avg: N)" and/or "(overtime by N)"
    actually changes, the line's trailing comment gets a " -- updated
    YYYY-MM-DD HH:MM TZ (was X[, avg was Y][, overtime was Z])" stamp -- only
    the piece(s) that actually changed appear in "(was ...)"; any hand-
    written free-text comment already on the line (e.g. CH_TELEPORT's "(PH4,
    idle sparkle -- not yet measured)") is preserved verbatim, after the
    "(avg: N)" slot; untouched lines keep whatever stamp (or lack of one)
    they already had
  - the '// Last updated: ...' stamp above budget[128] is refreshed whenever
    a new CSV is processed, whether or not it actually changed any budget[]
    values -- it is not touched on runs with no new CSVs
  - git add + commit main/board.c and the manifest for each CSV processed
    (never a blanket `git add .`, since other unrelated files in this tree
    may have their own in-progress uncommitted changes)
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
# token is everything up to the single comma before the trailing comment --
# covers "_untimed_", "_nop_" and "_B + 330" (which contains spaces, so the
# token group can't be restricted to \S+). An optional "(overtime by N) "
# prefix can sit right after "//", before the index. `rest` is everything
# after the index, unparsed (name + optional "(avg: N)" + optional hand
# comment + optional "-- updated ..." stamp, in whatever order they
# currently appear).
ENTRY_RE = re.compile(r"^(\s*)(.+?),(\s*)//\s*(?:\(overtime by (\d+)\)\s+)?(\d+)\s+(.*)$")
BASE_TOKEN_RE = re.compile(r"^_B\s*\+\s*(\d+)$")
STAMP_RE = re.compile(r"^// Last updated: .*$")
STAMP_TAIL_RE = re.compile(r"\s*--\s*updated\s+.*$")
AVG_RE = re.compile(r"\(avg:\s*(\d+)\)")
UNTIMED_TOKEN = "_untimed_"
NOP_TOKEN = "_nop_"

DEBUG_NAME_RE = re.compile(r"^debug\[(\d+)\]$")
DEBUG_OVERTIME_NAME_RE = re.compile(r"^debugOvertime\[(\d+)\]$")

# Sentinel index that only exists in the current 3-block (MAX/SUM/COUNT)
# debug[384] layout (main.h) -- its presence in a CSV is how we tell "new
# layout, safe to merge MAX/AVERAGE" from "older/incompatible capture,
# don't touch budget[] with it".
NEW_LAYOUT_SENTINEL = 383


def parse_csv(path):
    """Return (debug_values, overtime_values): {index: value} dicts for
    every debug[N] and debugOvertime[N] row in the CSV, respectively."""
    debug_values = {}
    overtime_values = {}
    with open(path, newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames:
            reader.fieldnames = [n.strip() for n in reader.fieldnames]
        for row in reader:
            name = (row.get("Name") or "").strip()
            m = DEBUG_NAME_RE.match(name)
            target = debug_values
            if not m:
                m = DEBUG_OVERTIME_NAME_RE.match(name)
                target = overtime_values
            if not m:
                continue
            idx = int(m.group(1))
            raw = (row.get("Value") or "").strip().lower()
            if not raw:
                continue
            if raw.startswith("0x"):
                raw = raw[2:]
            try:
                # exporter always writes Value as plain hex (fixed-width,
                # zero-padded to the type size), never prefixed with "0x"
                # and never decimal.
                val = int(raw, 16)
            except ValueError:
                continue
            target[idx] = val
    return debug_values, overtime_values


def has_new_layout(debug_values):
    return NEW_LAYOUT_SENTINEL in debug_values


def format_entry(token, idx, rest, overtime_s):
    prefix = f"(overtime by {overtime_s}) " if overtime_s is not None else ""
    return "    " + (token + ",").ljust(14) + "// " + prefix + str(idx).rjust(3) + " " + rest


def format_value_token(value):
    return f"_B + {value}"


def parse_token(token):
    """Return the raw measured MAX value N (an int), or None for _untimed_/
    _nop_ (and, as a degenerate fallback, for anything else unparseable)."""
    if token in (UNTIMED_TOKEN, NOP_TOKEN):
        return None
    m = BASE_TOKEN_RE.match(token)
    if m:
        return int(m.group(1))
    try:
        return int(token)  # legacy bare-int fallback, pre-_B migration
    except ValueError:
        return None


def split_rest(rest):
    """Split a budget[] line's trailing comment (everything after the index)
    into (name, extra_hand_comment, cur_avg_or_None, old_stamp_or_'')."""
    stamp_m = STAMP_TAIL_RE.search(rest)
    body = rest[: stamp_m.start()] if stamp_m else rest
    old_stamp = stamp_m.group(0) if stamp_m else ""

    avg_m = AVG_RE.search(body)
    cur_avg = avg_m.group(1) if avg_m else None
    body = AVG_RE.sub("", body)
    body = re.sub(r"\s+", " ", body).strip()

    parts = body.split(None, 1)
    name = parts[0] if parts else ""
    extra = parts[1] if len(parts) > 1 else ""
    return name, extra, cur_avg, old_stamp


def build_rest(name, extra, avg_s, stamp):
    parts = [name]
    if avg_s is not None:
        parts.append(f"(avg: {avg_s})")
    if extra:
        parts.append(extra)
    return " ".join(parts) + stamp


def merge(board_lines, debug_values, overtime_values):
    """Return (new_lines, list_of_changes). Merges MAX (into the token),
    AVERAGE (into "(avg: N)"), and OVERTIME (into a "(overtime by N)"
    prefix) independently for every budget[] line, from the same CSV, in
    one pass."""
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
            _indent, token, _pad, cur_overtime, idx_s, rest = m.groups()
            idx = int(idx_s)
            name, extra, cur_avg, _old_stamp = split_rest(rest)

            new_token = token
            new_avg = cur_avg
            new_overtime = cur_overtime
            token_changed = False
            avg_changed = False
            overtime_changed = False
            old_token_disp = None
            old_avg_disp = None
            old_overtime_disp = None

            if token != NOP_TOKEN:
                csv_val = debug_values.get(idx)    # MAX block
                current = parse_token(token)
                if csv_val is not None and csv_val != 0:
                    if current is None or csv_val > current:
                        old_token_disp = "untimed" if current is None else str(current)
                        new_token = format_value_token(csv_val)
                        token_changed = True

            total = debug_values.get(idx + 128)    # SUM block
            count = debug_values.get(idx + 256)    # COUNT block
            if count:
                avg_val = (total or 0) // count
                cur_avg_val = int(cur_avg) if cur_avg is not None else None
                if cur_avg_val is None or avg_val > cur_avg_val:
                    old_avg_disp = cur_avg    # None means "first capture, nothing to compare"
                    new_avg = str(avg_val)
                    avg_changed = True

            overtime_val = overtime_values.get(idx)
            if overtime_val:
                cur_overtime_val = int(cur_overtime) if cur_overtime is not None else None
                if cur_overtime_val is None or overtime_val > cur_overtime_val:
                    old_overtime_disp = cur_overtime
                    new_overtime = str(overtime_val)
                    overtime_changed = True

            if not token_changed and not avg_changed and not overtime_changed:
                out.append(line)
                continue

            was_bits = []
            if token_changed:
                was_bits.append(f"was {old_token_disp}")
            if avg_changed and old_avg_disp is not None:
                was_bits.append(f"avg was {old_avg_disp}")
            if overtime_changed and old_overtime_disp is not None:
                was_bits.append(f"overtime was {old_overtime_disp}")
            new_stamp = f" -- updated {stamp_now()}"
            if was_bits:
                new_stamp += " (" + ", ".join(was_bits) + ")"

            new_rest = build_rest(name, extra, new_avg, new_stamp)
            changes.append((idx, name, token, new_token, cur_avg, new_avg, cur_overtime, new_overtime))
            out.append(format_entry(new_token, idx, new_rest, new_overtime) + "\n")
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
        debug_values, overtime_values = parse_csv(path)

        if not has_new_layout(debug_values):
            print(f"skipping {fname}: doesn't have the MAX+SUM+COUNT debug[384] layout (main.h) -- older/incompatible capture, recording as processed, not merging")
            if not dry_run:
                append_manifest(fname)
            continue

        print(f"processing {fname}" + ("" if overtime_values else " (no debugOvertime[] data in this capture)"))

        with open(BOARD_C) as f:
            board_lines = f.readlines()

        new_lines, changes = merge(board_lines, debug_values, overtime_values)
        new_lines = update_stamp(new_lines)  # always stamp the attempt, changed or not

        if changes:
            print(f"  {len(changes)} budget[] entries updated:")
            for idx, name, old_tok, new_tok, old_avg, new_avg, old_ot, new_ot in changes:
                bits = []
                if old_tok != new_tok:
                    bits.append(f"{old_tok} -> {new_tok}")
                if old_avg != new_avg:
                    bits.append(f"avg {old_avg or '(none)'} -> {new_avg}")
                if old_ot != new_ot:
                    bits.append(f"overtime {old_ot or '(none)'} -> {new_ot}")
                print(f"    [{idx:3d}] {name}: " + ", ".join(bits))
        else:
            print("  no changes (all csv values already <= current budget[])")

        if dry_run:
            continue

        with open(BOARD_C, "w") as f:
            f.writelines(new_lines)

        append_manifest(fname)

        git(["add", "main/board.c", "tools/.budget_csv_manifest.txt"])
        if changes:
            msg = f"budget[]: merge {len(changes)} update(s) from {fname}"
        else:
            msg = f"budget[]: record {fname} as processed (no changes)"
        r = git(["commit", "-m", msg])
        print("  " + r.stdout.strip().splitlines()[0] if r.stdout.strip() else "  git commit produced no output")


if __name__ == "__main__":
    main()
