"""Freeze C03 source/evidence bytes and local build metadata without packaging binaries."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess

COURSE = Path(__file__).resolve().parents[2]
REPO = COURSE.parent
ROOT_EDITS = {"README.md", "LEARNCPP_GLOBAL_PLAN.md",
              "C02_Objects_Lifetime_Ownership/README.md", "C06_Ranges/README.md",
              "C09_Coroutines/README.md", "C10_Execution/README.md"}


def git(*args: str) -> list[str]:
    return subprocess.run(["git", "-c", "core.quotepath=false", *args], cwd=REPO,
        text=True, encoding="utf-8", capture_output=True, check=True, timeout=30).stdout.splitlines()


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def description(path: str) -> str:
    if path in ROOT_EDITS: return "课程导航、先修回链或全局进度"
    if "/src/student/" in path: return "学生独立编辑起点"
    if "/src/reference/" in path: return "完整参考实现"
    if "/validation/good/" in path: return "独立正确完成体"
    if "/validation/bad" in path: return "隔离的错误实现控制"
    if "/chapters/" in path: return "课程正文、推导与解析"
    if "/references/validation/" in path: return "验证/审查原始证据或复现工具，含历史失败"
    if "/checks/" in path: return "公共契约、可信输入设施或实际检查器"
    if path.endswith("README.md"): return "阅读/练习入口与解析"
    if path.endswith(".cpp"): return "可运行观察或能力示例"
    if "CMake" in path or path.endswith(".cmake"): return "构建、配置与测试接线"
    return "课程规格、来源、覆盖或交付说明"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True)
    args = parser.parse_args()
    if not args.tag.replace("-", "").isalnum():
        parser.error("tag must be alphanumeric with optional hyphens")
    output = COURSE / "references/validation/integration" / f"delivery-{args.tag}.json"
    index = COURSE / "references/delivery-manifest.md"
    if output.exists(): parser.error("choose a fresh tag")
    modified = set(git("diff", "--name-only"))
    if modified - ROOT_EDITS:
        raise RuntimeError(f"unexpected tracked edits: {sorted(modified - ROOT_EDITS)}")
    paths = set(git("ls-files", "--others", "--exclude-standard", "--", COURSE.name))
    paths.update(git("ls-files", "--", COURSE.name))
    paths.update(modified)
    paths.difference_update({str(output.relative_to(REPO)).replace("\\", "/"),
                            str(index.relative_to(REPO)).replace("\\", "/")})
    files = []
    for relative in sorted(paths):
        path = REPO / relative
        data = path.read_bytes()
        files.append({"path": relative, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                      "text_lf_sha256": hashlib.sha256(data.replace(b"\r\n", b"\n")).hexdigest(),
                      "summary": description(relative)})
    binaries, commands = [], []
    for preset in ("verify-core", "verify-debug", "asan", "student", "frontier"):
        build = COURSE / "exercises/build" / preset
        for binary in sorted(build.rglob("*.exe")):
            if binary.name.startswith(("L0", "L1", "P1_document", "F01_")):
                binaries.append({"preset": preset, "path": str(binary.relative_to(REPO)),
                                 "sha256": digest(binary), "bytes": binary.stat().st_size})
        for log in sorted(build.rglob("*.command.1.tlog")):
            if "CMakeFiles" in log.parts: continue
            raw = log.read_bytes()
            text = raw.decode("utf-16") if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else raw.decode("utf-8", "replace")
            commands.append({"preset": preset, "path": str(log.relative_to(REPO)),
                             "sha256": hashlib.sha256(raw).hexdigest(), "recorded_command": text})
    result = {"recorded_utc": datetime.now(timezone.utc).isoformat(), "head": git("rev-parse", "HEAD")[0],
              "source_file_count": len(files), "files": files, "local_binaries": binaries,
              "msbuild_command_records": commands,
              "boundaries": ["The index and this JSON exclude themselves to avoid a recursive hash.",
                             "Binaries/tlogs are local metadata only, not files to commit.",
                             "Test verdicts come from the cited JUnit/process records, not binary existence.",
                             "Raw hashes bind local bytes; LF-normalized hashes help identify Git line-ending changes.",
                             "Student include tracing added /showIncludes through process-local CL and VSLANG=1033."]}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    lines = ["# C03 交付文件与冻结指纹", "", f"快照：{args.tag}。本清单记录{len(files)}个文件，含旧失败/复验记录。",
             "源码、正文、数据的原始SHA256及LF归一化SHA均在JSON中；编译产物仅记录元数据，不加入交付文件。",
             f"[完整JSON](validation/integration/{output.name})。清单与当前JSON自身不纳入递归指纹。",
             "", "| 文件 | 一行变更/用途 |", "|---|---|"]
    for entry in files:
        relative = os.path.relpath(REPO / entry["path"], index.parent).replace("\\", "/")
        lines.append(f"| [{entry['path']}](<{relative}>) | {entry['summary']} |")
    index.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps({"files": len(files), "binary_metadata": len(binaries),
                      "command_records": len(commands), "snapshot": str(output)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
