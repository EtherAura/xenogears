English | [日本語](README.ja.md)

<div align="center">

# Xenogears decompilation

**Preserve the game. Rebuild it byte for byte.**

[![Tool checks](https://github.com/EtherAura/xenogears/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/EtherAura/xenogears/actions/workflows/ci.yml)
[![CodeQL](https://github.com/EtherAura/xenogears/actions/workflows/codeql.yml/badge.svg)](https://github.com/EtherAura/xenogears/actions/workflows/codeql.yml)
[![GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)
![US and JP](https://img.shields.io/badge/retail-US%20%2B%20JP-627eaa)
[![Listed on Reversed with AI](https://reversedwithai.com/badge.svg)](https://reversedwithai.com/p/xenogears-native-pc-port-from-matching-decomp-claude)

A matching decompilation of Xenogears for PlayStation, with shared C source for the US and Japanese releases.
Part of **-Project Exodus-**, an unofficial preservation effort.

[**Website**](https://exodus.agartha.digital/) · [**Build from your discs**](docs/Building.md) · [**Matching guide**](docs/Matching.md) · [**PC port**](https://github.com/EtherAura/xenogears-pc) · [**Contribute**](CONTRIBUTING.md)

![Workflow from your own discs through extraction and rebuilding to retail byte comparisons](docs/img/matching.svg)
</div>

**Bring your own discs.** No game data is included. GitHub CI checks tools and configuration;
retail matching is checked locally against your images.

**Matching progress: 100%.** Every function of both releases is matched. About 98.2% of
the code bytes are C that compiles to the retail bytes. The other 1.8% was written by hand
in assembly in the original (Sony's PSY-Q library routines, and the game's own GTE render
loops, LZSS decoder and a few helpers), so it stays assembly, taken from your disc when
you build. All 25 retail code modules per release rebuild byte-for-byte.
[Measure it](tools/progress.py) with the retail-verified builds;
[config/handwritten.yaml](config/handwritten.yaml) lists the hand-written routines and
the evidence for each.

<a id="original-work"></a>
**Original work.** The C here was written for this project, with AI assistance (Claude),
from the retail program's machine code, and a function counts as matched only when it
compiles to the retail bytes.
Nothing was copied from another decompilation of the game. Other projects, the hardware
documentation and emulators were references for facts only: an address, which compiler
built a file, how the hardware behaves. Function names follow yaz0r's
[Noah](https://github.com/yaz0r/Noah) where it names the same code;
[Tools and project credits](docs/Tooling.md) credits the tools and references.

**Build hosts.** The published build guide targets Linux x86-64. A FreeBSD 15.1 amd64
test with local compatibility fixes rebuilt and verified all 25 retail code modules per release.

[Tools and project credits](docs/Tooling.md) · [Security](SECURITY.md) · [Code of conduct](CODE_OF_CONDUCT.md) · [Discussions](https://github.com/EtherAura/xenogears/discussions)

<sub>Project work: GPL-3.0 or later. Xenogears © Square Enix. Unofficial and unaffiliated.</sub>
