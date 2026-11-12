# Tooling and acknowledgements

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

## Repository layout

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
