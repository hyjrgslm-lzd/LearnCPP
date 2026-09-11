"""Configure this course with existing pinned sources; no dependency downloads."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "C09_Coroutines/exercises"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--light", action="store_true")
    parser.add_argument("--student", action="store_true")
    parser.add_argument("--deps", type=Path, default=SOURCE / "build/full-windows")
    args = parser.parse_args()
    build = args.build.resolve()
    build.relative_to(SOURCE / "build")
    command = ["cmake", "-S", str(SOURCE), "-B", str(build),
               "-G", "Visual Studio 18 2026", "-A", "x64",
               f"-DCOROUTINE_STUDY_BUILD_REFERENCE={'OFF' if args.student else 'ON'}",
               f"-DCOROUTINE_STUDY_TEST_STARTERS={'ON' if args.student else 'OFF'}",
               f"-DCOROUTINE_STUDY_FETCH_DEPS={'ON' if args.light else 'OFF'}"]
    if args.light:
        deps = args.deps.resolve()
        pins = {"stdexec": "6d7ad689f4d4831c5136e4abe1c601f9a3b64e43",
                "asio": "8806a6803cde7054c3049d3666d3ec36786568c5",
                "cppcoro": "8642e98596a92be30a2b061d3ed306d959d3214e"}
        for name, expected in pins.items():
            actual = subprocess.check_output(
                ["git", "-C", str(deps / "_deps" / f"{name}-src"), "rev-parse", "HEAD"], text=True).strip()
            if actual != expected:
                raise RuntimeError(f"{name}: expected {expected}, found {actual}")
        # These bootstrap files otherwise download even when dependency sources exist.
        for relative in ("_deps/stdexec-build/RAPIDS.cmake", "_deps/stdexec-build/execution.bs",
                         "cmake/CPM_0.38.5.cmake"):
            target = build / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(deps / relative, target)
        command += ["-DFETCHCONTENT_FULLY_DISCONNECTED=ON"]
        for name in ("stdexec", "asio", "cppcoro", "icm", "rapids-cmake"):
            command.append(f"-DFETCHCONTENT_SOURCE_DIR_{name.upper()}={(deps / '_deps' / (name + '-src')).as_posix()}")
        for name in ("STDEXEC", "ASIO", "CPPCORO", "IOCP"):
            command.append(f"-DCOROUTINE_STUDY_ENABLE_{name}=ON")
    result = subprocess.run(command)
    if result.returncode:
        return result.returncode
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    home = next(line.split("=", 1)[1] for line in cache.splitlines()
                if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="))
    if Path(home).resolve() != SOURCE.resolve():
        raise RuntimeError(f"unexpected configured source: {home}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
