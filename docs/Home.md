# Xenogears decompilation

Part of -Project Exodus-, an unofficial Xenogears preservation effort.

This is a matching decompilation of Xenogears (PlayStation, 1998). The C in
this repository compiles back to byte-identical copies of every executable and
code overlay of the US (SLUS-00664) and Japanese (SLPS-01160) releases, 50
files in all. About 98% of the code is C. The rest, mostly in `main`, is still
assembly taken from your disc when you build.

No game data is included. You need your own discs.

## Pages

| page | what it covers |
|---|---|
| [Building](Building.md) | What you need, where your disc images go, and the commands from a fresh clone to a build that matches retail |
| [Matching](Matching.md) | The tools for working on a function, the conventions a change has to keep, and how the repository is laid out |

## In the repository

- [README](../README.md): the same ground in one page, and the projects this one is built with.
- [CONTRIBUTING](../CONTRIBUTING.md): what a change has to pass, and how a pull request is handled.
- [SECURITY](../SECURITY.md): how to report a vulnerability.

## The PC port

The same C also builds as a native PC port,
[EtherAura/xenogears-pc](https://github.com/EtherAura/xenogears-pc). The
`#ifdef PLATFORM_PC` blocks in the source are for it and compile to nothing on
the PlayStation.

## Licence

GPL-3.0 or later, in [LICENSE](../LICENSE), for this project's own work: the C,
headers, configs and tools. Xenogears is © Square Enix. This project isn't
affiliated with them, and no game data is included.
