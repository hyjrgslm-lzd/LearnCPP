"""Check generated VS 2026 projects; optionally compare a pre-refactor build.

Run after CMake generation. No compiler, dependency installation, or source edits.
Example: python tools/verify_exercise_ide.py --build <build-dir> --baseline <old-dir>
Use --require-file TARGET=path-suffix to check independently selected course files.
After an MSVC build, --check-read-headers also checks its actual compiler inputs.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

REPO_ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def normalized(value, build, source_repo=None):
    value = value.replace("\\", "/")
    roots = [(build, "<BUILD>")]
    if source_repo:
        roots.append((source_repo, "<REPO>"))
    roots.append((REPO_ROOT, "<REPO>"))
    for root, replacement in roots:
        value = re.sub(re.escape(root.as_posix()) + r'(?=/|[";\s]|$)', replacement, value)
    return value


def projects(build, source_repo=None):
    solutions = list(build.glob("*.slnx"))
    require(len(solutions) == 1, f"Expected one VS 2026 .slnx in {build}")
    result = {}
    root = ET.parse(solutions[0]).getroot()
    parents = {child: parent for parent in root.iter() for child in parent}
    for item in root.iter("Project"):
        relative = item.get("Path", "")
        if not relative.endswith(".vcxproj"):
            continue
        path = (build / relative).resolve()
        parent = parents[item]
        folder = parent.get("Name", "").strip("/") if parent.tag == "Folder" else ""
        tree = ET.parse(path)
        items = {}
        compile_settings = {}
        for entry in tree.iter():
            tag = entry.tag.rsplit("}", 1)[-1]
            if tag == "ItemDefinitionGroup":
                for tool in entry:
                    if tool.tag.rsplit("}", 1)[-1] == "ClCompile":
                        compile_settings[entry.get("Condition", "")] = [
                            (setting.tag.rsplit("}", 1)[-1], normalized(setting.text or "", build, source_repo))
                            for setting in tool]
            if tag in ("ClCompile", "ClInclude", "None", "CustomBuild") and entry.get("Include"):
                value = entry.get("Include")
                if "$" not in value:
                    value = str((path.parent / value).resolve())
                items.setdefault(tag, set()).add(normalized(value, build, source_repo))
        result[path.stem] = {"path": path, "build": build, "folder": folder, "items": items, "tree": tree,
                             "startup": item.get("DefaultStartup") == "true", "compile_settings": compile_settings}
    require(result, "Solution has no C++ projects")
    return result


def tests(build, config, source_repo=None):
    command = ["ctest", "--test-dir", str(build), "-C", config, "--show-only=json-v1"]
    completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", check=True)
    result = {}
    for test in json.loads(completed.stdout)["tests"]:
        # CTest omits commands for not-yet-built executables. Compare their
        # generated registrations below instead of depending on build order.
        result[test["name"]] = normalized(json.dumps(test.get("properties", []), sort_keys=True), build, source_repo)
    return result


def test_commands(build, source_repo=None):
    commands, pending, visited = [], [build], set()
    while pending:
        directory = pending.pop().resolve()
        if directory in visited:
            continue
        visited.add(directory)
        path = directory / "CTestTestfile.cmake"
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            line = line.strip()
            if line.startswith("add_test("):
                # C04's isolated compile cases hash parent build + test name.
                # Validate that hash before normalizing the output directory.
                diagnostic = re.search(r"/build/_diag/([0-9a-f]{12})/", line)
                if diagnostic:
                    name = re.match(r"add_test\(\[=\[(.*?)\]=\]", line)
                    require(name is not None, f"Unknown diagnostic test registration: {line}")
                    expected = hashlib.sha256(f"{build.as_posix()}|{name.group(1)}".encode()).hexdigest()[:12]
                    require(diagnostic.group(1) == expected, f"Unexpected diagnostic build key for {name.group(1)}")
                    line = line.replace(f"/build/_diag/{expected}/", "/build/_diag/<KEY>/")
                commands.append(normalized(line, build, source_repo))
            elif line.startswith("subdirs("):
                match = re.fullmatch(r'subdirs\("([^\"]+)"\)', line)
                require(match is not None, f"Unsupported generated CTest subdirectory: {line}")
                pending.append(directory / match.group(1))
    return sorted(commands)


def verify_layout(current):
    def executable(project):
        return any(node.tag.endswith("}ConfigurationType") and node.text == "Application"
                   for node in project["tree"].iter())

    startups = [project for project in current.values() if project["startup"]]
    runnable = any(executable(project) and not project["folder"].startswith("_") for project in current.values())
    if runnable:
        require(len(startups) == 1, "Expected exactly one default startup project")
        startup = startups[0]
        require(startup["folder"] and "/" not in startup["folder"] and not startup["folder"].startswith("_"),
                "Default startup is not a primary lesson project")
        require(executable(startup), "Default startup is not executable")
    else:
        # CMake can select ALL_BUILD in a checks-only or custom-build solution.
        require(len(startups) <= 1 and all(project["folder"].startswith("_") for project in startups),
                "Unexpected lesson startup in a solution without runnable lessons")
    lesson_projects = 0
    for name, project in current.items():
        folder = project["folder"]
        require(folder, f"{name}: project is outside a solution folder")
        auxiliary = folder.startswith(("_Course/Checks", "_Course/Benchmarks"))
        if folder.startswith("_CMake") or (folder.startswith("_Course") and not auxiliary):
            continue
        # Upstream dependencies can retain their own existing folder hierarchy.
        if not auxiliary and not re.match(r"^(?:[0-9]{2}_|[A-Z][0-9]|CAPSTONE|Capstone)", folder):
            continue
        lesson_projects += 1
        items = project["items"]
        all_files = set().union(*items.values()) if items else set()
        compiled = items.get("ClCompile", set())
        filters_path = Path(str(project["path"]) + ".filters")
        require(filters_path.exists(), f"{name}: no file filters")
        filters = ET.parse(filters_path)
        grouped = set()
        for entry in filters.iter():
            if entry.get("Include") and any(child.tag.endswith("}Filter") for child in entry):
                value = entry.get("Include")
                if "$" not in value:
                    value = str((project["path"].parent / value).resolve())
                grouped.add(normalized(value, project["build"]))
        displayed = set().union(*(items.get(kind, set()) for kind in ("ClCompile", "ClInclude", "None")))
        displayed = {file for file in displayed if not (file.startswith("<BUILD>/") and "/CMakeFiles/" in file)}
        require(displayed <= grouped, f"{name}: files lack filters: {sorted(displayed-grouped)}")
        for entry in project["tree"].iter():
            if entry.tag.endswith("}ConfigurationType") and entry.text == "Application":
                working_dirs = [node.text for node in project["tree"].iter()
                                if node.tag.endswith("}LocalDebuggerWorkingDirectory")]
                require(working_dirs and all(working_dirs), f"{name}: debugger working directory missing")
                break
        if name.endswith("_student"):
            require(not any(re.search(r"/(?:reference|validation)/|/solution[.]cpp$", file)
                            for file in compiled), f"{name}: student compiles answer/control sources")
            require(any(file.endswith("/README.md") for file in all_files), f"{name}: README missing")
            require(not any(re.search(r"/reference/.*[.](?:hpp|h|cpp)$", file) for file in all_files),
                    f"{name}: reference implementation appears in the student project")
        # Primary projects live directly in the unit folder.
        if "/" not in folder and compiled:
            require(any(file.endswith("/README.md") for file in all_files), f"{name}: primary README missing")
    require(lesson_projects, "No lesson projects were checked")
    return lesson_projects


def compare(current, baseline):
    removed = set(baseline) - set(current)
    added = set(current) - set(baseline)
    rpc_library = "Capstone4_rpc_framework_student_lib"
    require(not added, f"Unexpected new targets: {sorted(added)}")
    require(removed <= {rpc_library}, f"Unexpected removed targets: {sorted(removed)}")
    rpc_sources = baseline.get(rpc_library, {}).get("items", {}).get("ClCompile", set())
    for name in current.keys() & baseline.keys():
        require(current[name]["compile_settings"] == baseline[name]["compile_settings"],
                f"{name}: compiler settings changed")
        before = baseline[name]["items"].get("ClCompile", set())
        after = current[name]["items"].get("ClCompile", set())
        if rpc_library in removed and name in ("Capstone4_rpc_framework", "Capstone4_rpc_framework_student_check"):
            before = before | rpc_sources
        require(before == after,
                f"{name}: compiled sources changed; added={sorted(after-before)}, removed={sorted(before-after)}")


def verify_read_headers(build, current):
    """Use MSVC's recorded reads instead of guessing C++ include resolution."""
    repo = REPO_ROOT
    prefix = repo.as_posix().casefold() + "/"
    reads, logs = {}, 0
    for log in build.rglob("CL.read.1.tlog"):
        target = log.parent.parent.parent.name.removesuffix(".dir")
        if target not in current:
            continue
        logs += 1
        for line in log.read_text(encoding="utf-16").splitlines():
            path = line.replace("\\", "/").casefold()
            if not path.startswith(prefix) or not path.endswith((".h", ".hpp", ".hxx")):
                continue
            relative = path[len(prefix):]
            if any(part.startswith("build") or part in ("_deps", "third_party", "vcpkg_installed")
                   for part in relative.split("/")[:-1]):
                continue
            reads.setdefault(target, set()).add("<repo>/" + relative)
    require(logs, "No MSVC compiler read logs; build the requested targets first")
    missing = []
    for target, headers in reads.items():
        visible = {file.casefold() for files in current[target]["items"].values() for file in files}
        missing.extend(f"{target}: {header}" for header in sorted(headers-visible))
    require(not missing, "Compiled headers missing from projects:\n" + "\n".join(missing))
    return sum(len(headers) for headers in reads.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--baseline-repo", type=Path,
                        help="Repository root of a Git-exported baseline source snapshot")
    parser.add_argument("--require-file", action="append", default=[], metavar="TARGET=SUFFIX")
    parser.add_argument("--check-read-headers", action="store_true",
                        help="After building, require all MSVC-read course headers in their projects")
    args = parser.parse_args()
    require(not args.baseline_repo or args.baseline, "--baseline-repo requires --baseline")
    build = args.build.resolve()
    current = projects(build)
    count = verify_layout(current)
    for requirement in args.require_file:
        target, separator, suffix = requirement.partition("=")
        require(separator and suffix and target in current, f"Invalid file requirement: {requirement}")
        files = set().union(*current[target]["items"].values())
        require(any(file.endswith(suffix.replace("\\", "/")) for file in files),
                f"{target}: required file missing: {suffix}")
    if args.baseline:
        baseline = args.baseline.resolve()
        source_repo = args.baseline_repo.resolve() if args.baseline_repo else None
        if source_repo:
            cache = (baseline / "CMakeCache.txt").read_text(encoding="utf-8-sig")
            home = re.search(r"^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$", cache, re.MULTILINE)
            require(home is not None and Path(home.group(1).strip()).resolve().is_relative_to(source_repo),
                    "Baseline was not configured from --baseline-repo")
        compare(current, projects(baseline, source_repo))
        require(test_commands(baseline, source_repo) == test_commands(build), "Generated CTest commands changed")
        for config in ("Debug", "Release"):
            before, after = tests(baseline, config, source_repo), tests(build, config)
            require(before == after,
                    f"{config}: CTest registrations changed: "
                    f"{sorted(name for name in before.keys() | after.keys() if before.get(name) != after.get(name))}")
    header_count = verify_read_headers(build, current) if args.check_read_headers else None
    print(f"PASS: {count} course projects; folders, file filters, debugger directories"
          + ("; baseline sources, compiler settings and Debug/Release test registrations preserved" if args.baseline else "")
          + (f"; {header_count} compiler-read header associations verified" if header_count is not None else ""))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, ET.ParseError, subprocess.CalledProcessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
