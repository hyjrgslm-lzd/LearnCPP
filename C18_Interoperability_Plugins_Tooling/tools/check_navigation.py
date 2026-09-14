"""Reuse the existing Markdown checker and audit C18's root entry."""
from pathlib import Path
import sys

sys.dont_write_bytecode = True
import importlib.util


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    existing = root / "C16_Desktop_Multimedia/tools/check_navigation.py"
    spec = importlib.util.spec_from_file_location("c18_shared_navigation", existing)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    course = root / "C18_Interoperability_Plugins_Tooling"
    failures = module.check_markdown_links(course)
    entry = f"{course.name}/README.md"
    if entry not in module.links(root / "README.md"):
        failures.append(f"root README missing {entry}")
    for failure in failures:
        print(failure, file=sys.stderr)
    print(f"navigation: {len(module.markdown_files(course))} documents, {len(failures)} failures")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
