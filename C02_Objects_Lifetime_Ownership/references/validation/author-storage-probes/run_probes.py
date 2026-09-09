"""Run bounded C02 capability probes and write JSON evidence."""
from __future__ import annotations

import json
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import time
from typing import Any

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "Engineering_Study/exercises/tools"))
from process_runner import run_process

SOURCE_DIR = Path(__file__).resolve().parent
BUILD_DIR = ROOT / "build/c02-storage-probes"
EVIDENCE_DIR = BUILD_DIR / "evidence"
ARCHIVE_DIR = SOURCE_DIR / "evidence"
CLANGXX = Path("D:/VisualStudio2026/Installed/VC/Tools/Llvm/x64/bin/clang++.exe")
VSDEVCMD = Path("D:/VisualStudio2026/Installed/Common7/Tools/VsDevCmd.bat")
MSVC_TOOLS = Path("D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231")
CL = MSVC_TOOLS / "bin/Hostx64/x64/cl.exe"
RUN_TARGETS = ["env_report", "implicit_move", "range_for_lifetime_macro", "start_lifetime_as"]
FRONTIER_SOURCES = ["frontier_base_designated.cpp", "frontier_return_temp_ref.cpp"]


def unique_evidence_path() -> Path:
    EVIDENCE_DIR.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    candidate = EVIDENCE_DIR / f"storage-probes-{stamp}.json"
    if not candidate.exists():
        return candidate
    for index in range(1, 100):
        candidate = EVIDENCE_DIR / f"storage-probes-{stamp}-{index}.json"
        if not candidate.exists():
            return candidate
    raise RuntimeError("could not allocate unique evidence filename")


def unique_archive_path(name: str) -> Path:
    ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    candidate = ARCHIVE_DIR / name
    if not candidate.exists():
        return candidate
    stem = candidate.stem
    suffix = candidate.suffix
    for index in range(1, 100):
        candidate = ARCHIVE_DIR / f"{stem}-{index}{suffix}"
        if not candidate.exists():
            return candidate
    raise RuntimeError("could not allocate unique archive filename")


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def source_fingerprints() -> dict[str, str]:
    files = [SOURCE_DIR / "CMakeLists.txt", SOURCE_DIR / "run_probes.py"]
    files.extend(sorted((SOURCE_DIR / "probes").glob("*.cpp")))
    return {str(path.relative_to(SOURCE_DIR)).replace("\\", "/"): file_sha256(path) for path in files}


def run(command: list[str], timeout: float = 180) -> dict[str, Any]:
    return run_process(command, timeout)


def vsdev_env() -> tuple[dict[str, str] | None, dict[str, Any]]:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    script = BUILD_DIR / "capture-vsdev-env.cmd"
    script.write_text(
        f'@echo off\n'
        f'call "{VSDEVCMD}" -arch=x64 -host_arch=x64 >nul\n'
        f'set VSLANG=1033\n'
        f'set\n',
        encoding="utf-8",
    )
    result = run(["cmd.exe", "/d", "/c", str(script)], 60)
    if result["exit_code"] != 0 or result["timeout"] or result["error"] or result["cleanup_error"]:
        return None, result
    env = os.environ.copy()
    raw_stdout = result["stdout"]
    for line in raw_stdout.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            env[key] = value
    env["VSLANG"] = "1033"
    allowed = {
        "CommandPromptType", "DevEnvDir", "INCLUDE", "LIB", "LIBPATH",
        "UCRTVersion", "UniversalCRTSdkDir", "VCINSTALLDIR", "VCToolsInstallDir",
        "VCToolsVersion", "VisualStudioVersion", "VSCMD_ARG_HOST_ARCH",
        "VSCMD_ARG_TGT_ARCH", "VSCMD_VER", "VSLANG", "VSINSTALLDIR", "WindowsSDKVersion",
    }
    result["stdout"] = "\n".join(
        line for line in raw_stdout.splitlines()
        if line.split("=", 1)[0] in allowed
    ) + "\n"
    return env, result


def run_with_env(command: list[str], env: dict[str, str], timeout: float = 180) -> dict[str, Any]:
    started = time.monotonic()
    settings: dict[str, Any] = {"creationflags": subprocess.CREATE_NO_WINDOW | subprocess.CREATE_NEW_PROCESS_GROUP}
    try:
        completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8",
                                   errors="replace", timeout=timeout, env=env, **settings)
        return {"command": command, "exit_code": completed.returncode, "timeout": False,
                "process_seconds": time.monotonic() - started, "stdout": completed.stdout,
                "stderr": completed.stderr, "cleanup_error": "", "error": "",
                "status": "PASS" if completed.returncode == 0 else "FAIL"}
    except subprocess.TimeoutExpired as error:
        return {"command": command, "exit_code": None, "timeout": True,
                "process_seconds": time.monotonic() - started, "stdout": error.stdout or "",
                "stderr": error.stderr or "", "cleanup_error": "", "error": str(error),
                "status": "FAIL"}
    except OSError as error:
        return {"command": command, "exit_code": None, "timeout": False,
                "process_seconds": time.monotonic() - started, "stdout": "", "stderr": "",
                "cleanup_error": "", "error": f"launch failed: {error}", "status": "FAIL"}


