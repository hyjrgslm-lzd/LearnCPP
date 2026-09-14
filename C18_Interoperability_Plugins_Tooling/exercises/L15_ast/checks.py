from __future__ import annotations

import importlib.util
import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


def fail(message: str) -> int:
    print(f"check failed: {message}")
    return 1


def load_source(path: Path) -> str:
    spec = importlib.util.spec_from_file_location("c18_l15_solution", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module.source()


def clang_command() -> list[str] | None:
    fixed = Path("/usr/lib/llvm-18/bin/clang++")
    if os.name != "nt" and fixed.exists():
        return [str(fixed), "-std=c++23", "-x", "c++", "-Xclang", "-ast-dump=json", "-fsyntax-only", "-"]
    wsl = shutil.which("wsl")
    if wsl:
        return [
            wsl,
            "-d",
            "LearnCPP-C08-Ubuntu-24.04",
            "--",
            "/usr/lib/llvm-18/bin/clang++",
            "-std=c++23",
            "-x",
            "c++",
            "-Xclang",
            "-ast-dump=json",
            "-fsyntax-only",
            "-",
        ]
    clang = shutil.which("clang++")
    if clang:
        return [clang, "-std=c++23", "-x", "c++", "-Xclang", "-ast-dump=json", "-fsyntax-only", "-"]
    return None


def walk(node: object):
    if isinstance(node, list):
        for item in node:
            yield from walk(item)
    if isinstance(node, dict):
        yield node
        for child in node.get("inner", []):
            yield from walk(child)


def contains_kind(node: object, kind: str) -> bool:
    return any(child.get("kind") == kind for child in walk(node))


def receiver_is_api(member: dict) -> bool:
    for child in walk(member.get("inner", [])):
        if child.get("kind") == "DeclRefExpr":
            qual = child.get("type", {}).get("qualType", "")
            return qual in {"Api", "Api *", "struct Api", "struct Api *"}
    return False


def has_api_process_call(ast: dict) -> bool:
    def is_process_pointer(node: dict) -> bool:
        qual = node.get("type", {}).get("qualType", "")
        return "(*" in qual and ")(" in qual

    api_field_ids = {
        node.get("id")
        for node in walk(ast)
        if node.get("kind") == "FieldDecl" and node.get("name") == "process" and is_process_pointer(node)
    }
    for call in walk(ast):
        if call.get("kind") != "CallExpr":
            continue
        for member in walk(call.get("inner", [])):
            if member.get("kind") != "MemberExpr" or member.get("name") != "process":
                continue
            if member.get("referencedMemberDecl") in api_field_ids and receiver_is_api(member):
                return True
    return False


def run_check(solution: Path) -> tuple[int, str]:
    command = clang_command()
    if command is None:
        return 77, "SKIP: clang++ capability not found"
    code = load_source(solution)
    result = subprocess.run(command, input=code, text=True, capture_output=True, timeout=20)
    if result.returncode != 0:
        combined = result.stdout + result.stderr
        normalized = combined.replace("\x00", "")
        if "Wsl/Service/E_ACCESSDENIED" in normalized or "E_ACCESSDENIED" in normalized:
            return 77, "SKIP: WSL clang capability denied by host service"
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        return 1, "check failed: clang AST dump failed"
    try:
        ast = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        return 1, f"check failed: clang JSON AST parse failed: {exc}"
    required = ["CXXRecordDecl", "FunctionDecl"]
    missing = [kind for kind in required if not contains_kind(ast, kind)]
    if missing:
        return 1, "check failed: AST dump missing " + ",".join(missing)
    if not has_api_process_call(ast):
        return 1, "check failed: AST dump did not contain a resolved call through Api::process"
    return 0, "L15 AST observation PASS"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--expect-reject", action="store_true")
    parser.add_argument("solution")
    args = parser.parse_args(argv[1:])
    rc, message = run_check(Path(args.solution))
    if args.expect_reject:
        if rc == 77:
            print(message)
            return 77
        if rc == 1 and "resolved call through Api::process" in message:
            print("L15 bad implementation rejected PASS")
            return 0
        print(message)
        return fail("bad implementation was not rejected for the target reason")
    print(message)
    if rc == 1:
        return 1
    if rc == 77:
        return 77
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
