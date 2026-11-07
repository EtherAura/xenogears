#!/usr/bin/env python3
"""Print source-file boundary evidence for a range of a module's functions.

Per function, in address order:
  li   which assembler: `ori rX,$zero,imm` (ASPSX < 2.50) vs `addiu rX,$zero,
       imm` with a positive imm (ASPSX >= 2.50)
  gp   globals reached through $gp: the file defines them
  hilo small-data globals (inside the $gp window) reached directly through
       %hi/%lo: the file does NOT define them
  jtbl jump tables and their address mod 8 (the assembler 8-aligns them
       relative to the start of the object's .rodata)
Then conflicts: a global reached via $gp in one function and directly via
%hi/%lo in another means the two are in different files.

With --rodata the range is read-only data instead: each label, its address
mod 8, and the functions that reference it.

Reads work/fullasm/<ver>/<module>/ (run tools/fullasm.py first).

    python tools/boundaries.py us main 0x80025FA8 0x80027D40
    python tools/boundaries.py us main 0x800188CC 0x80018BF0 --rodata
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# $gp is set once by main and stays put while overlays run.
GP = {"us": 0x80059170, "jp": 0x80058810}

INSN = re.compile(r"^\s*/\* \w+ ([0-9A-F]{8}) [0-9A-F]{8} \*/\s+(\w+)\s*(.*)$")


def functions(asm_dir: Path):
    for f in asm_dir.rglob("*.s"):
        text = f.read_text()
        for m in re.finditer(r"^glabel (\w+)\n(.*?)^endlabel", text, re.S | re.M):
            insns = []
            for line in m.group(2).splitlines():
                i = INSN.match(line)
                if i:
                    insns.append((int(i.group(1), 16), i.group(2), i.group(3)))
            if insns:
                yield insns[0][0], m.group(1), insns


def sym_addr(name: str) -> int | None:
    m = re.search(r"_([0-9A-F]{8})$", name)
    return int(m.group(1), 16) if m else None


def analyse(insns, gp_value: int):
    ori = addiu = 0
    gp, hilo, jtbl = set(), set(), set()
    hi_reg: dict[str, str] = {}     # register -> symbol loaded by lui %hi
    for _, op, args in insns:
        if op == "ori" and ", $zero, " in args:
            ori += 1
        if op == "addiu" and ", $zero, " in args and not args.split(", ")[-1].startswith("-"):
            addiu += 1
        for s in re.findall(r"%gp_rel\((\w+)\)", args):
            gp.add(s)
        for s in re.findall(r"%(?:hi|lo)\((jtbl_\w+)\)", args):
            jtbl.add(s)
        regs = re.findall(r"\$\w+", args)
        m = re.match(r"(\$\w+), %hi\((\w+)\)", args)
        if op == "lui" and m:
            hi_reg[m.group(1)] = m.group(2)
            continue
        m = re.search(r"%lo\((\w+)\)\((\$\w+)\)", args)
        if m and hi_reg.get(m.group(2)) == m.group(1):
            a = sym_addr(m.group(1))
            if a is not None and gp_value - 0x8000 <= a < gp_value + 0x8000:
                hilo.add(m.group(1))
        # Any other write to a register forgets what lui put there.
        if regs and op not in ("sw", "sh", "sb", "swc2", "beq", "bne", "jr", "jalr"):
            hi_reg.pop(regs[0], None)
    return ori, addiu, gp, hilo, jtbl


def rodata(asm_dir: Path, start: int, end: int) -> None:
    users = defaultdict(set)
    for _, name, insns in functions(asm_dir):
        for _, _, a in insns:
            for s in re.findall(r"%(?:hi|lo|gp_rel)\((\w+)\)", a):
                users[s].add(name)
    labels = set()
    for f in asm_dir.rglob("*.s"):
        labels.update(re.findall(r"^dlabel (\w+)$", f.read_text(), re.M))
    for label in sorted(labels, key=lambda l: sym_addr(l) or 0):
        a = sym_addr(label)
        if a is not None and start <= a < end:
            print(f"{a:08X} %8={a % 8} {label:16} {', '.join(sorted(users[label])) or '-'}")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("version")
    ap.add_argument("module")
    ap.add_argument("start", type=lambda s: int(s, 16))
    ap.add_argument("end", type=lambda s: int(s, 16))
    ap.add_argument("--rodata", action="store_true", help="list read-only data labels instead")
    args = ap.parse_args()

    asm_dir = ROOT / "work" / "fullasm" / args.version / args.module / "asm"
    if not asm_dir.is_dir():
        sys.exit(f"missing {asm_dir.relative_to(ROOT)}; run tools/fullasm.py {args.version} {args.module}")
    gp_value = GP[args.version]
    if args.rodata:
        rodata(asm_dir, args.start, args.end)
        return

    rows = []
    seen = set()
    for addr, name, insns in sorted(functions(asm_dir)):
        if args.start <= addr < args.end and addr not in seen:
            seen.add(addr)
            rows.append((addr, name, len(insns) * 4, *analyse(insns, gp_value)))

    by_gp, by_hilo = defaultdict(list), defaultdict(list)
    for addr, name, size, ori, addiu, gp, hilo, jtbl in rows:
        li = "ori" if ori and not addiu else "addiu" if addiu and not ori else f"{ori}/{addiu}" if ori else "-"
        jt = " ".join(f"{j}%8={sym_addr(j) % 8}" for j in sorted(jtbl))
        print(f"{addr:08X} {size:5X} {name:32} li={li:6} gp={','.join(sorted(gp)) or '-'}"
              f" hilo={','.join(sorted(hilo)) or '-'} {jt}")
        for s in gp:
            by_gp[s].append(name)
        for s in hilo:
            by_hilo[s].append(name)

    conflicts = sorted(set(by_gp) & set(by_hilo))
    if conflicts:
        print("\nconflicts ($gp vs %hi/%lo => different files):")
        for s in conflicts:
            print(f"  {s}: gp in {', '.join(by_gp[s])}; hi/lo in {', '.join(by_hilo[s])}")


if __name__ == "__main__":
    main()
