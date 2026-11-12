# Security

## Reporting a vulnerability

**[Open a private security advisory](https://github.com/EtherAura/xenogears/security/advisories/new).**
Private vulnerability reporting is enabled on this repository: the report stays
between you and the maintainer until an advisory is published.

Please do not open a regular issue for a suspected vulnerability. Issues are
public the moment they are filed.

Say what you found, how to reproduce it and what it lets someone do. A fix is
welcome and not required.

## What counts here

This repository is source code and build tools. It ships no executable and no
game data, and nothing in it runs unless you run it. What it does on your
machine:

- `tools/setup.sh` downloads the PlayStation toolchain into `tools/toolchain`
  and the Python packages into `.venv`, without root. The downloads are pinned
  by checksum.
- The other tools read your own disc images and the files extracted from them,
  and write inside this tree.

So a vulnerability is, for example: a download that is not checked against its
pin, a tool that writes outside the tree, or a tool that can be made to run
something by a crafted disc image or configuration file.

Not vulnerabilities: bugs of the game itself (they are reproduced on purpose:
the build is byte-identical to retail), and anything that needs a modified
copy of this repository to work.

## Supported versions

The newest commit on `main`. There are no releases.
