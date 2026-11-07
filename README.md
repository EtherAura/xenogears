# Xenogears decompilation

Part of -Project Exodus-, an unofficial Xenogears preservation effort.

A matching decompilation of Xenogears (PlayStation, 1998). The C here compiles
back to byte-identical copies of every executable and code overlay of the US
(SLUS-00664) and Japanese (SLPS-01160) releases, 50 files in all. About 98% of
the code is C; the rest, mostly in `main`, is still assembly taken from your
disc when you build.

No game data is included. You need your own discs.

## Setup

You need Linux x86-64 with bash, curl, git, make, gcc/cpp, Python 3 and Ninja.
No root access is needed.

```bash
tools/setup.sh                       # toolchain -> tools/toolchain, Python deps -> .venv
source .venv/bin/activate            # fish: source .venv/bin/activate.fish
```

Put your disc images under `bin/` with the Redump names listed in
`config/<version>/disc.yaml`, then:

```bash
python tools/extract.py us     # the code files from your disc, checked against retail
python tools/configure.py us   # split every module with splat, write build.ninja
ninja                          # rebuild every module and check it against retail
python tools/check_bss.py      # statics at their addresses (the checksum can't see .bss)
python tools/progress.py       # matched code per module
```

Use `jp` for the Japanese release, or leave the version out to do both.

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
[EtherAura/xenogears-pc](https://github.com/EtherAura/xenogears-pc); the
`#ifdef PLATFORM_PC` blocks are for it and compile to nothing on the
PlayStation.

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

GPL-3.0 or later, in `LICENSE`, for this project's own work: the C, headers,
configs and tools. Xenogears is © Square Enix; this project isn't affiliated
with them, and no game data is included.
