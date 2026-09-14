from __future__ import annotations

import argparse
import hashlib
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


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tool_args(tool: str) -> list[str]:
    if Path(tool).stem == "c18_tool":
        return [tool, "--c18-subcommand=rewrite"]
    return [tool]


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=["plain", "reject", "guard-input", "existing-output"], required=True)
    parser.add_argument("--expect", default="")
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("tool")
    parser.add_argument("source")
    parser.add_argument("compile_args", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv[1:])
    source = Path(args.source)
    before = sha(source)
    out_dir = Path(args.out_dir)
    if args.mode == "guard-input":
        out_dir = source.parent
        args.expect = args.expect or "output equals input"
    if args.mode == "existing-output":
        out_dir.mkdir(parents=True, exist_ok=True)
        existing = out_dir / source.name
        existing.write_text("sentinel\n", encoding="utf-8")
        args.expect = args.expect or "output already exists"
    command = [
        *tool_args(args.tool),
        "--old-symbol=c18_process_old",
        "--new-symbol=c18_process",
        f"--out-dir={out_dir}",
        str(source),
        *args.compile_args,
    ]
    should_fail = args.mode in {"reject", "guard-input", "existing-output"}
    ok, text = run(command, expect=1 if should_fail else 0, needle=args.expect)
    if not ok:
        print(text, end="")
        return fail("rewrite native tool result did not match expectation")
    if sha(source) != before:
        return fail("rewrite modified input file")
    if args.mode == "reject" and (out_dir / source.name).exists():
        return fail("rewrite rejection created output copy")
    if args.mode == "existing-output" and (out_dir / source.name).read_text(encoding="utf-8") != "sentinel\n":
        return fail("rewrite overwrote existing output")
    if args.mode == "plain":
        edited = out_dir / source.name
        if not edited.exists():
            return fail("rewrite did not create output copy")
        body = edited.read_text(encoding="utf-8")
        if "c18_process(" not in body or "c18_process_old" in body:
            return fail("rewrite output did not migrate declaration and reference")
        second = out_dir.parent / (out_dir.name + "-again")
        ok, text = run([
            *tool_args(args.tool),
            "--old-symbol=c18_process_old",
            "--new-symbol=c18_process",
            f"--out-dir={second}",
            str(edited),
            *args.compile_args,
        ])
        if not ok:
            print(text, end="")
            return fail("rewrite was not idempotent after migration")
    print("native rewrite behavior PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
