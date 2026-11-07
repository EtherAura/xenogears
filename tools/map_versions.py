#!/usr/bin/env python3
"""Pair a module's functions and data symbols between two versions.

Functions are compared with every symbol and label operand masked out, so
code that only moved (different addresses) compares equal. The two function
lists are aligned in address order; functions of equal masked code pair up,
and the symbols their operands reference pair up too (US data symbol <->
JP address). Reads work/fullasm/<ver>/<module>/ (run tools/fullasm.py for
both versions first).

    python tools/map_versions.py us jp main [--report] [--symbols out.txt]

--symbols writes symbol-file lines giving the first version's names at the
second version's addresses, for every paired name that is not an address
name of the second version already.
"""

from __future__ import annotations

import argparse
import difflib
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INSN = re.compile(r"^\s*/\* \w+ ([0-9A-F]{8}) [0-9A-F]{8} \*/\s+(\w+(?:\.\w+)?)\s*(.*)$")
SYMREF = re.compile(r"%(?:hi|lo|gp_rel)\(([A-Za-z_]\w*)(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?\)")


def parse(asm_dir: Path):
    """Yield (addr, name, masked_insns, symrefs) per function."""
    funcs = {}
    for f in sorted(asm_dir.rglob("*.s")):
        text = f.read_text()
        for m in re.finditer(r"^glabel (\w+)\n(.*?)^endlabel", text, re.S | re.M):
            masked, refs, addr = [], [], None
            for line in m.group(2).splitlines():
                i = INSN.match(line)
                if not i:
                    continue
                if addr is None:
                    addr = int(i.group(1), 16)
                op, args = i.group(2), i.group(3)
                here = [(s, int(off, 0) if off else 0) for s, off in SYMREF.findall(args)]
                a = SYMREF.sub("%sym", args)
                if op in ("jal", "j") and not a.startswith("$"):
                    here.append((a.strip(), 0))
                    a = "sym"
                refs.append(tuple(here))
                a = re.sub(r"\.L[0-9A-F_]+\w*", "L", a)          # branch labels
                # li of a small positive constant: ori (ASPSX < 2.50) or addiu
                lm = re.fullmatch(r"(\$\w+), \$zero, (0x[0-9A-Fa-f]+|\d+)", a)
                if op in ("ori", "addiu") and lm:
                    op, a = "li", f"{lm.group(1)}, {lm.group(2)}"
                a = re.sub(r"\b(?:func|D|jtbl)_[0-9A-F]{8}\b", "sym", a)
                masked.append(f"{op} {a}")
            if addr is not None and addr not in funcs:
                funcs[addr] = (addr, m.group(1), tuple(masked), refs)
    return [funcs[a] for a in sorted(funcs)]


def sym_addr(name: str, table: dict[str, int]) -> int | None:
    if name in table:
        return table[name]
    m = re.search(r"_([0-9A-F]{8})$", name)
    return int(m.group(1), 16) if m else None


MARK = "// ---- Names shared with "


