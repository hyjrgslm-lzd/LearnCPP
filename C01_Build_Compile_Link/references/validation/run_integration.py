"""Rebuild the reviewed C01 matrix in new directories and retain each result.

Run in an x64 MSVC developer shell on Windows. Nothing is installed globally.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import sys

COURSE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(COURSE / "exercises/tools"))
from process_runner import run_process

PRESETS = ("verify-core", "verify-debug", "modules-msvc-ninja", "import-std-msvc-ninja")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--presets", nargs="+", choices=PRESETS, default=list(PRESETS))
    args = parser.parse_args()
    output = args.output.resolve()
    builds = COURSE / "exercises/build" / output.name
    if output.exists() or builds.exists():
        parser.error("output and its matching build directory must both be new")
    cmake, ctest = shutil.which("cmake"), shutil.which("ctest")
    if not cmake or not ctest:
        parser.error("cmake and ctest must be available")
    ninja = Path(os.environ.get("VSINSTALLDIR", "")) / "Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
    if any("ninja" in preset for preset in args.presets) and not ninja.is_file():
        parser.error("module presets require the installed VS Ninja in an MSVC developer shell")

    output.mkdir(parents=True)
    os.chdir(COURSE / "exercises")
    report = {
        "status": "RUNNING", "presets": args.presets, "steps": [],
        "platform": platform.platform(), "python": sys.version,
        "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "process_runner_sha256": hashlib.sha256((COURSE / "exercises/tools/process_runner.py").read_bytes()).hexdigest(),
        "build_root": str(builds),
    }

    def save() -> None:
        (output / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")

    save()
    for preset in args.presets:
        build = builds / preset
        config = "Debug" if preset == "verify-debug" else "Release"
        configure = [cmake, "--preset", preset, "-B", str(build)]
        if "ninja" in preset:
            configure.append(f"-DCMAKE_MAKE_PROGRAM={ninja}")
        elif os.name == "nt":
            configure += ["-G", "Visual Studio 18 2026", "-A", "x64"]
        commands = [
            ("configure", configure, 180),
            ("build", [cmake, "--build", str(build), "--config", config, "--parallel", "4", "--verbose"], 600),
            ("ctest", [ctest, "--test-dir", str(build), "-C", config, "--output-on-failure", "--output-junit", str(output / f"{preset}.xml")], 600),
        ]
        for stage, command, timeout in commands:
            name = f"{preset}-{stage}"
            print(f"START {name}", flush=True)
            result = run_process(command, timeout)
            (output / f"{name}.stdout.txt").write_text(result["stdout"], encoding="utf-8")
            (output / f"{name}.stderr.txt").write_text(result["stderr"], encoding="utf-8")
            metadata = {key: value for key, value in result.items() if key not in {"stdout", "stderr"}}
            (output / f"{name}.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8")
            report["steps"].append({"name": name, **metadata})
            if result["status"] != "PASS":
                report["status"] = "FAIL"
                save()
                print(result["stdout"][-2000:], result["stderr"][-2000:], flush=True)
                return 1
            print(f"PASS {name} ({result['process_seconds']:.2f}s)", flush=True)
            save()
    report["status"] = "PASS"
    save()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