def main() -> int:
    evidence: dict[str, Any] = {
        "source_dir": str(SOURCE_DIR),
        "build_dir": str(BUILD_DIR),
        "msvc_tools": str(MSVC_TOOLS),
        "clangxx": str(CLANGXX),
        "vsdevcmd": str(VSDEVCMD),
        "source_fingerprints": source_fingerprints(),
        "tool_versions": {},
        "steps": {},
        "matrix": {},
        "notes": [
            "range-for lifetime is macro-only here; no dangling range is executed",
            "provenance, invalid pointer, lifetime-end DRs, erroneous initialization, and defaulted assignment restrictions are not claimed as runtime-probed",
        ],
    }
    evidence["steps"]["tool_cmake_version"] = run(["cmake", "--version"], 30)
    evidence["tool_versions"]["cmake"] = evidence["steps"]["tool_cmake_version"]["stdout"].splitlines()[:1]
    evidence["steps"]["tool_clang_version"] = run([str(CLANGXX), "--version"], 30)
    evidence["tool_versions"]["clang++"] = evidence["steps"]["tool_clang_version"]["stdout"].splitlines()[:1]

    msvc_build = BUILD_DIR / "msvc-vs18"
    evidence["steps"]["msvc_configure"] = run([
        "cmake", "-S", str(SOURCE_DIR), "-B", str(msvc_build),
        "-G", "Visual Studio 18 2026", "-A", "x64",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ])
    for target in RUN_TARGETS:
        build_key = f"msvc_build_{target}"
        evidence["steps"][build_key] = run([
            "cmake", "--build", str(msvc_build), "--config", "Release", "--target", target
        ])
        exe = msvc_build / "Release" / f"{target}.exe"
        run_key = f"msvc_run_{target}"
        evidence["steps"][run_key] = run([str(exe)], 30) if exe.exists() else {
            "command": [str(exe)], "exit_code": None, "timeout": False, "process_seconds": 0,
            "stdout": "", "stderr": "", "cleanup_error": "", "error": "executable not built", "status": "FAIL",
        }
        evidence["matrix"][f"msvc:{target}"] = {
            "build": evidence["steps"][build_key]["status"],
            "run": evidence["steps"][run_key]["status"],
        }

    env, env_step = vsdev_env()
    evidence["steps"]["vsdev_env"] = env_step
    if env is not None:
        version_source = BUILD_DIR / "cl-version-probe.cpp"
        version_source.write_text("int main() { return 0; }\n", encoding="utf-8")
        version_obj = BUILD_DIR / "cl-version-probe.obj"
        evidence["steps"]["tool_cl_bv"] = run_with_env([
            str(CL), "/nologo", "/Bv", "/c", str(version_source), f"/Fo{version_obj}"
        ], env, 30)
        evidence["tool_versions"]["cl"] = (
            evidence["steps"]["tool_cl_bv"]["stdout"] + evidence["steps"]["tool_cl_bv"]["stderr"]
        ).splitlines()[:12]
        clang_build = BUILD_DIR / "clang"
        msvc_frontier_build = BUILD_DIR / "msvc-frontier"
        clang_build.mkdir(parents=True, exist_ok=True)
        msvc_frontier_build.mkdir(parents=True, exist_ok=True)
        for target in RUN_TARGETS:
            source = SOURCE_DIR / "probes" / f"{target}.cpp"
            exe = clang_build / f"{target}.exe"
            build_key = f"clang_build_{target}"
            evidence["steps"][build_key] = run_with_env([
                str(CLANGXX), "-std=c++23", "-Wall", "-Wextra", "-Wpedantic",
                str(source), "-o", str(exe)
            ], env)
            run_key = f"clang_run_{target}"
            evidence["steps"][run_key] = run_with_env([str(exe)], env, 30) if exe.exists() else {
                "command": [str(exe)], "exit_code": None, "timeout": False, "process_seconds": 0,
                "stdout": "", "stderr": "", "cleanup_error": "", "error": "executable not built", "status": "FAIL",
            }
            evidence["matrix"][f"clang:{target}"] = {
                "build": evidence["steps"][build_key]["status"],
                "run": evidence["steps"][run_key]["status"],
            }

        for source_name in FRONTIER_SOURCES:
            source = SOURCE_DIR / "probes" / source_name
            msvc_obj = msvc_frontier_build / f"{source.stem}.obj"
            msvc_key = f"msvc_frontier_{source.stem}"
            evidence["steps"][msvc_key] = run_with_env([
                str(CL), "/nologo", "/std:c++latest", "/EHsc", "/W4", "/permissive-",
                "/Zc:__cplusplus", "/c", str(source), f"/Fo{msvc_obj}"
            ], env)
            evidence["matrix"][f"msvc:c++latest:{source.stem}"] = {
                "compile_only": evidence["steps"][msvc_key]["status"],
            }

            obj = clang_build / f"{source.stem}.obj"
            key = f"clang_frontier_{source.stem}"
            evidence["steps"][key] = run_with_env([
                str(CLANGXX), "-std=c++2c", "-Wall", "-Wextra", "-Wpedantic",
                "-c", str(source), "-o", str(obj)
            ], env)
            evidence["matrix"][f"clang:c++2c:{source.stem}"] = {
                "compile_only": evidence["steps"][key]["status"],
            }

    output = unique_evidence_path()
    with output.open("x", encoding="utf-8") as handle:
        json.dump(evidence, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    archive = unique_archive_path(output.name)
    archive.write_text(output.read_text(encoding="utf-8"), encoding="utf-8")
    print(output)
    print(archive)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
