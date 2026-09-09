"""Create a new source/evidence manifest and file-by-file delivery inventory."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

REPO = Path(__file__).resolve().parents[3]
CORE = REPO / "C02_Objects_Lifetime_Ownership"
BRIDGES = ["README.md", "LEARNCPP_GLOBAL_PLAN.md", *[
    f"{course}/README.md" for course in (
        "C01_Build_Compile_Link", "C09_Coroutines", "C08_Concurrency", "C10_Execution", "C06_Ranges")]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or not output.is_relative_to(CORE):
        parser.error("output must be a new directory inside C02_Objects_Lifetime_Ownership")
    result = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z",
                             "--", "C02_Objects_Lifetime_Ownership", *BRIDGES], cwd=REPO, check=True,
                            capture_output=True, timeout=30)
    paths = sorted(set(result.stdout.decode("utf-8").split("\0")) - {""})
    sources, evidence, inventory = [], [], []
    for name in paths:
        path = REPO / name
        if not path.is_file() or "/snapshots/" in name or name.endswith("delivery-manifest.md"):
            continue
        if "/validation/reviews/" in name and path.suffix == ".md":
            continue
        if path.suffix.lower() in {".exe", ".dll", ".obj", ".lib", ".pdb", ".vcxproj", ".slnx", ".tlog"}:
            raise RuntimeError(f"generated artifact visible to Git: {name}")
        is_evidence = "/references/validation/" in name and path.suffix.lower() not in {
            ".cpp", ".hpp", ".h", ".cmake", ".py", ".ps1", ".cmd"} and path.name != "CMakeLists.txt"
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        (evidence if is_evidence else sources).append(f"{digest}  {name}\n")
        summary = ("公开原始验证与诊断证据" if is_evidence else
                   "C02导航与实施状态回填" if name in BRIDGES else
                   "正文、解析或课程说明" if path.suffix == ".md" else
                   "构建/验证工具或可复现教学源码")
        inventory.append(f"- `{name}`：{summary}。\n")
    for name in ("C01_Build_Compile_Link/exercises/include/check.hpp",
                 "C01_Build_Compile_Link/exercises/tools/process_runner.py",
                 "C08_Concurrency/exercises/tools/verify_materials.py"):
        sources.append(f"{hashlib.sha256((REPO / name).read_bytes()).hexdigest()}  {name}\n")
    output.mkdir(parents=True)
    for name, lines in (("source.sha256", sources), ("evidence.sha256", evidence)):
        (output / name).write_text("".join(lines), encoding="utf-8")
    summary = {"source_files": len(sources), "evidence_files": len(evidence),
               "source_manifest_sha256": hashlib.sha256((output / "source.sha256").read_bytes()).hexdigest(),
               "evidence_manifest_sha256": hashlib.sha256((output / "evidence.sha256").read_bytes()).hexdigest()}
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    (CORE / "references/delivery-manifest.md").write_text(
        "# C02逐文件交付清单\n\n由git可见文件生成，排除构建产物、清单自身及独立审查签署正文。"
        "源与证据SHA清单在validation/snapshots。本表列范围，不代替质量报告。\n\n" +
        "".join(inventory), encoding="utf-8")
    print(json.dumps(summary))


if __name__ == "__main__":
    main()
