from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def fail(message: str) -> int:
    print(f"check failed: {message}")
    return 1


def run(command: list[str], expect: int = 0, needle: str = "") -> tuple[bool, str]:
    result = subprocess.run(command, text=True, capture_output=True, timeout=30)
    text = result.stdout + result.stderr
    return result.returncode == expect and (not needle or needle in text), text


def tool_args(tool: str) -> list[str]:
    if Path(tool).stem == "c18_tool":
        return [tool, "--c18-subcommand=generate"]
    return [tool]


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--expect-failure", default="")
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--include", required=True)
    parser.add_argument("--precreate", action="append", default=[])
    parser.add_argument("tool")
    parser.add_argument("header")
    parser.add_argument("compile_args", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv[1:])
    out_dir = Path(args.out_dir)
    precreated = []
    if args.precreate:
        out_dir.mkdir(parents=True, exist_ok=True)
        for name in args.precreate:
            path = out_dir / name
            path.write_text("sentinel\n", encoding="utf-8")
            precreated.append(path)
    ok, text = run(
        [*tool_args(args.tool), f"--out-dir={out_dir}", args.header, *args.compile_args],
        expect=1 if args.expect_failure else 0,
        needle=args.expect_failure,
    )
    if not ok:
        print(text, end="")
        return fail("generate native tool result did not match expectation")
    if args.expect_failure:
        for path in precreated:
            if path.read_text(encoding="utf-8") != "sentinel\n":
                return fail("generate overwrote existing output")
        print("native codegen bad rejected PASS")
        return 0
    manifest = out_dir / "exports.txt"
    c_file = out_dir / "contract_check.c"
    cpp_file = out_dir / "contract_check.cpp"
    if not manifest.exists() or not c_file.exists() or not cpp_file.exists():
        return fail("generator missed manifest or contract files")
    if "c18_get_api" not in manifest.read_text(encoding="utf-8"):
        return fail("manifest missed c18_get_api")
    checks = [
        [args.cc, "-std=c11", "-I", args.include, "-fsyntax-only", str(c_file)],
        [args.cxx, "-std=c++23", "-I", args.include, "-fsyntax-only", str(cpp_file)],
    ]
    for command in checks:
        ok, text = run(command)
        if not ok:
            print(text, end="")
            return fail("generated consumer did not compile")
    print("native codegen behavior PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
