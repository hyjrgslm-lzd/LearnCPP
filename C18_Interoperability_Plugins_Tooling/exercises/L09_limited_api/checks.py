from __future__ import annotations

import importlib.util
import argparse
import re
import shutil
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from run_check import run_process


def fail(message: str) -> int:
    print(f"check failed: {message}")
    return 1


def load(path: str):
    spec = importlib.util.spec_from_file_location(Path(path).name.split(".")[0], path)
    if spec is None or spec.loader is None:
        raise RuntimeError("no extension loader for this binary")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("module")
    parser.add_argument("--imports-tool")
    args = parser.parse_args()
    if sys.platform == "win32":
        tool = args.imports_tool or shutil.which("dumpbin") or shutil.which("objdump")
        if not tool:
            return fail("a real PE import inspector is required")
        option = "/imports" if Path(tool).stem.lower() == "dumpbin" else "-p"
        inspected = run_process([tool, option, str(Path(args.module).resolve())], 15)
        if inspected["status"] != "PASS":
            return fail("PE import inspection failed: " + inspected["stderr"])
        imports = set(re.findall(r"\bpython\d+(?:_d)?\.dll\b", inspected["stdout"].lower()))
        if imports != {"python3.dll"}:
            return fail("extension depends on a version-specific Python DLL")
    module = load(args.module)
    # abi_tag() is descriptive metadata, never proof of the binary ABI.
    table = bytes.maketrans(b"abcdefghijklmnopqrstuvwxyz", b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    for payload in (b"", b"azA\x00\xffm", bytes(range(256)), b"a\x00\xff" * 97):
        if module.transform(payload) != payload.translate(table):
            return fail("limited module broke byte semantics")
    try:
        module.transform(17)
    except TypeError:
        pass
    else:
        return fail("limited module accepted a non-bytes object")
    print(f"L09 limited API PASS on {sys.version.split()[0]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
