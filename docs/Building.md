English | [日本語](Building-ja.md)

# Building

This page takes you from a fresh clone to a build that matches the retail game.
Follow the steps below with your own disc images.

## What you need

- Linux x86-64 with bash, curl, git, make, gcc/cpp, Python 3 and Ninja. No root
  access is needed.
- Your own disc images of the US (SLUS-00664) or Japanese (SLPS-01160) release.
  Nothing from the game is in this repository.

On FreeBSD 15.1 amd64, the native bootstrap was exercised with Bash, curl, Git,
GNU make (`gmake`), Texinfo, Ninja, and Python 3.12. The pinned `cc1` and
`objdiff-cli` executables use the `linux` and `linux64` compatibility modules.
The bootstrap sets the Linux ELF brand on the installed `objdiff-cli` copy and
checks the original download against the upstream hash.

With the FreeBSD packages `rust`, `py312-pip`, `py312-maturin`, `py312-wheel`,
and `py312-setuptools` installed, build the `crunch64` dependency before setup.
Then run the standard project bootstrap as a regular user.

```bash
python3.12 -m venv .venv
PYTHONPATH=/usr/local/lib/python3.12/site-packages \
  .venv/bin/python -m pip install --no-build-isolation crunch64==0.6.2
bash tools/setup.sh
```

The FreeBSD US and JP retail-matching checks are tracked in `xg-q58j`.

## 1. Set up the toolchain

```bash
tools/setup.sh                       # toolchain -> tools/toolchain, Python deps -> .venv
source .venv/bin/activate            # fish: source .venv/bin/activate.fish
```

`tools/setup.sh` puts the PlayStation toolchain in `tools/toolchain` and the
Python packages in `.venv`, both inside the clone.

## 2. Put your disc images in place

Put the images under `bin/`, with the Redump names listed in
`config/<version>/disc.yaml`.

## 3. Extract, configure, build

```bash
python tools/extract.py us     # the code files from your disc, checked against retail
python tools/configure.py us   # split every module with splat, write build.ninja
ninja                          # rebuild every module and check it against retail
python tools/check_bss.py      # statics at their addresses (the checksum can't see .bss)
python tools/progress.py       # matched code per module
```

Use `jp` for the Japanese release, or leave the version out to do both. US and
JP build from the same C; JP-only differences are under `#if VERSION_JP`.

| step | what it does |
|---|---|
| `tools/extract.py` | takes the code files from your disc and checks them against retail |
| `tools/configure.py` | splits every module with splat and writes `build.ninja` |
| `ninja` | rebuilds every module and checks each against retail |
| `tools/check_bss.py` | checks that the statics sit at their addresses, which the checksum cannot see |
| `tools/progress.py` | reports the matched code per module |

## What the build leaves behind

These are generated and never committed: `disc/` (the extracted retail files),
`asm/`, `linker/`, `build/`, `expected/`, `tools/toolchain/` and `.venv/`.

## Next

[Matching](Matching.md) covers the tools for working on a function and the
conventions a change has to keep.
