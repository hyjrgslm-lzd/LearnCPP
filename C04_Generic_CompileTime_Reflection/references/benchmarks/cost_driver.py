"""Run C04 B01 compile-cost experiments with bounded child processes."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import platform
from pathlib import Path
import random
import statistics
import sys
import textwrap
import time
from typing import Any

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process
from meta_lookup_cost import (
    COUNTS as META_COUNTS,
    EXPECTED_MP11_COMMIT,
    KINDS as META_KINDS,
    TARGETS as META_TARGETS,
    generate_meta_lookup,
    mp11_include_dir,
    mp11_marker_path,
    mp11_source_root,
    read_dependency_marker,
    variant_name as meta_variant_name,
)


COURSE = Path(__file__).resolve().parents[2]
REPO = COURSE.parent
CLANG = Path("D:/VisualStudio2026/Installed/VC/Tools/Llvm/x64/bin/clang-cl.exe")
LLVM_READOBJ = CLANG.with_name("llvm-readobj.exe")
LLVM_NM = CLANG.with_name("llvm-nm.exe")
LLVM_SIZE = CLANG.with_name("llvm-size.exe")
BUILD_NAMES = {"cl.exe", "clang-cl.exe", "MSBuild.exe", "cmake.exe", "ninja.exe", "link.exe", "lld-link.exe"}
SEED = 40412025
EVENT_LOG: Path | None = None
MSBUILD_CPU_ACTIVE_THRESHOLD = 0.05


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def command_record(command: list[str], timeout: float = 180) -> dict[str, Any]:
    result = run_process(command, timeout)
    result["cwd"] = str(Path.cwd())
    emit_event("command", result)
    return result


def command_record_perf(command: list[str], timeout: float = 180) -> dict[str, Any]:
    started = time.perf_counter()
    result = run_process(command, timeout)
    result["perf_counter_seconds"] = time.perf_counter() - started
    result["cwd"] = str(Path.cwd())
    emit_event("command", result)
    return result


def emit_event(kind: str, payload: dict[str, Any]) -> None:
    if EVENT_LOG is None:
        return
    EVENT_LOG.parent.mkdir(parents=True, exist_ok=True)
    entry = {"time": datetime.now(timezone.utc).isoformat(), "kind": kind, "payload": payload}
    with EVENT_LOG.open("a", encoding="utf-8") as output:
        output.write(json.dumps(entry, ensure_ascii=False) + "\n")


def build_process_snapshot() -> dict[str, Any]:
    script = (
        "$names=@('cl','clang-cl','MSBuild','cmake','ninja','link','lld-link'); "
        "Get-Process -ErrorAction SilentlyContinue | "
        "Where-Object { $names -contains $_.ProcessName } | "
        "Select-Object ProcessName,Id,CPU | ConvertTo-Json -Compress"
    )
    result = command_record(["powershell", "-NoProfile", "-Command", script], 20)
    if result["status"] != "PASS":
        return {"status": "FAIL", "record": result, "processes": []}
    if not result["stdout"].strip():
        return {"status": "PASS", "record": result, "processes": []}
    try:
        data = json.loads(result["stdout"])
    except json.JSONDecodeError as error:
        return {"status": "FAIL", "record": result, "processes": [], "error": str(error)}
    if isinstance(data, dict):
        data = [data]
    processes = []
    for item in data:
        cpu = item.get("CPU")
        processes.append({"name": item["ProcessName"] + ".exe", "pid": int(item["Id"]),
                          "cpu_seconds": None if cpu is None else float(cpu)})
    return {"status": "PASS", "record": result, "processes": processes}


def classify_process_activity(before: dict[str, Any], after: dict[str, Any]) -> dict[str, Any]:
    if before["status"] != "PASS" or after["status"] != "PASS":
        return {"idle": False, "detection_failed": True, "before": before, "after": after,
                "active": [], "idle_msbuild_nodes": []}
    before_by_pid = {p["pid"]: p for p in before["processes"]}
    after_pids = {p["pid"] for p in after["processes"]}
    active = []
    idle_msbuild_nodes = []
    for process in after["processes"]:
        if process["name"] != "MSBuild.exe":
            active.append({"reason": "compiler-or-build-process-present", **process})
            continue
        previous = before_by_pid.get(process["pid"])
        if previous is None:
            active.append({"reason": "new-msbuild-node", **process})
            continue
        if process["cpu_seconds"] is None or previous["cpu_seconds"] is None:
            active.append({"reason": "msbuild-cpu-unavailable", **process})
            continue
        delta = process["cpu_seconds"] - previous["cpu_seconds"]
        if delta > MSBUILD_CPU_ACTIVE_THRESHOLD:
            active.append({"reason": "msbuild-cpu-delta", "cpu_delta_seconds": delta, **process})
        else:
            idle_msbuild_nodes.append({"cpu_delta_seconds": delta, **process})
    for pid, process in before_by_pid.items():
        if pid in after_pids:
            continue
        if process["name"] == "MSBuild.exe":
            active.append({"reason": "msbuild-node-exited-during-probe", **process})
        else:
            active.append({"reason": "compiler-or-build-process-exited-during-probe", **process})
    return {"idle": not active, "detection_failed": False, "before": before, "after": after,
            "active": active, "idle_msbuild_nodes": idle_msbuild_nodes}


def active_build_probe() -> dict[str, Any]:
    before = build_process_snapshot()
    time.sleep(1)
    after = build_process_snapshot()
    return classify_process_activity(before, after)


def wait_for_idle() -> dict[str, Any]:
    observations = []
    for _ in range(12):
        probe = active_build_probe()
        observations.append({"time": datetime.now(timezone.utc).isoformat(), **probe})
        emit_event("idle_probe", observations[-1])
        if probe["idle"]:
            return {"idle": True, "observations": observations}
        time.sleep(5)
    return {"idle": False, "observations": observations}


def last_idle_snapshot(wait_record: dict[str, Any]) -> dict[str, Any] | None:
    if not wait_record.get("idle"):
        return None
    observations = wait_record.get("observations", [])
    if not observations:
        return None
    return observations[-1].get("after")


def sample_overlap(pre_wait: dict[str, Any], post_probe: dict[str, Any]) -> dict[str, Any]:
    before = last_idle_snapshot(pre_wait)
    if before is None:
        return {"idle": False, "reason": "pre-sample-idle-missing", "active": [], "detection_failed": True}
    if post_probe.get("detection_failed"):
        return {"idle": False, "reason": "post-probe-detection-failed", "active": [], "detection_failed": True}
    after = post_probe["before"]
    compared = classify_process_activity(before, after)
    compared["reason"] = "pre-sample-to-post-sample-first-snapshot"
    return compared


def detector_self_check() -> dict[str, Any]:
    def snap(processes: list[dict[str, Any]], status: str = "PASS") -> dict[str, Any]:
        return {"status": status, "record": {"status": status}, "processes": processes}

    cases = [
        ("idle_msbuild_node",
         snap([{"name": "MSBuild.exe", "pid": 1, "cpu_seconds": 10.0}]),
         snap([{"name": "MSBuild.exe", "pid": 1, "cpu_seconds": 10.0}]),
         True),
        ("busy_msbuild_node",
         snap([{"name": "MSBuild.exe", "pid": 1, "cpu_seconds": 10.0}]),
         snap([{"name": "MSBuild.exe", "pid": 1, "cpu_seconds": 10.2}]),
         False),
        ("new_msbuild_node",
         snap([]),
         snap([{"name": "MSBuild.exe", "pid": 2, "cpu_seconds": 0.1}]),
         False),
        ("exited_msbuild_node",
         snap([{"name": "MSBuild.exe", "pid": 1, "cpu_seconds": 10.0}]),
         snap([]),
         False),
        ("compiler_present",
         snap([]),
         snap([{"name": "cl.exe", "pid": 3, "cpu_seconds": 1.0}]),
         False),
        ("compiler_exited",
         snap([{"name": "cl.exe", "pid": 3, "cpu_seconds": 1.0}]),
         snap([]),
         False),
        ("detection_failure",
         snap([], "FAIL"),
         snap([]),
         False),
    ]
    results = []
    for name, before, after, expected_idle in cases:
        actual = classify_process_activity(before, after)
        results.append({"name": name, "expected_idle": expected_idle, "actual_idle": actual["idle"],
                        "status": "PASS" if actual["idle"] == expected_idle else "FAIL",
                        "active_reasons": [entry["reason"] for entry in actual["active"]]})
    samples = [{"group": "type_query", "count": 32, "kind": "recursive", "phase": "warmup", "valid": False}]
    samples.extend({"group": "type_query", "count": 32, "kind": "recursive", "phase": "sample", "valid": True}
                   for _ in range(4))
    count_check = validate_sample_counts({"type_query": {"samples": samples}, "multi_tu": {"samples": []}}, require_all=False)
    count_ok = any(item["status"] == "FAIL" and item["group"] == "type_query" for item in count_check)
    results.append({"name": "four_samples_rejected", "expected_idle": False, "actual_idle": not count_ok,
                    "status": "PASS" if count_ok else "FAIL", "active_reasons": []})
    return {"status": "PASS" if all(item["status"] == "PASS" for item in results) else "FAIL", "cases": results}


def generated_type_query(kind: str, count: int) -> str:
    tags = ", ".join(f"tag<{i}>" for i in range(count))
    if kind == "recursive":
        body = """
