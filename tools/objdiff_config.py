#!/usr/bin/env python3
"""Write an objdiff project file (objdiff.json) for one version: a unit per C file.

base_path is the object the build makes (build/<ver>/src/<module>/<file>.c.o),
target_path the expected object tools/objdiff_expected.py assembles from
retail for the same file (the same path under its output folder). Each unit's
progress category is its module. Paths are relative to the config's folder,
which is the project folder objdiff loads; at the repo root the GUI can also
rebuild base objects with ninja.

    python tools/objdiff_expected.py us                  # build/us/expected/
    python tools/objdiff_config.py us                    # objdiff.json
    tools/toolchain/bin/objdiff-cli report generate -p . -o build/us/report.json
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

from objdiff_expected import ROOT, c_objects, modules


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("version")
    ap.add_argument("--expected", type=Path, help="expected objects (default: build/<ver>/expected)")
    ap.add_argument("-o", "--output", type=Path, default=ROOT / "objdiff.json")
    args = ap.parse_args()
    ver = args.version
    expected = (args.expected or ROOT / "build" / ver / "expected").resolve()
    project = args.output.resolve().parent

    def rel(path: Path) -> str:
        return Path(os.path.relpath(path, project)).as_posix()

    units, missing, linked = [], [], modules(ver)
    for module in linked:
        for obj in c_objects(ver, module):
            target = expected / Path(obj).relative_to(f"build/{ver}")
            if not target.is_file():
                missing.append(obj)
                continue
            source = Path(obj).relative_to(f"build/{ver}").with_suffix("")  # src/<module>/<file>.c
            units.append({
                "name": source.relative_to("src").with_suffix("").as_posix(),  # <module>/<file>
                "base_path": rel(ROOT / obj),
                "target_path": rel(target),
                "metadata": {"source_path": rel(ROOT / source), "progress_categories": [module]},
            })
    config = {
        "min_version": "3.0.0",
        "custom_make": "ninja",
        "build_base": project == ROOT,
        "build_target": False,
        "watch_patterns": ["src/**/*.c", "src/**/*.h", "include/**/*.h", "asm/**/*.s"],
        # objdiff-cli's report ignores relocation targets unless told otherwise
        "options": {"functionRelocDiffs": "name_address", "mips.instrCategory": "r3000gte"},
        "progress_categories": [{"id": m, "name": m} for m in linked],
        "units": units,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(config, indent=2) + "\n")
    print(f"wrote {args.output}: {len(units)} units" + (f", {len(missing)} without an expected object "
          f"(run tools/objdiff_expected.py {ver})" if missing else ""))


if __name__ == "__main__":
    main()
