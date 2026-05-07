#!/usr/bin/env python3
"""
Reads DISPLAY_SIZE, ROM_SIZE, C_START from an ASM file and updates
ORIGIN/LENGTH values in a linker script (.lds file).
"""

import re
import sys
from pathlib import Path


def parse_asm_file(asm_path: Path) -> dict[str, int]:
    """Extract DISPLAY_SIZE, ROM_SIZE, C_START from ASM file."""
    required = {"DISPLAY_SIZE", "ROM_SIZE", "C_START"}
    values = {}

    # Matches:  LABEL   EQU   0x1234  (or decimal, or $1234)
    pattern = re.compile(
        r"^\s*(\w+)\s+(?:EQU|equ|=)\s+(0[xX][0-9A-Fa-f]+|\$[0-9A-Fa-f]+|\d+)",
        re.MULTILINE,
    )

    text = asm_path.read_text()
    for match in pattern.finditer(text):
        label, raw = match.group(1), match.group(2)
        if label in required:
            if raw.startswith("$"):
                values[label] = int(raw[1:], 16)
            else:
                values[label] = int(raw, 0)  # handles 0x... and decimal

    missing = required - values.keys()
    if missing:
        raise ValueError(f"ASM file missing required labels: {missing}")

    return values


def update_lds_file(lds_path: Path, asm_values: dict[str, int]) -> None:
    """Update ORIGIN and LENGTH values in the linker script."""
    display_size = asm_values["DISPLAY_SIZE"]
    rom_size     = asm_values["ROM_SIZE"]
    c_start      = asm_values["C_START"]

    # Derived values
    boot_origin    = 0x0000
    c_code_origin  = c_start
    c_code_length  = rom_size - c_start - display_size
    ram_origin     = rom_size
    ram_length     = 0x10000 - rom_size  # assumes 64 KB address space; adjust if needed

    def hex4(n: int) -> str:
        return f"0x{n:04X}"

    # Each rule: (section_name, field, new_value_hex)
    updates = [
        ("boot",   "ORIGIN", hex4(boot_origin)),
        ("C_code", "ORIGIN", hex4(c_code_origin)),
        ("C_code", "LENGTH", hex4(c_code_length)),
        ("ram",    "ORIGIN", hex4(ram_origin)),
        ("ram",    "LENGTH", hex4(ram_length)),
    ]

    text = lds_path.read_text()

    for section, field, new_val in updates:
        # Matches lines like:   boot (rx) : ORIGIN = 0x0000, LENGTH = 0x1000
        # Captures everything before the value so we can replace just the value.
        pat = re.compile(
            rf"(^\s*{re.escape(section)}\b.*?{field}\s*=\s*)(0[xX][0-9A-Fa-f]+|\d+)",
            re.MULTILINE | re.IGNORECASE,
        )
        new_text, count = pat.subn(rf"\g<1>{new_val}", text)
        if count == 0:
            raise ValueError(f"Could not find '{section}' / '{field}' in {lds_path}")
        text = new_text

    lds_path.write_text(text)
    print(f"Updated {lds_path}:")
    for section, field, val in updates:
        print(f"  {section:<8} {field:<8} = {val}")


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <asm_file> <lds_file>")
        sys.exit(1)

    asm_path = Path(sys.argv[1])
    lds_path = Path(sys.argv[2])

    for p in (asm_path, lds_path):
        if not p.exists():
            print(f"Error: file not found: {p}")
            sys.exit(1)

    asm_values = parse_asm_file(asm_path)
    print("Parsed from ASM:")
    for k, v in asm_values.items():
        print(f"  {k} = 0x{v:04X}")

    update_lds_file(lds_path, asm_values)


if __name__ == "__main__":
    main()
    