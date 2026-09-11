"""Run bounded Linux sanitizer capabilities and coroutine lifetime regressions."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
COURSE = ROOT / "C09_Coroutines"
SOURCE = COURSE / "exercises"
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "C07_OS_Memory_System_IO/exercises/tools"))
from run_test import supervise


def main():
    if os.name == "nt":
        raise SystemExit("Run with python3 in the existing WSL distro")
    os.chdir(ROOT)
    build = SOURCE / "build/c09-final-diagnostics"
    build.mkdir(parents=True, exist_ok=True)
    output = COURSE / "references/validation/c09-refresh/final" / datetime.now(timezone.utc).strftime("diagnostics-%Y%m%dT%H%M%SZ")
    output.mkdir(parents=True)
    records = []

    def run(name, command, timeout=120, capability=False):
        result = supervise([str(v) for v in command], timeout)
        clean = not (result["timeout"] or result["error"] or result["cleanup_error"])
        passed = clean and result["exit_code"] == 0
        diagnostic = (result["stdout"] + result["stderr"]).lower()
        passed = passed and not any(marker in diagnostic for marker in
                                   ("error: addresssanitizer", "runtime error:", "warning: threadsanitizer"))
        result["verdict"] = "PASS" if passed else "SKIP" if capability and clean else "FAIL"
        (output / f"{name}.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
        records.append({"name": name, "verdict": result["verdict"]})
        print(name, result["verdict"], flush=True)
        if not passed and not capability:
            print((result["stdout"] + result["stderr"])[-5000:])
            raise RuntimeError(name)
        return passed

    run("compiler", ["g++", "--version"])
    probe = build / "sanitizer-capability.cpp"
    probe.write_text('#include <atomic>\n#include <thread>\nint main(){std::atomic<int> n{0};'
                     '{std::jthread t([&]{++n;});}return n==1?0:1;}\n', encoding="utf-8")
    (output / "sanitizer-capability.cpp.txt").write_bytes(probe.read_bytes())
    common = ["g++", "-std=c++23", "-g", "-O1", "-fno-omit-frame-pointer", "-pthread"]
    groups = [("address", "-fsanitize=address,undefined"), ("thread", "-fsanitize=thread")]
    sources = [SOURCE / "runtime_tests" / name for name in
               ("lazy_task_reference_test.cpp", "await_resume_exception_test.cpp", "sync_wait_reference_test.cpp",
                "task_scope_reference_test.cpp", "when_any_cancel_join_reference_test.cpp", "sync_completion_reference_test.cpp")]
    reference = SOURCE / "Capstone5_mini_corolib/reference"
    sources += [reference / "tests" / name for name in
                ("task_test.cpp", "sync_wait_test.cpp", "when_all_test.cpp", "when_any_test.cpp",
                 "task_scope_test.cpp", "run_loop_test.cpp", "shared_task_test.cpp", "shared_task_abandon_test.cpp")]
    includes = ["-I", SOURCE / "include", "-I", reference / "include"]
    for mode, flag in groups:
        executable = build / f"{mode}-capability"
        if not run(f"{mode}-capability-build", [*common, flag, probe, "-o", executable], capability=True):
            continue
        environment = ["env", "ASAN_OPTIONS=detect_leaks=1:halt_on_error=1",
                       "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"] if mode == "address" else []
        if not run(f"{mode}-capability-run", [*environment, executable], 30, capability=True):
            continue
        for index, source in enumerate(sources):
            name = f"{mode}-{index:02d}-{source.stem}"
            executable = build / name
            run(name + "-build", [*common, flag, *includes, source, "-o", executable])
            run(name, [*environment, executable], 30)
    (output / "summary.json").write_text(json.dumps({"records": records,
        "sources": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}, indent=2), encoding="utf-8")
    print(f"diagnostics complete: {output}", flush=True)


if __name__ == "__main__":
    main()
