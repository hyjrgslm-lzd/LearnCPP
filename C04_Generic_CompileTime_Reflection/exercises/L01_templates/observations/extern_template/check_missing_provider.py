from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
repo = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(repo / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--generator", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--compiler", required=True)
    args = parser.parse_args()

    source = Path(__file__).resolve().parent
    case_source = args.build / "source"
    binary = args.build / "binary"
    case_source.mkdir(parents=True, exist_ok=True)
    cmake = repo / "C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake"
    (case_source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(l01_missing_provider LANGUAGES CXX)\n"
        f'include("{cmake.as_posix()}")\n'
        f'add_executable(subject "{(source / "missing_provider_main.cpp").as_posix()}")\n'
        "c04_configure_target(subject)\n"
        f'target_include_directories(subject PRIVATE "{source.as_posix()}")\n',
        encoding="utf-8",
    )

    configure = ["cmake", "-S", str(case_source), "-B", str(binary), "-G", args.generator,
                 "-DBUILD_TESTING=OFF", f"-DCMAKE_CXX_COMPILER={args.compiler}"]
    if args.platform:
        configure += ["-A", args.platform]
    records = {"configure": run_process(configure, 60)}
    accepted = False
    reason = "configuration must succeed"
    if records["configure"]["status"] == "PASS":
        subject = run_process(["cmake", "--build", str(binary), "--config", args.config,
                               "--target", "subject"], 180)
        records["subject"] = subject
        text = subject["stdout"] + "\n" + subject["stderr"]
        accepted = (
            subject["exit_code"] not in (None, 0)
            and not subject["timeout"]
            and not subject["error"]
            and not subject["cleanup_error"]
            and re.search(r"(LNK2019|undefined reference|unresolved external)", text) is not None
        )
        reason = "missing explicit-instantiation provider must fail at link"

    records.update(
        verdict="PASS" if accepted else "FAIL",
        reason=reason,
        input_sha256={
            str(source / "twice.hpp"): hashlib.sha256((source / "twice.hpp").read_bytes()).hexdigest(),
            str(source / "missing_provider_main.cpp"): hashlib.sha256((source / "missing_provider_main.cpp").read_bytes()).hexdigest(),
            str(Path(__file__).resolve()): hashlib.sha256(Path(__file__).resolve().read_bytes()).hexdigest(),
        },
    )
    serial = 1
    while (args.build / f"evidence-{serial}.json").exists():
        serial += 1
    evidence = args.build / f"evidence-{serial}.json"
    evidence.write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f'{records["verdict"]}: {reason}; {evidence}')
    if not accepted:
        for key in ("configure", "subject"):
            if key in records:
                print(records[key]["stdout"] + records[key]["stderr"])
    return 0 if accepted else 1


if __name__ == "__main__":
    raise SystemExit(main())