template<class T, class... Ts>
struct contains : std::false_type {};

template<class T, class Head, class... Tail>
struct contains<T, Head, Tail...>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type,
                         contains<T, Tail...>> {};

constexpr bool answer = contains<tag<Target>, Types...>::value;
constexpr bool missing = contains<tag<Missing>, Types...>::value;
constexpr bool empty = contains<tag<Target>>::value;
"""
    else:
        body = """
template<class T, class... Ts>
constexpr bool contains = (std::is_same_v<T, Ts> || ...);

constexpr bool answer = contains<tag<Target>, Types...>;
constexpr bool missing = contains<tag<Missing>, Types...>;
constexpr bool empty = contains<tag<Target>>;
"""
    return textwrap.dedent(f"""
        #include <type_traits>
        template<int I> struct tag {{}};
        {body.replace('tag<Target>', f'tag<{count - 1}>').replace('tag<Missing>', f'tag<{count}>').replace('Types...', tags)}
        static_assert(answer);
        static_assert(!missing);
        static_assert(!empty);
        extern "C" __declspec(dllexport) int result() {{ return (answer ? 100 : 0) + (missing ? 10 : 0) + (empty ? 1 : 0); }}
        int main() {{ return result() == 100 ? 0 : 1; }}
    """).strip() + "\n"


def generated_multi_files(kind: str) -> dict[str, str]:
    header_extra = "extern template std::uint32_t transform<64>(std::uint32_t);\n" if kind == "explicit" else ""
    header = textwrap.dedent(f"""
        #pragma once
        #include <cstdint>
        template<int N>
        __declspec(noinline) std::uint32_t transform(std::uint32_t value) {{
            std::uint32_t result = value + static_cast<std::uint32_t>(N);
            for (int i = 0; i != 96; ++i) {{
                result = (result * 31u) ^ (static_cast<std::uint32_t>(N) + static_cast<std::uint32_t>(i) * 17u);
            }}
            return result;
        }}
        {header_extra}
    """).strip() + "\n"
    files = {"kernel.hpp": header}
    for i in range(4):
        files[f"tu{i}.cpp"] = textwrap.dedent(f"""
            #include "kernel.hpp"
            extern "C" __declspec(dllexport) std::uint32_t use_{i}(std::uint32_t value) {{
                return transform<64>(value + {i});
            }}
        """).strip() + "\n"
    files["main.cpp"] = textwrap.dedent("""
        #include <cstdint>
        #include <cstdio>
        extern "C" std::uint32_t use_0(std::uint32_t);
        extern "C" std::uint32_t use_1(std::uint32_t);
        extern "C" std::uint32_t use_2(std::uint32_t);
        extern "C" std::uint32_t use_3(std::uint32_t);

        std::uint32_t reference_transform(std::uint32_t value) {
            std::uint32_t result = value + 64u;
            for (int i = 0; i != 96; ++i) {
                result = (result * 31u) ^ (64u + static_cast<std::uint32_t>(i) * 17u);
            }
            return result;
        }

        int main() {
            const std::uint32_t inputs[4] = {1u, 2u, 3u, 4u};
            const std::uint32_t actual[4] = {use_0(inputs[0]), use_1(inputs[1]), use_2(inputs[2]), use_3(inputs[3])};
            const std::uint32_t expected[4] = {
                reference_transform(inputs[0] + 0u),
                reference_transform(inputs[1] + 1u),
                reference_transform(inputs[2] + 2u),
                reference_transform(inputs[3] + 3u)
            };
            std::printf("%u %u %u %u\\n", actual[0], actual[1], actual[2], actual[3]);
            for (int i = 0; i != 4; ++i) {
                if (actual[i] != expected[i]) {
                    return i + 1;
                }
            }
            return 0;
        }
    """).strip() + "\n"
    if kind == "explicit":
        files["instantiation.cpp"] = '#include "kernel.hpp"\ntemplate std::uint32_t transform<64>(std::uint32_t);\n'
    return files


def compile_obj(source: Path, obj: Path, trace: Path | None = None,
                include_dirs: list[Path] | None = None, perf_counter: bool = False) -> dict[str, Any]:
    obj.parent.mkdir(parents=True, exist_ok=True)
    if trace is not None:
        trace.parent.mkdir(parents=True, exist_ok=True)
    command = [str(CLANG), "/nologo", "/c", "/std:c++latest", "/O2", "/EHsc", "/GR-",
               f"/Fo{obj}", str(source)]
    for include_dir in include_dirs or []:
        command.insert(-1, f"/I{include_dir}")
    if trace is not None:
        command.insert(4, f"/clang:-ftime-trace={trace}")
        command.insert(5, "/clang:-ftime-trace-granularity=0")
    record = command_record_perf if perf_counter else command_record
    return record(command, 180)


def link_exe(objects: list[Path], exe: Path) -> dict[str, Any]:
    exe.parent.mkdir(parents=True, exist_ok=True)
    return command_record([str(CLANG), "/nologo", *map(str, objects), f"/Fe{exe}", "/link", "/INCREMENTAL:NO"], 180)


def run_exe(exe: Path) -> dict[str, Any]:
    return command_record([str(exe)], 30)


def require_pass(label: str, result: dict[str, Any]) -> None:
    if result.get("status") != "PASS":
        emit_event("fatal", {"label": label, "result": result})
        raise RuntimeError(f"{label} failed")


def read_trace(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    totals = {}
    for event in data.get("traceEvents", []):
        name = event.get("name", "")
        if name.startswith("Total "):
            totals[name] = {"duration_us": event.get("dur", 0), "count": event.get("args", {}).get("count")}
    return totals


def section_text_size(path: Path) -> dict[str, Any]:
    result = command_record([str(LLVM_READOBJ), "--sections", str(path)], 60)
    sections = []
    current: dict[str, Any] | None = None
    for line in result["stdout"].splitlines():
        stripped = line.strip()
        if stripped == "Section {":
            current = {}
        elif stripped == "}" and current is not None:
            name = current.get("name", "")
            if name == ".text" or name.startswith(".text$"):
                sections.append(current)
            current = None
        elif current is not None and stripped.startswith("Name:"):
            current["name"] = stripped.split(":", 1)[1].strip().split(" ", 1)[0]
        elif current is not None and stripped.startswith("RawDataSize:"):
            current["raw_data_size"] = int(stripped.split(":", 1)[1].strip(), 0)
        elif current is not None and stripped.startswith("VirtualSize:"):
            current["virtual_size"] = int(stripped.split(":", 1)[1].strip(), 0)
    parsed = result["status"] == "PASS" and bool(sections)
    record = {"tool": "llvm-readobj --sections", "status": "PASS" if parsed else "FAIL",
              "text_section_count": len(sections),
              "text_raw_total": sum(int(s.get("raw_data_size", 0)) for s in sections),
              "text_virtual_total": sum(int(s.get("virtual_size", 0)) for s in sections),
              "sections": sections, "record": result}
    emit_event("section_parse", {"path": str(path), **{k: v for k, v in record.items() if k != "record"}})
    return record


def symbols(path: Path) -> dict[str, Any]:
    nm = command_record([str(LLVM_NM), "--print-size", "--demangle", str(path)], 60)
    size = command_record([str(LLVM_SIZE), str(path)], 60)
    interesting = [line for line in nm["stdout"].splitlines() if "transform<64>" in line or "result" in line or "use_" in line]
    return {"nm_interesting": interesting, "nm_status": nm["status"], "size_stdout": size["stdout"], "size_status": size["status"]}


def object_record(path: Path) -> dict[str, Any]:
    section = section_text_size(path)
    section["artifact_sha256"] = sha256_file(path)
    return section


def record_type_query(root: Path, include_timing: bool) -> dict[str, Any]:
    source_root = root / "generated" / "type_query"
    records = {"sources": {}, "trace": {}, "correctness": {}, "samples": []}
    for count in (32, 128, 256):
        for kind in ("recursive", "fold"):
            source = source_root / f"{kind}_{count}.cpp"
            write_text(source, generated_type_query(kind, count))
            records["sources"][f"{kind}_{count}"] = {"path": str(source), "sha256": sha256_file(source)}
            emit_event("source", {"group": "type_query", "kind": kind, "count": count,
                                  **records["sources"][f"{kind}_{count}"]})
            obj = root / "baseline" / "type_query" / f"{kind}_{count}.obj"
            exe = root / "baseline" / "type_query" / f"{kind}_{count}.exe"
            compile_result = compile_obj(source, obj)
            require_pass(f"type_query {kind} {count} baseline compile", compile_result)
            link_result = link_exe([obj], exe)
            require_pass(f"type_query {kind} {count} baseline link", link_result)
            run_result = run_exe(exe)
            require_pass(f"type_query {kind} {count} baseline run", run_result)
            trace = root / "trace" / "type_query" / f"{kind}_{count}.json"
            trace_result = compile_obj(source, root / "trace" / "type_query" / f"{kind}_{count}.obj", trace)
            require_pass(f"type_query {kind} {count} trace compile", trace_result)
            object_text = object_record(obj)
            exe_text = object_record(exe)
            require_pass(f"type_query {kind} {count} object section parse", object_text)
            require_pass(f"type_query {kind} {count} exe section parse", exe_text)
            records["correctness"][f"{kind}_{count}"] = {
                "compile": compile_result, "link": link_result, "run": run_result,
                "object_text": object_text,
                "exe_text": exe_text,
                "symbols": symbols(obj),
            }
            records["trace"][f"{kind}_{count}"] = {
                "compile": trace_result,
                "totals": read_trace(trace) if trace.exists() else {},
            }
            emit_event("phase_complete", {"phase": "type_query_correctness_trace", "kind": kind, "count": count})
    if not include_timing:
        return records
    rng = random.Random(SEED)
    for count in (32, 128, 256):
        order = ["recursive", "fold"] * 6
        rng.shuffle(order)
        seen = {"recursive": 0, "fold": 0}
        for kind in order:
            seen[kind] += 1
            phase = "warmup" if seen[kind] == 1 else "sample"
            if seen[kind] > 6:
                continue
            idle_before = wait_for_idle()
            source = source_root / f"{kind}_{count}.cpp"
            obj = root / "timing" / "type_query" / f"{kind}_{count}_{phase}_{seen[kind]}.obj"
            result = compile_obj(source, obj)
            post_probe = active_build_probe()
            overlap = sample_overlap(idle_before, post_probe)
            valid = phase == "sample" and result["status"] == "PASS" and idle_before["idle"] and post_probe["idle"] and overlap["idle"]
            sample = {"group": "type_query", "count": count, "kind": kind,
                      "phase": phase, "index": seen[kind], "valid": valid,
                      "idle_before": idle_before, "compile": result, "post_probe": post_probe,
                      "pre_to_post_overlap": overlap,
                      "artifact_sha256": sha256_file(obj) if result["status"] == "PASS" and obj.exists() else None}
            records["samples"].append(sample)
            emit_event("sample", sample)
    return records


def record_multi_tu(root: Path, include_timing: bool) -> dict[str, Any]:
    source_root = root / "generated" / "multi_tu"
    records = {"sources": {}, "trace": {}, "correctness": {}, "samples": []}
    for kind in ("implicit", "explicit"):
        files = generated_multi_files(kind)
        kind_root = source_root / kind
        for name, text in files.items():
            write_text(kind_root / name, text)
        records["sources"][kind] = {name: {"path": str(kind_root / name), "sha256": sha256_file(kind_root / name)}
                                    for name in sorted(files)}
        emit_event("source_group", {"group": "multi_tu", "kind": kind, "files": records["sources"][kind]})
        compile_units = [kind_root / f"tu{i}.cpp" for i in range(4)]
        if kind == "explicit":
            compile_units.append(kind_root / "instantiation.cpp")
        main = kind_root / "main.cpp"
        objects = []
        compiles = []
        for source in [*compile_units, main]:
            obj = root / "baseline" / "multi_tu" / kind / (source.stem + ".obj")
            result = compile_obj(source, obj)
            require_pass(f"multi_tu {kind} {source.name} baseline compile", result)
            compiles.append({"source": source.name, "result": result})
            objects.append(obj)
        exe = root / "baseline" / "multi_tu" / f"{kind}.exe"
        link_result = link_exe(objects, exe)
        require_pass(f"multi_tu {kind} baseline link", link_result)
        run_result = run_exe(exe)
        require_pass(f"multi_tu {kind} baseline run", run_result)
        object_text = {obj.name: object_record(obj) for obj in objects}
        for name, parsed in object_text.items():
            require_pass(f"multi_tu {kind} {name} section parse", parsed)
        exe_text = object_record(exe)
        require_pass(f"multi_tu {kind} exe section parse", exe_text)
        records["correctness"][kind] = {"compiles": compiles, "link": link_result, "run": run_result,
                                        "object_text": object_text,
                                        "exe_text": exe_text,
                                        "symbols": {obj.name: symbols(obj) for obj in objects},
                                        "exe_sha256": sha256_file(exe) if exe.exists() else None}
        for source in compile_units:
            trace = root / "trace" / "multi_tu" / kind / f"{source.stem}.json"
            trace_obj = root / "trace" / "multi_tu" / kind / f"{source.stem}.obj"
            trace_result = compile_obj(source, trace_obj, trace)
            require_pass(f"multi_tu {kind} {source.name} trace compile", trace_result)
            records["trace"][f"{kind}_{source.stem}"] = {"compile": trace_result,
                                                          "totals": read_trace(trace) if trace.exists() else {}}
        emit_event("phase_complete", {"phase": "multi_tu_correctness_trace", "kind": kind})
    if records["correctness"]["implicit"]["run"]["stdout"] != records["correctness"]["explicit"]["run"]["stdout"]:
        emit_event("fatal", {"label": "multi_tu variant stdout equality",
                             "implicit": records["correctness"]["implicit"]["run"]["stdout"],
                             "explicit": records["correctness"]["explicit"]["run"]["stdout"]})
        raise RuntimeError("multi_tu variant stdout equality failed")
    if not include_timing:
        return records
    rng = random.Random(SEED + 1)
    order = ["implicit", "explicit"] * 6
    rng.shuffle(order)
    seen = {"implicit": 0, "explicit": 0}
    for kind in order:
        seen[kind] += 1
        phase = "warmup" if seen[kind] == 1 else "sample"
        if seen[kind] > 6:
            continue
        kind_root = source_root / kind
        compile_units = [kind_root / f"tu{i}.cpp" for i in range(4)]
        if kind == "explicit":
            compile_units.append(kind_root / "instantiation.cpp")
        idle_before = wait_for_idle()
        compiles = []
        for source in compile_units:
            obj = root / "timing" / "multi_tu" / f"{kind}_{phase}_{seen[kind]}_{source.stem}.obj"
            result = compile_obj(source, obj)
            compiles.append({"source": source.name, "result": result,
                             "artifact_sha256": sha256_file(obj) if result["status"] == "PASS" and obj.exists() else None})
        post_probe = active_build_probe()
        overlap = sample_overlap(idle_before, post_probe)
        total = sum(c["result"]["process_seconds"] for c in compiles)
        valid = phase == "sample" and all(c["result"]["status"] == "PASS" for c in compiles) and idle_before["idle"] and post_probe["idle"] and overlap["idle"]
        sample = {"group": "multi_tu_compile", "kind": kind, "phase": phase,
                  "index": seen[kind], "valid": valid, "idle_before": idle_before,
                  "compiles": compiles, "total_compile_process_seconds": total,
                  "post_probe": post_probe, "pre_to_post_overlap": overlap}
        records["samples"].append(sample)
        emit_event("sample", sample)
    return records


def verify_mp11_dependency() -> dict[str, Any]:
    source = mp11_source_root(COURSE)
    include = mp11_include_dir(COURSE)
    marker_path = mp11_marker_path(COURSE)
    marker = read_dependency_marker(COURSE)
    record = {
        "expected_commit": EXPECTED_MP11_COMMIT,
        "marker_path": str(marker_path),
        "marker_exists": marker_path.exists(),
        "marker_sha256": sha256_file(marker_path) if marker_path.exists() else None,
        "marker": marker,
        "source_root": str(source),
        "include_dir": str(include),
        "source_exists": source.exists(),
        "include_exists": include.exists(),
    }
    if not source.exists() or not include.exists():
        record["status"] = "FAIL"
        return record
    commit = command_record(["git", "-C", str(source), "rev-parse", "HEAD"], 30)
    tree_status = command_record(["git", "-C", str(source), "status", "--short"], 30)
    record["rev_parse"] = commit
    record["worktree_status"] = tree_status
    record["actual_commit"] = commit["stdout"].strip()
    marker_commit = marker.get("LEARNCPP_DEP_COMMIT")
    record["marker_commit"] = marker_commit
    record["worktree_clean"] = tree_status["status"] == "PASS" and not tree_status["stdout"].strip()
    record["status"] = "PASS" if (
        marker_path.exists()
        and marker_commit == EXPECTED_MP11_COMMIT
        and commit["status"] == "PASS"
        and record["actual_commit"] == EXPECTED_MP11_COMMIT
        and record["worktree_clean"]
    ) else "FAIL"
    return record


def record_meta_map(root: Path, include_timing: bool) -> dict[str, Any]:
    dependency = verify_mp11_dependency()
    require_pass("mp11 dependency pin", dependency)
    include_dirs = [mp11_include_dir(COURSE)]
    source_root = root / "generated" / "meta_map"
    records = {"dependency": dependency, "sources": {}, "trace": {}, "correctness": {}, "samples": []}
    for count in META_COUNTS:
        for target in META_TARGETS:
            outputs: dict[str, str] = {}
            for kind in META_KINDS:
                name = meta_variant_name(kind, count, target)
                source = source_root / f"{name}.cpp"
                write_text(source, generate_meta_lookup(kind, count, target))
                records["sources"][name] = {"path": str(source), "sha256": sha256_file(source)}
                emit_event("source", {"group": "meta_map", "kind": kind, "count": count,
                                      "target": target, **records["sources"][name]})
                obj = root / "baseline" / "meta_map" / f"{name}.obj"
                exe = root / "baseline" / "meta_map" / f"{name}.exe"
                compile_result = compile_obj(source, obj, include_dirs=include_dirs)
                require_pass(f"meta_map {name} baseline compile", compile_result)
                link_result = link_exe([obj], exe)
                require_pass(f"meta_map {name} baseline link", link_result)
                run_result = run_exe(exe)
                require_pass(f"meta_map {name} baseline run", run_result)
                outputs[kind] = run_result["stdout"]
                trace = root / "trace" / "meta_map" / f"{name}.json"
                trace_result = compile_obj(source, root / "trace" / "meta_map" / f"{name}.obj",
                                           trace, include_dirs=include_dirs)
                require_pass(f"meta_map {name} trace compile", trace_result)
                object_text = object_record(obj)
                exe_text = object_record(exe)
                require_pass(f"meta_map {name} object section parse", object_text)
                require_pass(f"meta_map {name} exe section parse", exe_text)
                records["correctness"][name] = {
                    "compile": compile_result, "link": link_result, "run": run_result,
                    "object_text": object_text, "exe_text": exe_text, "symbols": symbols(obj),
                }
                records["trace"][name] = {
                    "compile": trace_result,
                    "totals": read_trace(trace) if trace.exists() else {},
                }
            if outputs["manual"] != outputs["mp11"]:
                emit_event("fatal", {"label": "meta_map stdout equality", "count": count,
                                     "target": target, "outputs": outputs})
                raise RuntimeError(f"meta_map stdout equality failed for {count} {target}")
            emit_event("phase_complete", {"phase": "meta_map_correctness_trace",
                                          "count": count, "target": target})
    if not include_timing:
        return records
    rng = random.Random(SEED + 2)
    for count in META_COUNTS:
        for target in META_TARGETS:
            order = list(META_KINDS) * 6
            rng.shuffle(order)
            seen = {kind: 0 for kind in META_KINDS}
            for kind in order:
                seen[kind] += 1
                phase = "warmup" if seen[kind] == 1 else "sample"
                if seen[kind] > 6:
                    continue
                name = meta_variant_name(kind, count, target)
                idle_before = wait_for_idle()
                source = source_root / f"{name}.cpp"
                obj = root / "timing" / "meta_map" / f"{name}_{phase}_{seen[kind]}.obj"
                result = compile_obj(source, obj, include_dirs=include_dirs, perf_counter=True)
                post_probe = active_build_probe()
                overlap = sample_overlap(idle_before, post_probe)
                valid = phase == "sample" and result["status"] == "PASS" and idle_before["idle"] and post_probe["idle"] and overlap["idle"]
                sample = {"group": "meta_map", "count": count, "target": target, "kind": kind,
                          "phase": phase, "index": seen[kind], "valid": valid,
                          "idle_before": idle_before, "compile": result, "post_probe": post_probe,
                          "pre_to_post_overlap": overlap,
                          "artifact_sha256": sha256_file(obj) if result["status"] == "PASS" and obj.exists() else None}
                records["samples"].append(sample)
                emit_event("sample", sample)
    return records


def validate_sample_counts(data: dict[str, Any], require_all: bool = True) -> list[dict[str, Any]]:
    checks = []
    if "type_query" in data:
        type_samples = data.get("type_query", {}).get("samples", [])
        for count in (32, 128, 256):
            for kind in ("recursive", "fold"):
                group = [s for s in type_samples if s.get("count") == count and s.get("kind") == kind]
                warmups = [s for s in group if s.get("phase") == "warmup"
                           and s.get("compile", {}).get("status") == "PASS"
                           and s.get("idle_before", {}).get("idle")]
                valid = [s for s in group if s.get("phase") == "sample" and s.get("valid")]
                status = "PASS" if len(warmups) == 1 and len(valid) == 5 else "FAIL"
                checks.append({"group": "type_query", "count": count, "kind": kind,
                               "warmup_success_count": len(warmups), "valid_sample_count": len(valid),
                               "status": status})
    if "multi_tu" in data:
        multi_samples = data.get("multi_tu", {}).get("samples", [])
        for kind in ("implicit", "explicit"):
            group = [s for s in multi_samples if s.get("kind") == kind]
            warmups = [s for s in group if s.get("phase") == "warmup"
                       and all(c.get("result", {}).get("status") == "PASS" for c in s.get("compiles", []))
                       and s.get("idle_before", {}).get("idle")]
            valid = [s for s in group if s.get("phase") == "sample" and s.get("valid")]
            status = "PASS" if len(warmups) == 1 and len(valid) == 5 else "FAIL"
            checks.append({"group": "multi_tu_compile", "kind": kind,
                           "warmup_success_count": len(warmups), "valid_sample_count": len(valid),
                           "status": status})
    if "meta_map" in data:
        meta_samples = data.get("meta_map", {}).get("samples", [])
        for count in META_COUNTS:
            for target in META_TARGETS:
                for kind in META_KINDS:
                    group = [s for s in meta_samples if s.get("count") == count and s.get("target") == target and s.get("kind") == kind]
                    warmups = [s for s in group if s.get("phase") == "warmup"
                               and s.get("compile", {}).get("status") == "PASS"
                               and s.get("idle_before", {}).get("idle")]
                    valid = [s for s in group if s.get("phase") == "sample" and s.get("valid")]
                    status = "PASS" if len(warmups) == 1 and len(valid) == 5 else "FAIL"
                    checks.append({"group": "meta_map", "count": count, "target": target, "kind": kind,
                                   "warmup_success_count": len(warmups), "valid_sample_count": len(valid),
                                   "status": status})
    if require_all and any(check["status"] != "PASS" for check in checks):
        emit_event("fatal", {"label": "formal sample count validation", "checks": checks})
        raise RuntimeError("formal sample count validation failed")
    return checks


def environment_record() -> dict[str, Any]:
    cpu_command = ["powershell", "-NoProfile", "-Command",
                   "Get-CimInstance Win32_Processor | Select-Object -First 1 Name,NumberOfCores,NumberOfLogicalProcessors | ConvertTo-Json -Compress"]
    cpu = command_record(cpu_command, 30)
    cpu_fallback = None
    if cpu["status"] != "PASS" or not cpu["stdout"].strip():
        fallback_command = ["powershell", "-NoProfile", "-Command",
                            "[pscustomobject]@{ProcessorIdentifier=$env:PROCESSOR_IDENTIFIER; NumberOfLogicalProcessors=$env:NUMBER_OF_PROCESSORS} | ConvertTo-Json -Compress"]
        cpu_fallback = command_record(fallback_command, 30)
    return {
        "clang_version": command_record([str(CLANG), "--version"], 30),
        "python_version": sys.version,
        "platform": {
            "system": platform.system(),
            "release": platform.release(),
            "version": platform.version(),
            "machine": platform.machine(),
            "processor": platform.processor(),
        },
        "cpu": cpu,
        "cpu_fallback": cpu_fallback,
        "clock_info": {
            "monotonic": vars(time.get_clock_info("monotonic")),
            "perf_counter": vars(time.get_clock_info("perf_counter")),
        },
        "hash_timing_note": "Source, object and executable SHA-256 are read after compile/link commands and are not included in process_seconds.",
        "measurement_boundary": "Post-sample detection compares the final pre-sample snapshot with the first immediate post-sample snapshot, then probes one more second. Build activity that starts and finishes wholly between snapshots may be missed; ordinary OS noise is not excluded.",
    }


def median_range(values: list[float]) -> dict[str, float | None]:
    if not values:
        return {"median": None, "min": None, "max": None}
    return {"median": statistics.median(values), "min": min(values), "max": max(values)}


def fmt_seconds(value: float | None) -> str:
    return "INVALID" if value is None else f"{value:.4f}"


def summarize_meta_map(data: dict[str, Any]) -> str:
    lines = ["# C04 B01 元 map 查找成本实验结果", ""]
    mode = data.get("mode", "meta-map")
    lines.append(f"模式：`{mode}`。记录时间：`{data['recorded_utc']}`。随机种子：`{SEED + 2}`。")
    lines.append("")
    lines.append("## 环境")
    lines.append("")
    lines.append(f"- 编译器：`{CLANG}`，sha256 `{data['tools']['clang_sha256']}`")
    version = data.get("environment", {}).get("clang_version", {}).get("stdout", "").splitlines()
    if version:
        lines.append(f"- clang 版本：`{version[0]}`")
    dependency = data["meta_map"]["dependency"]
    lines.append(f"- Mp11：`{dependency['source_root']}`，commit `{dependency.get('actual_commit')}`，期望 `{dependency['expected_commit']}`。")
    lines.append("- 标准与选项：`/std:c++latest /O2 /EHsc /GR-`；manual 与 Mp11 使用同一 TU 契约、同一 `source_map`、同一 `entry_value` 适配、同一 Boost.Mp11 头依赖、同一 include path、同一编译选项。")
    clocks = data.get("environment", {}).get("clock_info", {})
    if clocks:
        perf = clocks.get("perf_counter", {})
        lines.append(f"- 计时 clock：`time.perf_counter()` implementation `{perf.get('implementation')}`，resolution `{perf.get('resolution')}`。")
    lines.append("- `perf_counter_seconds` 包含 Python runner、进程创建、clang-cl 编译和等待返回；源码/产物哈希在命令结束后记录，不计入该值。")
    lines.append("- trace 插桩只用于定位，正式计时不用 trace。")
    if mode == "meta-map-check-only":
        lines.append("")
        lines.append("## Check-only 结果")
        lines.append("")
        lines.append(f"- detector 控制：`{data.get('detector_self_check', {}).get('status')}`。")
        lines.append("- 12 个正例先编译、链接、运行：manual 递归 map_find 与 Mp11 `mp_map_find` 输出相等；缺键为 `void`。")
        lines.append("")
        lines.append("## Trace 定位")
    else:
        lines.append("")
        lines.append("## 样本门")
        lines.append("")
        lines.append("| 组 | 版本 | 项数 | 查询 | 成功预热 | 有效样本 | 状态 |")
        lines.append("|---|---|---:|---|---:|---:|---|")
        for check in data.get("sample_count_checks", []):
            lines.append(f"| {check['group']} | {check.get('kind', '')} | {check.get('count', '')} | {check.get('target', '')} | {check['warmup_success_count']} | {check['valid_sample_count']} | {check['status']} |")
        lines.append("")
        lines.append("## 正式计时")
        lines.append("")
        lines.append("| 项数 | 查询 | 版本 | 有效样本 perf_counter 秒 | 中位数 | 范围 |")
        lines.append("|---:|---|---|---|---:|---|")
        for count in META_COUNTS:
            for target in META_TARGETS:
                for kind in META_KINDS:
                    values = [s["compile"]["perf_counter_seconds"] for s in data["meta_map"]["samples"]
                              if s["valid"] and s["count"] == count and s["target"] == target and s["kind"] == kind]
                    stats = median_range(values)
                    sample_text = ", ".join(f"{v:.4f}" for v in values)
                    lines.append(f"| {count} | {target} | {kind} | {sample_text or 'INVALID'} | {fmt_seconds(stats['median'])} | {fmt_seconds(stats['min'])}-{fmt_seconds(stats['max'])} |")
        lines.append("")
        lines.append("## Trace 定位")
    lines.append("")
    lines.append("| 项数 | 查询 | 版本 | Total Frontend us | Total InstantiateClass count | Total EvaluateAsConstantExpr count |")
    lines.append("|---:|---|---|---:|---:|---:|")
    for count in META_COUNTS:
        for target in META_TARGETS:
            for kind in META_KINDS:
                name = meta_variant_name(kind, count, target)
                totals = data["meta_map"]["trace"][name]["totals"]
                front = totals.get("Total Frontend", {}).get("duration_us", 0)
                cls = totals.get("Total InstantiateClass", {}).get("count", 0)
                ce = totals.get("Total EvaluateAsConstantExpr", {}).get("count", 0)
                lines.append(f"| {count} | {target} | {kind} | {front} | {cls} | {ce} |")
    lines.append("")
    lines.append("## 边界")
    lines.append("")
    lines.append("- 本实验测的是“手写递归 type map 查找”与 Boost.Mp11 `mp_map_find` 的编译成本，不测运行时吞吐。")
    lines.append("- manual/Mp11 的输入类型、输出适配和 Boost.Mp11 头解析成本相同；本实验隔离 `manual_find` 与 `mp_map_find` 的查找机制形状。")
    lines.append("- 0 有效样本会失败；所有预热、无效样本、命令、哈希和 probe 记录保存在 `raw.json`/`events.jsonl`。")
    return "\n".join(lines) + "\n"


def summarize(data: dict[str, Any]) -> str:
    if "meta_map" in data:
        return summarize_meta_map(data)
    lines = ["# C04 B01 编译成本实验结果", ""]
    mode = data.get("mode", "full")
    lines.append(f"模式：`{mode}`。记录时间：`{data['recorded_utc']}`。随机种子：`{SEED}`。")
    lines.append("")
    lines.append("## 环境")
    lines.append("")
    lines.append(f"- 编译器：`{CLANG}`，sha256 `{data['tools']['clang_sha256']}`")
    version = data.get("environment", {}).get("clang_version", {}).get("stdout", "").splitlines()
    if version:
        lines.append(f"- clang 版本：`{version[0]}`")
    cpu_text = data.get("environment", {}).get("cpu", {}).get("stdout", "").strip()
    if not cpu_text:
        cpu_text = data.get("environment", {}).get("cpu_fallback", {}).get("stdout", "").strip()
    if cpu_text:
        lines.append(f"- CPU：`{cpu_text}`")
    os_info = data.get("environment", {}).get("platform", {})
    if os_info:
        lines.append(f"- OS：`{os_info.get('system')} {os_info.get('release')} {os_info.get('version')}`，machine `{os_info.get('machine')}`。")
    lines.append(f"- 标准与选项：`/std:c++latest /O2 /EHsc /GR-`；trace 额外使用 `-ftime-trace` 和 `-ftime-trace-granularity=0`。")
    clocks = data.get("environment", {}).get("clock_info", {})
    if clocks:
        monotonic = clocks.get("monotonic", {})
        perf = clocks.get("perf_counter", {})
        lines.append(f"- 计时 clock：`time.monotonic()` implementation `{monotonic.get('implementation')}`，resolution `{monotonic.get('resolution')}`；`time.perf_counter()` implementation `{perf.get('implementation')}`，resolution `{perf.get('resolution')}`。本实验保持 C01 runner 的 `monotonic` 协议。")
    lines.append(f"- 进程空闲检测：按进程名/PID/CPU 观察 `{', '.join(sorted(BUILD_NAMES))}`；不读取完整 command line。")
    lines.append(f"- 多 TU 计时责任：implicit 为 4 个调用方编译进程；explicit 为 4 个调用方加 1 个 provider/instantiation 编译进程。main 编译与 link 只进入 correctness/section 绑定，不进入正式编译时间样本。")
    lines.append("- 产物 SHA-256 在编译/链接命令结束后读取，不计入 `process_seconds`。")
    lines.append("- 样本值保留 4 位小数用于复算展示；本机 `time.monotonic()` resolution 决定了可分辨边界，不能从 1ms 级差异推断收益。")
    if mode == "check-only":
        lines.append("")
        lines.append("## Check-only 结果")
        lines.append("")
        lines.append("- 本模式只声明 correctness、trace 和 parser 检查通过；不产生正式 timing 结论。")
        lines.append(f"- detector 控制：`{data.get('detector_self_check', {}).get('status')}`。")
        lines.append("- `type_query` 覆盖 found、not-found、empty 边界；`multi_tu` 检查 implicit/explicit stdout 相等。")
        lines.append("")
        lines.append("## 多 TU section 证据")
        lines.append("")
        lines.append("| 版本 | 调用方 .obj .text raw 合计 | 全部 .obj .text raw 合计 | exe .text virtual |")
        lines.append("|---|---:|---:|---:|")
        for kind in ("implicit", "explicit"):
            objects = data["multi_tu"]["correctness"][kind]["object_text"]
            caller = sum(v["text_raw_total"] for k, v in objects.items() if k.startswith("tu"))
            all_obj = sum(v["text_raw_total"] for v in objects.values())
            exe_text = data["multi_tu"]["correctness"][kind]["exe_text"]["text_virtual_total"]
            lines.append(f"| {kind} | {caller} | {all_obj} | {exe_text} |")
        return "\n".join(lines) + "\n"
    lines.append("")
    if "sample_count_checks" in data:
        lines.append("## 样本门")
        lines.append("")
        lines.append("| 组 | 版本 | 项数 | 成功预热 | 有效样本 | 状态 |")
        lines.append("|---|---|---:|---:|---:|---|")
        for check in data["sample_count_checks"]:
            lines.append(f"| {check['group']} | {check.get('kind', '')} | {check.get('count', '')} | {check['warmup_success_count']} | {check['valid_sample_count']} | {check['status']} |")
        lines.append("")
    lines.append("## 类型查询正式计时")
    lines.append("")
    lines.append("| 项数 | 版本 | 有效样本秒 | 中位数 | 范围 |")
    lines.append("|---:|---|---|---:|---|")
    for count in (32, 128, 256):
        for kind in ("recursive", "fold"):
            values = [s["compile"]["process_seconds"] for s in data["type_query"]["samples"]
                      if s["valid"] and s["count"] == count and s["kind"] == kind]
            stats = median_range(values)
            sample_text = ", ".join(f"{v:.4f}" for v in values)
            lines.append(f"| {count} | {kind} | {sample_text or 'INVALID'} | {fmt_seconds(stats['median'])} | {fmt_seconds(stats['min'])}-{fmt_seconds(stats['max'])} |")
    lines.append("")
    lines.append("## 类型查询 trace 定位")
    lines.append("")
    lines.append("| 项数 | 版本 | Total Frontend us | Total InstantiateClass count | Total InstantiateFunction count |")
    lines.append("|---:|---|---:|---:|---:|")
    for count in (32, 128, 256):
        for kind in ("recursive", "fold"):
            totals = data["type_query"]["trace"][f"{kind}_{count}"]["totals"]
            front = totals.get("Total Frontend", {}).get("duration_us", 0)
            cls = totals.get("Total InstantiateClass", {}).get("count", 0)
            fn = totals.get("Total InstantiateFunction", {}).get("count", 0)
            lines.append(f"| {count} | {kind} | {front} | {cls} | {fn} |")
    lines.append("")
    lines.append("## 多 TU 正式计时")
    lines.append("")
    lines.append("| 版本 | 有效样本秒 | 中位数 | 范围 |")
    lines.append("|---|---|---:|---|")
    for kind in ("implicit", "explicit"):
        values = [s["total_compile_process_seconds"] for s in data["multi_tu"]["samples"]
                  if s["valid"] and s["kind"] == kind]
        stats = median_range(values)
        sample_text = ", ".join(f"{v:.4f}" for v in values)
        lines.append(f"| {kind} | {sample_text or 'INVALID'} | {fmt_seconds(stats['median'])} | {fmt_seconds(stats['min'])}-{fmt_seconds(stats['max'])} |")
    lines.append("")
    lines.append("## 多 TU section 证据")
    lines.append("")
    lines.append("| 版本 | 调用方 .obj .text raw 合计 | 全部 .obj .text raw 合计 | exe .text virtual |")
    lines.append("|---|---:|---:|---:|")
    for kind in ("implicit", "explicit"):
        objects = data["multi_tu"]["correctness"][kind]["object_text"]
        caller = sum(v["text_raw_total"] for k, v in objects.items() if k.startswith("tu"))
        all_obj = sum(v["text_raw_total"] for v in objects.values())
        exe_text = data["multi_tu"]["correctness"][kind]["exe_text"]["text_virtual_total"]
        lines.append(f"| {kind} | {caller} | {all_obj} | {exe_text} |")
    lines.append("")
    lines.append("## 边界")
    lines.append("")
    lines.append("- trace 是定位证据，不与正式时间混计。")
    lines.append("- trace 事件有嵌套关系，表格只列 `Total ...` 计数和持续时间，不累计成 CPU 占比。")
    lines.append("- 空闲检测按构建进程名/PID/CPU 增量判断；完全发生在两次快照之间的短任务仍可能漏检，普通 OS 噪声不排除。")
    lines.append("- 无效样本保留在 `raw.json`，不进入中位数。")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--check-only", action="store_true",
                        help="run correctness, trace and parser checks, but skip formal timing samples")
    parser.add_argument("--meta-map", action="store_true",
                        help="run manual type-map lookup versus Boost.Mp11 mp_map_find")
    args = parser.parse_args()
    if not CLANG.exists():
        parser.error(f"missing clang-cl: {CLANG}")
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    root_name = f"meta-map-run-{stamp}" if args.meta_map else f"compile-cost-run-{stamp}"
    root = args.output_root / root_name
    if root.exists():
        parser.error(f"output already exists: {root}")
    root.mkdir(parents=True)
    global EVENT_LOG
    EVENT_LOG = root / "events.jsonl"
    emit_event("start", {"root": str(root)})
    data: dict[str, Any] = {
        "recorded_utc": datetime.now(timezone.utc).isoformat(),
        "repo_head": command_record(["git", "rev-parse", "HEAD"], 30)["stdout"].strip(),
        "mode": ("meta-map-check-only" if args.check_only else "meta-map") if args.meta_map else ("check-only" if args.check_only else "full"),
        "tools": {
            "clang": str(CLANG),
            "clang_sha256": sha256_file(CLANG),
            "llvm_readobj": str(LLVM_READOBJ),
            "llvm_nm": str(LLVM_NM),
            "llvm_size": str(LLVM_SIZE),
            "driver_sha256": sha256_file(Path(__file__)),
            "meta_lookup_driver_sha256": sha256_file(Path(__file__).with_name("meta_lookup_cost.py")),
        },
        "environment": environment_record(),
        "random_seed": SEED + 2 if args.meta_map else SEED,
        "timing_compile_options": ["/nologo", "/c", "/std:c++latest", "/O2", "/EHsc", "/GR-"],
        "trace_extra_options": ["/clang:-ftime-trace=<path>", "/clang:-ftime-trace-granularity=0"],
        "process_detection_scope": sorted(BUILD_NAMES),
    }
    try:
        if args.check_only:
            data["detector_self_check"] = detector_self_check()
            require_pass("detector self-check", data["detector_self_check"])
            data["initial_probe"] = active_build_probe()
        else:
            data["initial_idle"] = wait_for_idle()
            if not data["initial_idle"].get("idle"):
                raise RuntimeError("initial idle validation failed")
        if args.meta_map:
            data["meta_map"] = record_meta_map(root, not args.check_only)
        else:
            data["type_query"] = record_type_query(root, not args.check_only)
            data["multi_tu"] = record_multi_tu(root, not args.check_only)
        if not args.check_only:
            data["sample_count_checks"] = validate_sample_counts(data)
    except BaseException as error:
        data["status"] = "FAIL"
        data["error"] = f"{type(error).__name__}: {error}"
        (root / "raw.json").write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
        emit_event("finish", {"status": "FAIL", "error": data["error"], "raw": str(root / "raw.json")})
        raise
    raw = root / "raw.json"
    data["status"] = "CHECK_ONLY_PASS" if args.check_only else "PASS"
    raw.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    summary = root / "summary.md"
    summary.write_text(summarize(data), encoding="utf-8")
    latest = None
    if not args.check_only and not args.meta_map:
        latest = args.output_root / "latest-summary.md"
        latest.write_text(summary.read_text(encoding="utf-8"), encoding="utf-8")
    emit_event("finish", {"status": data["status"], "raw": str(raw), "summary": str(summary),
                          "latest_summary": None if latest is None else str(latest)})
    print(json.dumps({"status": data["status"], "root": str(root), "raw": str(raw),
                      "summary": str(summary), "latest_summary": None if latest is None else str(latest)},
                     ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
