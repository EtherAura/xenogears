# Xenogears decompilation

Part of **Project Exodus**, an unofficial Xenogears preservation effort.

A matching decompilation of Xenogears (PlayStation, 1998): C source that
compiles back to byte-identical copies of the game's executable and every code
overlay, for the US release (SLUS-00664) and the Japanese release
(SLPS-01160).

Every module of both versions rebuilds byte for byte (50 images, each
SHA-1-checked against retail). About 98% of the code is C; the rest, most of
it in `main`, is still assembly that the build takes from your disc.

No game data is included. You need your own disc images.

## Setup

Host requirements: Linux x86-64, bash, curl, git, make, gcc/cpp, python3 and
ninja. No root access is needed.

```bash
tools/setup.sh                       # toolchain -> tools/toolchain, Python deps -> .venv
source .venv/bin/activate            # fish: source .venv/bin/activate.fish
```

Put your disc images under `bin/` using the Redump names listed in
`config/<version>/disc.yaml`, then extract the code files:

```bash
python tools/extract.py --check-image      # every version whose disc is present
```

Then split, build and verify:

```bash
python tools/configure.py      # run splat on every module (us + jp), write build.ninja
ninja                          # rebuild every module and sha1-check it against retail
python tools/check_bss.py      # C statics at their addresses (the sha1 can't see .bss)
python tools/progress.py       # matched code per module
```

## Working on a function

| Tool | What it does |
|---|---|
| `tools/funcdiff.py us main` | C functions against retail, with disassembly diffs |
| `tools/objdiff_expected.py us`, `tools/objdiff_config.py us` | Expected objects and `objdiff.json` for objdiff |
| `tools/m2c_func.py`, `tools/trymatch.py`, `tools/splice.py` | A first draft from m2c, match attempts, splicing a function into its file |
| `tools/fullasm.py` | Every function's assembly, matched ones too |
| `tools/boundaries.py`, `tools/split_c.py`, `tools/add_c_file.py` | File boundaries: the evidence, splitting a C file, carving a new one |
| `tools/datacheck.py` | A C file's data sections against retail |
| `tools/share_symbols.py`, `tools/map_versions.py`, `tools/version_fallback.py` | Sharing names and C between US and JP |

US and JP build from the same C. JP-only differences go under `#if VERSION_JP`.
The same C also builds as a native PC port,
[EtherAura/xenogears-pc](https://github.com/EtherAura/xenogears-pc): that is
what the `#ifdef PLATFORM_PC` blocks are for. The PlayStation build never
defines `PLATFORM_PC`, so they compile to nothing there.

## Conventions

- A function counts as matched only when objdiff says it's exact.
- No `.word` or hex bodies.
- GTE operations only through the macros in `include/gte.h`; stack switching
  and `$ra` capture only through `include/stack.h`; the debugger poll only
  through `include/pollhost.h`; the inline-asm ordering-table link only
  through `include/ot.h`.
- Struct fields are tagged with their offsets, e.g. `m1C_position`.

## Layout

| Path | Contents |
|---|---|
| `config/<ver>/` | Per-version disc manifest, splat configs and symbol lists |
| `config/build.yaml` | Compiler presets by path |
| `src/`, `include/` | Decompiled C, shared across versions |
| `tools/` | Setup, extraction, build and matching scripts |

Generated and never committed: `disc/` (extracted retail files), `asm/`,
`linker/`, `build/`, `expected/`, `tools/toolchain/`, `.venv/`.

## Thanks

Built with [splat](https://github.com/ethteck/splat),
[spimdisasm](https://github.com/Decompollaborate/spimdisasm),
[maspsx](https://github.com/mkst/maspsx), the
[decompals old-gcc](https://github.com/decompals/old-gcc) builds,
[objdiff](https://github.com/encounter/objdiff),
[m2c](https://github.com/matt-kempster/m2c) and
[psx_psyq_signatures](https://github.com/lab313ru/psx_psyq_signatures).
[psx-spx](https://psx-spx.consoledev.net/) and yaz0r's
[Noah](https://github.com/yaz0r/Noah) were references for the hardware and
the game's behaviour.

## Licence

This project's own work (the C, headers, configs and tools) is free software
under the GNU General Public License, version 3 or (at your option) any later
version; the text is in `LICENSE`.

Xenogears is © Square Enix. This project is not affiliated with or endorsed
by Square Enix, and the licence does not cover the game: its code and data
come from your own discs and are never part of this repository.
