# Matching

This page lists the tools for working on a function, the conventions every
change keeps, and where things are in the repository. Build first: see
[Building](Building.md).

## Tools

| tool | what it does |
|---|---|
| `tools/funcdiff.py us main` | C functions against retail, with disassembly diffs |
| `tools/objdiff_expected.py us`, `tools/objdiff_config.py us` | expected objects and `objdiff.json` for objdiff |
| `tools/m2c_func.py`, `tools/trymatch.py`, `tools/splice.py` | a first draft from m2c, match attempts, splicing a function into its file |
| `tools/fullasm.py` | every function's assembly, matched ones too |
| `tools/boundaries.py`, `tools/split_c.py`, `tools/add_c_file.py` | file boundaries: the evidence, splitting a C file, carving a new one |
| `tools/datacheck.py` | a C file's data sections against retail |
| `tools/share_symbols.py`, `tools/map_versions.py`, `tools/version_fallback.py` | sharing names and C between US and JP |

## Conventions

- A function counts as matched only when objdiff says it's exact.
- No `.word` or hex bodies.
- GTE operations only through the macros in `include/gte.h`; stack switching
  and `$ra` capture only through `include/stack.h`; the debugger poll only
  through `include/pollhost.h`; the inline-asm ordering-table link only through
  `include/ot.h`.
- Struct fields are tagged with their offsets, for example `m1C_position`.
- US and JP build from the same C. JP-only differences go under
  `#if VERSION_JP`.
- The `#ifdef PLATFORM_PC` blocks are for the
  [PC port](https://github.com/EtherAura/xenogears-pc) and compile to nothing
  on the PlayStation.

## Layout

| path | contents |
|---|---|
| `config/<ver>/` | per-version disc manifest, splat configs and symbol lists |
| `config/build.yaml` | compiler presets by path |
| `src/`, `include/` | decompiled C, shared across versions |
| `tools/` | setup, extraction, build and matching scripts |

## Sending a change

[CONTRIBUTING](../CONTRIBUTING.md) says what a change has to pass and how a
pull request is handled.
