English | [日本語](README.ja.md)

<div align="center">

# Xenogears decompilation

**Preserve the game. Rebuild it byte for byte.**

[![Tool checks](https://github.com/EtherAura/xenogears/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/EtherAura/xenogears/actions/workflows/ci.yml)
[![CodeQL](https://github.com/EtherAura/xenogears/actions/workflows/codeql.yml/badge.svg)](https://github.com/EtherAura/xenogears/actions/workflows/codeql.yml)
[![GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)
![US and JP](https://img.shields.io/badge/retail-US%20%2B%20JP-627eaa)

A matching decompilation of Xenogears for PlayStation, with shared C source for the US and Japanese releases.
Part of **-Project Exodus-**, an unofficial preservation effort.

[**Build from your discs**](docs/Building.md) · [**Matching guide**](docs/Matching.md) · [**PC port**](https://github.com/EtherAura/xenogears-pc) · [**Contribute**](CONTRIBUTING.md)

![Workflow from your own discs through extraction and rebuilding to retail byte comparisons](docs/img/matching.svg)
</div>

**Bring your own discs.** No game data is included. GitHub CI checks tools and configuration;
retail matching is checked locally against your images.

**Matching progress.** About 98.2% of code bytes match in C in the measured US and
Japanese builds. All 25 retail code modules per release rebuild byte-for-byte.
[Measure C coverage](tools/progress.py) with the retail-verified builds.

**Build hosts.** The published build guide targets Linux x86-64. A FreeBSD 15.1 amd64
test with local compatibility fixes rebuilt and verified all 25 retail code modules per release.

[Tools and project credits](docs/Tooling.md) · [Security](SECURITY.md) · [Discussions](https://github.com/EtherAura/xenogears/discussions)

<sub>Project work: GPL-3.0 or later. Xenogears © Square Enix. Unofficial and unaffiliated.</sub>
