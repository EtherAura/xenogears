## What changed

<!-- One logical change per pull request: a function, a file, a tool. -->

## How it was verified

- [ ] `ninja` rebuilds every module and each one still matches retail, US and JP
- [ ] `python tools/check_bss.py` passes
- [ ] A newly matched function is exact in objdiff, with no `.word` or hex bodies
- [ ] This is my own work: nothing is copied from another decompilation of the game

<!-- Accepted changes are applied to the development tree this repository is published
     from and appear here with the next publish. See CONTRIBUTING.md. -->
