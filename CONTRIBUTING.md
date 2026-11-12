# Contributing

Thank you for considering it. Matched functions, better names, fixes to the
tools and corrections to the documentation are all welcome.

## Before you start

- **You need your own discs.** Nothing from the game is in this repository,
  and nothing from the game may be added to it: no disc images, no extracted
  files, no dumps of its data, in a pull request or in an issue.
- **This is clean-room work.** What you contribute must be your own, written
  from the retail game's code and behaviour. Do not copy code, headers or
  configuration from another decompilation of the game. Other projects, the
  hardware documentation and emulators are fine as references for facts: an
  address, which compiler built a file, how the hardware behaves.
- **Set up and build first.** [Building](docs/Building.md) walks through
  the whole procedure step by step.

## What a change has to pass

1. `ninja` rebuilds every module and each one still matches retail, for both
   releases. A change that makes any module differ by one byte is not ready.
2. `python tools/check_bss.py` passes. The checksum cannot see `.bss`, so this
   is what checks the statics.
3. The conventions in [Matching](docs/Matching.md) hold: a function is matched only when objdiff says it is exact, no `.word`
   or hex bodies, the hardware idioms only through their headers, struct fields
   tagged with their offsets.

CI here cannot run the build, because no runner has a disc. It checks only
that the tools compile and the configuration parses. The matching build is
yours to run before you open the pull request, and it is run again before
anything is published.

## How a pull request is handled

This repository is published from a development tree, one commit at a time. A
pull request is read and discussed here. When it is accepted, the change is
applied to the development tree and appears on `main` with the next publish,
with you credited in the commit message. It is not merged with the merge
button, so do not be surprised when it is closed with a link to that commit.

One logical change per pull request keeps that simple: a function, a file, a
tool.

## Questions

[Discussions](https://github.com/EtherAura/xenogears/discussions) is the place
for questions and for talking a function through. Problems with the PC port
belong in [its own repository](https://github.com/EtherAura/xenogears-pc).

## Licence

GPL-3.0 or later. By contributing you agree that your contribution is licensed
under the same terms.
