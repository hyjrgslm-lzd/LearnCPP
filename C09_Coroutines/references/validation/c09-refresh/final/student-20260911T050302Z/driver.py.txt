"""Local C09 delivery matrix; reuse the repository's bounded process supervisor."""
import argparse
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


def snapshot():
    files = {}
    for folder, directories, names in os.walk(SOURCE):
        directories[:] = [name for name in directories if not name.lower().startswith("build")
                          and name not in (".git", "__pycache__")]
        if Path(folder) == SOURCE:
            directories[:] = [name for name in directories if name != "tools"]
        for name in names:
            p = Path(folder) / name
            if p.suffix in (".hpp", ".h", ".cpp", ".cmake", ".py", ".in", ".txt", ".json"):
                files[str(p.relative_to(COURSE))] = hashlib.sha256(p.read_bytes()).hexdigest()
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("lane", choices=("windows", "student", "linux"))
    args = parser.parse_args()
    os.chdir(ROOT)
    os.environ.setdefault("VSLANG", "1033")
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    output = COURSE / "references/validation/c09-refresh/final" / f"{args.lane}-{stamp}"
    output.mkdir(parents=True)
    # Preserve the orchestration version separately from compiler inputs.
    (output / "driver.py.txt").write_bytes(Path(__file__).read_bytes())
    before = snapshot()
    (output / "source.json").write_text(json.dumps(before, indent=2), encoding="utf-8")
    records = []

    def run(name, command, timeout=120, expected=0, rejection=False):
        result = supervise([str(x) for x in command], timeout)
        text = result["stdout"] + result["stderr"]
        clean = not (result["timeout"] or result["error"] or result["cleanup_error"])
        accepted = clean and result["exit_code"] == expected
        if rejection:
            accepted = accepted and any(word in text.lower() for word in ("todo", "check failed", "student check"))
        result.update(name=name, expected_exit=expected,
                      verdict="PASS" if accepted else "FAIL", expected_rejection=rejection)
        (output / f"{name}.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
        records.append({"name": name, "verdict": result["verdict"], "exit_code": result["exit_code"]})
        print(f"{name}: {result['verdict']} (exit {result['exit_code']})", flush=True)
        if not accepted:
            print(text[-5000:], flush=True)
            raise RuntimeError(f"{name} failed; see {output}")
        return result

    build = SOURCE / "build" / f"c09-final-{args.lane}"
    if args.lane == "student":
        query = build / ".cmake/api/v1/query/codemodel-v2"
        query.parent.mkdir(parents=True, exist_ok=True)
        query.touch()
    run("cmake-version", ["cmake", "--version"])
    if args.lane in ("windows", "student"):
        if os.name != "nt":
            raise RuntimeError("this lane needs the existing Windows VS 2026 installation")
        command = [sys.executable, SOURCE / "tools/configure_windows.py", "--build", build, "--light"]
        if args.lane == "student":
            command += ["--student"]
        run("configure", command, 180)
        configs = ["Release", "Debug"] if args.lane == "windows" else ["Release"]
        for config in configs:
            run(f"build-{config}", ["cmake", "--build", build, "--config", config, "--",
                                    "/m:1", "/nr:false", "/verbosity:minimal"], 900)
            if args.lane == "windows":
                run(f"ctest-{config}", ["ctest", "--test-dir", build, "-C", config,
                                        "--output-on-failure", "--timeout", "60",
                                        "--output-junit", build / f"{config}.xml"], 600)
    else:
        if os.name == "nt":
            raise RuntimeError("run the Linux lane with python3 inside the existing WSL distro")
        run("compiler", ["g++", "--version"])
        run("kernel", ["uname", "-a"])
        cache = SOURCE / "build/full-windows/_deps"
        asio_include = next((p for p in (cache / "asio-src/asio/include", cache / "asio-src/include")
                             if (p / "asio.hpp").is_file()), None)
        if asio_include is None:
            raise RuntimeError("pinned local Asio headers are missing")
        run("configure", ["cmake", "-S", SOURCE, "-B", build, "-G", "Ninja",
                          "-DCMAKE_BUILD_TYPE=Release", "-DCOROUTINE_STUDY_BUILD_REFERENCE=ON",
                          "-DCOROUTINE_STUDY_FETCH_DEPS=ON", "-DFETCHCONTENT_FULLY_DISCONNECTED=ON",
                          "-DCOROUTINE_STUDY_ENABLE_ASIO=ON", f"-DASIO_INCLUDE_DIR={asio_include}",
                          "-DCOROUTINE_STUDY_ENABLE_CPPCORO=ON",
                          f"-DFETCHCONTENT_SOURCE_DIR_CPPCORO={cache / 'cppcoro-src'}",
                          "-DCOROUTINE_STUDY_ENABLE_IO_URING=ON",
                          "-DCOROUTINE_STUDY_LIBURING_ROOT=/root/learncpp-c07/deps/install-liburing-2.15"], 180)
        listing = json.loads(run("test-list", ["ctest", "--test-dir", build, "--show-only=json-v1"])["stdout"])
        probe = build / "generator-capability.cpp"
        probe.write_text('#include <generator>\nstd::generator<int> one() { co_yield 1; }\n'
                         'int main() { for (int n : one()) if (n != 1) return 1; }\n', encoding="utf-8")
        capability = supervise(["g++", "-std=c++23", "-fsyntax-only", str(probe)], 30)
        if capability["timeout"] or capability["error"] or capability["cleanup_error"]:
            raise RuntimeError("generator capability could not be checked")
        supported = capability["exit_code"] == 0
        capability["verdict"] = "PASS" if supported else "SKIP"
        (output / "generator-capability.json").write_text(json.dumps(capability, indent=2), encoding="utf-8")
        skipped = set() if supported else {"P2_generator_basics_reference", "A1_first_generator_reference",
                                            "B1_recursive_generator_reference", "Capstone1_async_crawler_reference"}
        names = [test["name"] for test in listing["tests"] if test["name"] not in skipped]
        targets = [("F3_halo_diagnose" if name == "F3_halo_contract" else
                    name.replace("bad_newline_rejected", "bad_newline_check")) for name in names]
        run("build-Release", ["cmake", "--build", build, "--parallel", "2", "--target", *targets], 900)
        run("ctest-Release", ["ctest", "--test-dir", build, "--output-on-failure", "--timeout", "60",
                              "-E", "^(" + "|".join(sorted(skipped)) + ")$",
                              "--output-junit", build / "Release.xml"], 600)
        (output / "excluded.json").write_text(json.dumps({"reason": "GCC 13 libstdc++ lacks <generator>",
                                                          "targets": sorted(skipped)}, indent=2), encoding="utf-8")

    if args.lane == "student":
        listing = json.loads(run("test-list", ["ctest", "--test-dir", build, "-C", "Release",
                                               "--show-only=json-v1"])["stdout"])
        provided = {"B3_callback_to_awaiter", "C1_stop_token_cancel", "C3_async_scope",
                    "J1_eight_pitfalls", "mini_task_test", "mini_generator_test", "mini_stop_test"}
        for test in listing["tests"]:
            labels = next((p["value"] for p in test["properties"] if p["name"] == "LABELS"), [])
            if any(label in labels for label in ("reference", "answer", "good", "reference_adapter")):
                raise RuntimeError(f"answer registered in Student-only lane: {test['name']}")
            if "starter" in labels:
                expected = 0 if test["name"] in provided else 1
                run(test["name"], test["command"], 60, expected, rejection=bool(expected))
        # Actual compiler dependency logs, not only target names or include-directory declarations.
        violations = []
        observed_sources = set()
        def inspect_path(path):
            value = str(path).strip().lstrip("^").replace("\\", "/").lower()
            if "c09_coroutines/" in value and ("/reference/" in value or value.endswith("/solution.cpp")):
                violations.append(value)
            if value.endswith((".cpp", ".cxx")):
                observed_sources.add(value)
        for log in build.rglob("CL.read.1.tlog"):
            content = log.read_text(encoding="utf-16", errors="replace").replace("\\", "/").lower()
            for line in content.splitlines():
                for path in line.split("|"):
                    inspect_path(path)
        source_reports = 0
        for report in build.rglob("*.json"):
            try:
                data = json.loads(report.read_text(encoding="utf-8-sig")).get("Data", {})
            except (UnicodeError, json.JSONDecodeError, AttributeError):
                continue
            if isinstance(data, dict) and data.get("Source") and isinstance(data.get("Includes"), list):
                source_reports += 1
                inspect_path(data["Source"])
                for path in data["Includes"]:
                    inspect_path(path)
        expected_sources = set()
        reply = build / ".cmake/api/v1/reply"
        index = json.loads(sorted(reply.glob("index-*.json"))[-1].read_text())
        model_ref = next(item for item in index["objects"] if item["kind"] == "codemodel")
        model = json.loads((reply / model_ref["jsonFile"]).read_text())
        for config in model["configurations"]:
            if config["name"] != "Release":
                continue
            for entry in config["targets"]:
                target = json.loads((reply / entry["jsonFile"]).read_text())
                for item in target.get("sources", []):
                    path = (Path(model["paths"]["source"]) / item["path"]).resolve().as_posix().lower()
                    if path.endswith((".cpp", ".cxx")) and "c09_coroutines/exercises/" in path and "/build/" not in path:
                        expected_sources.add(path)
        missing_sources = expected_sources - observed_sources
        if violations:
            raise RuntimeError(f"Student includes answer inputs: {violations[:5]}")
        if not expected_sources or missing_sources:
            raise RuntimeError(f"compiler dependency coverage is incomplete: {sorted(missing_sources)}")
        (output / "isolation.json").write_text(json.dumps({"violations": violations,
            "dependency_logs": len(list(build.rglob('CL.read.1.tlog'))), "source_reports": source_reports,
            "course_sources": len(expected_sources), "missing_sources": sorted(missing_sources)}, indent=2), encoding="utf-8")
    if before != snapshot():
        raise RuntimeError("course code changed during this matrix; preserve records and rerun affected lane")
    (output / "summary.json").write_text(json.dumps({"lane": args.lane, "verdict": "PASS",
                                                     "records": records}, indent=2), encoding="utf-8")
    print(f"matrix complete: {output}", flush=True)


if __name__ == "__main__":
    main()
