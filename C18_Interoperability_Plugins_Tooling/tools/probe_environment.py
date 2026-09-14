"""Report installed capabilities without importing/downloading optional dependencies."""
from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import sysconfig


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lua-prefix", type=Path)
    parser.add_argument("--llvm-prefix", type=Path)
    args = parser.parse_args()
    prefix = args.llvm_prefix
    if prefix is None and sys.platform != "win32" and Path("/usr/lib/llvm-18").is_dir():
        prefix = Path("/usr/lib/llvm-18")
    python_h = Path(sysconfig.get_path("include")) / "Python.h"
    lua_h = args.lua_prefix / "include/lua.h" if args.lua_prefix else None
    tooling_h = prefix / "include/clang/Tooling/Tooling.h" if prefix else None
    report = {
        "python": {"executable": sys.executable, "version": sys.version.split()[0], "header": str(python_h), "header_present": python_h.is_file()},
        "pybind11": {"python_package_present": importlib.util.find_spec("pybind11") is not None},
        "lua": {"executable": shutil.which("lua"), "requested_header": str(lua_h) if lua_h else None, "header_present": lua_h.is_file() if lua_h else None},
        "clang": {"executable": shutil.which("clang++") or (str(prefix / "bin/clang++") if prefix and (prefix / "bin/clang++").is_file() else None), "requested_tooling_header": str(tooling_h) if tooling_h else None, "tooling_header_present": tooling_h.is_file() if tooling_h else None},
        "meaning": "Inventory only: header presence is not compile/link/runtime validation. Missing explicit prefix is unknown, not proof of global absence.",
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