def load_names(version: str, module: str) -> tuple[dict[str, int], dict[str, int]]:
    """(hand-written names, names in the generated section) of a symbol file."""
    base, generated = {}, {}
    p = ROOT / "config" / version / "symbols" / f"{module}.txt"
    target = base
    for line in p.read_text().splitlines() if p.is_file() else []:
        if line.startswith(MARK):
            target = generated
        m = re.match(r"\s*(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;", line)
        if m:
            target[m.group(1)] = int(m.group(2), 16)
    return base, generated


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("module")
    ap.add_argument("--report", action="store_true", help="print every function pair")
    ap.add_argument("--symbols", type=Path, help="write symbol lines for version b")
    ap.add_argument("--b-asm", type=Path, help="version b disassembly to map against "
                    "(default work/fullasm/<b>/<module>/asm)")
    args = ap.parse_args()

    fa = parse(ROOT / "work" / "fullasm" / args.a / args.module / "asm")
    fb = parse(args.b_asm or ROOT / "work" / "fullasm" / args.b / args.module / "asm")
    names_a = {**load_names(args.a, args.module)[0], **load_names(args.a, args.module)[1]}
    names_b, generated_b = load_names(args.b, args.module)
    # a native disassembly of b knows only the hand-written names
    all_b = dict(names_b) if args.b_asm else {**generated_b, **names_b}

    pairs = []  # (fa_item, fb_item, identical)
    sm = difflib.SequenceMatcher(a=[f[2] for f in fa], b=[f[2] for f in fb], autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            pairs += [(fa[i1 + k], fb[j1 + k], True) for k in range(i2 - i1)]
        elif tag == "replace":
            # pair greedily by similarity, keeping order
            j = j1
            for i in range(i1, i2):
                best, bj = 0.0, None
                for jj in range(j, j2):
                    r = difflib.SequenceMatcher(a=fa[i][2], b=fb[jj][2], autojunk=False).ratio()
                    if r > best:
                        best, bj = r, jj
                if bj is not None and best >= 0.6:
                    pairs.append((fa[i], fb[bj], False))
                    j = bj + 1

    # Functions not paired identically may sit inside a longer function of
    # the other version (splat merged them there): find their code as an
    # exact run inside some function of b.
    exact = {p[0][0] for p in pairs if p[2]}
    index = defaultdict(list)
    for y in fb:
        for k in range(len(y[2])):
            index[y[2][k:k + 3]].append((y, k))
    located = {}
    for x in fa:
        if x[0] in exact or len(x[2]) < 3:
            continue
        hits = [(y, k) for y, k in index.get(x[2][:3], []) if y[2][k:k + len(x[2])] == x[2]]
        if len(hits) == 1:
            y, k = hits[0]
            located[x[0]] = (x, y[0] + 4 * k)
    pairs = [p for p in pairs if p[0][0] not in located or p[2]]

    # Still unplaced: if the function before it is identical in both
    # versions, it starts right after that function in b too.
    placed = {p[0][0]: p[1][0] for p in pairs if p[2]}
    placed.update({a: addr for a, (x, addr) in located.items()})
    paired_a = {p[0][0] for p in pairs}
    for i in range(1, len(fa)):
        x, prev = fa[i], fa[i - 1]
        if x[0] in paired_a or x[0] in located or prev[0] not in placed:
            continue
        if prev[0] not in exact and prev[0] not in located:
            continue
        addr = placed[prev[0]] + 4 * len(prev[2])
        # only where b has a function start, or inside the b function that
        # starts where prev does (splat merged the two there)
        starts = {y[0]: y for y in fb}
        host = starts.get(placed[prev[0]])
        if addr not in starts and not (host and host[0] < addr < host[0] + 4 * len(host[2])):
            continue
        located[x[0]] = (x, addr)
        placed[x[0]] = addr

    # Gaps: an unplaced function whose neighbours are both placed, with exactly
    # one b function between them, is that function (its code changed).
    placed.update({p[0][0]: p[1][0] for p in pairs})
    paired_a = {p[0][0] for p in pairs}
    b_starts = sorted(y[0] for y in fb)
    for i in range(1, len(fa) - 1):
        x, prev, nxt = fa[i], fa[i - 1], fa[i + 1]
        if x[0] in paired_a or x[0] in located or prev[0] not in placed or nxt[0] not in placed:
            continue
        between = [a for a in b_starts if placed[prev[0]] < a < placed[nxt[0]]]
        if len(between) == 1:
            located[x[0]] = (x, between[0])
            placed[x[0]] = between[0]

    # symbol pairs: from every instruction the two versions share (all of an
    # identical function, the matching runs of a similar one)
    votes = defaultdict(Counter)
    def vote(ra, rb):
        for (sa, oa), (sb, ob) in zip(ra, rb):
            ab = sym_addr(sb, all_b)
            if ab is not None:
                votes[sa][ab + ob - oa] += 1
    for x, y, same in pairs:
        if same:
            for ra, rb in zip(x[3], y[3]):
                vote(ra, rb)
        else:
            blocks = difflib.SequenceMatcher(a=x[2], b=y[2], autojunk=False).get_matching_blocks()
            for i0, j0, n in blocks:
                for k in range(n):
                    vote(x[3][i0 + k], y[3][j0 + k])
    # functions placed by position: align with the b code found there
    for x, addr in located.values():
        host = next((y for y in fb if y[0] <= addr < y[0] + 4 * len(y[2])), None)
        if host is None:
            continue
        k = (addr - host[0]) // 4
        bm, br = host[2][k:], host[3][k:]
        for i0, j0, n in difflib.SequenceMatcher(a=x[2], b=bm, autojunk=False).get_matching_blocks():
            for q in range(n):
                vote(x[3][i0 + q], br[j0 + q])

    sym_map = {}
    conflicts = []
    for s, c in votes.items():
        (addr, n), *rest = c.most_common()
        if rest:
            conflicts.append((s, c))
        sym_map[s] = addr
    for x, y, _ in pairs:              # functions themselves
        sym_map[x[1]] = y[0]
    for x, addr in located.values():
        sym_map[x[1]] = addr

    same = sum(1 for p in pairs if p[2])
    print(f"{args.a}: {len(fa)} functions, {args.b}: {len(fb)}; paired {len(pairs)} "
          f"({same} identical after masking, {len(pairs) - same} similar)")
    print(f"symbols paired: {len(sym_map)}; with conflicting votes: {len(conflicts)}")
    for s, c in conflicts[:20]:
        print(f"  conflict {s}: " + ", ".join(f"0x{a:08X}x{n}" for a, n in c.most_common(4)))
    print(f"located inside longer {args.b} functions: {len(located)}")
    unpaired_a = {f[0] for f in fa} - {p[0][0] for p in pairs} - set(located)
    print(f"unpaired {args.a} functions: {len(unpaired_a)}")
    if args.report:
        for x, y, s in pairs:
            print(f"{x[0]:08X} {x[1]:36} -> {y[0]:08X} {y[1]:36} {'=' if s else '~'}")
        for a in sorted(unpaired_a):
            print(f"{a:08X} unpaired")

    if args.symbols:
        by_addr_b = {}
        for n, a in names_b.items():
            by_addr_b.setdefault(a, n)
        func_names = {p[0][1] for p in pairs} | {x[1] for x, _ in located.values()}
        lines, seen_addr, disagree = [], {}, []
        for s, addr in sorted(sym_map.items(), key=lambda kv: kv[1]):
            if re.fullmatch(r"\.L\w+|L", s):
                continue
            if addr in by_addr_b:          # b's own name for that address wins
                if by_addr_b[addr] != s:
                    disagree.append((s, addr, addr))
                continue
            if s in names_b:           # b names it already (e.g. SDK scan): keep b's
                if names_b[s] != addr:
                    disagree.append((s, names_b[s], addr))
                continue
            if addr in seen_addr:
                print(f"  two names for 0x{addr:08X}: {seen_addr[addr]}, {s}", file=sys.stderr)
                continue
            seen_addr[addr] = s
            kind = " // type:func" if s in func_names else ""
            lines.append(f"{s} = 0x{addr:08X};{kind}")
        for n, have, got in disagree:
            print(f"  {n}: {args.b} has 0x{have:08X}, mapping says 0x{got:08X}", file=sys.stderr)
        args.symbols.write_text("\n".join(lines) + "\n")
        print(f"wrote {len(lines)} symbol lines to {args.symbols}")


if __name__ == "__main__":
    main()
