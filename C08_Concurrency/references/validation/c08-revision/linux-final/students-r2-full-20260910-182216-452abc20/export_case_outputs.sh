#!/usr/bin/env bash
set -u

RUN_ID="students-r2-full-20260910-182216-452abc20"
BUILD="/root/learncpp-c08-builds/${RUN_ID}/student-validation"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
DEST="${EVIDENCE}/case-outputs"
INDEX="${EVIDENCE}/public-index.json"
MANIFEST="${EVIDENCE}/public-index.sha256.txt"

mkdir -p "${DEST}"

find "${BUILD}" -mindepth 3 -maxdepth 3 -type d -name logs -print | sort | while read -r logs; do
  rel="${logs#${BUILD}/}"
  target="${rel%%/*}"
  rest="${rel#*/}"
  mode="${rest%%/*}"
  outdir="${DEST}/${target}/${mode}"
  mkdir -p "${outdir}"
  find "${logs}" -maxdepth 1 -type f \( -name "*.txt" -o -name "*.json" \) -print | sort | while read -r file; do
    cp -f "${file}" "${outdir}/$(basename "${file}")"
  done
done

python3 - <<'PY' > "${INDEX}"
import hashlib
import json
from pathlib import Path

evidence = Path("/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/students-r2-full-20260910-182216-452abc20")
dest = evidence / "case-outputs"
entries = []
for path in sorted(dest.rglob("*")):
    if not path.is_file():
        continue
    if path.is_symlink():
        raise SystemExit(f"symlink not allowed: {path}")
    if path.suffix not in {".txt", ".json"}:
        raise SystemExit(f"unexpected file extension: {path}")
    data = path.read_bytes()
    rel = path.relative_to(evidence).as_posix()
    parts = path.relative_to(dest).parts
    if len(parts) != 3:
        raise SystemExit(f"unexpected exported path: {rel}")
    target, mode, name = parts
    entries.append({
        "target": target,
        "mode": mode,
        "path": rel,
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
    })
cases = sorted({(e["target"], e["mode"]) for e in entries})
targets = sorted({e["target"] for e in entries})
modes = sorted({e["mode"] for e in entries})
manifest_rows = [f'{e["sha256"]}  {e["path"]}' for e in entries]
(evidence / "public-index.sha256.txt").write_text("\n".join(manifest_rows) + "\n", encoding="utf-8")
summary = {
    "runId": "students-r2-full-20260910-182216-452abc20",
    "sourceBuildRoot": "/root/learncpp-c08-builds/students-r2-full-20260910-182216-452abc20/student-validation",
    "exportRoot": "case-outputs",
    "caseCount": len(cases),
    "targetCount": len(targets),
    "modes": modes,
    "fileCount": len(entries),
    "manifestSha256": hashlib.sha256(("\n".join(manifest_rows) + "\n").encode("utf-8")).hexdigest(),
    "entries": entries,
}
print(json.dumps(summary, indent=2, ensure_ascii=False))
PY

python3 - <<'PY' > "${EVIDENCE}/case-output-consistency.json"
import json
from pathlib import Path

evidence = Path("/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/students-r2-full-20260910-182216-452abc20")
stdout_cases = []
for line in (evidence / "verify-students-r2-full-ninja.stdout.txt").read_text(encoding="utf-8").splitlines():
    if line.startswith("{") and '"target"' in line:
        item = json.loads(line)
        stdout_cases.append((item["target"], item["mode"]))
index = json.loads((evidence / "public-index.json").read_text(encoding="utf-8"))
export_cases = sorted({(entry["target"], entry["mode"]) for entry in index["entries"]})
required_files = {"configure.json", "configure.stdout.txt", "configure.stderr.txt", "build.json", "build.stdout.txt", "build.stderr.txt", "run.json", "run.stdout.txt", "run.stderr.txt"}
missing = []
for target, mode in export_cases:
    names = {entry["path"].split("/")[-1] for entry in index["entries"] if entry["target"] == target and entry["mode"] == mode}
    miss = sorted(required_files - names)
    if miss:
        missing.append({"target": target, "mode": mode, "missing": miss})
summary = {
    "stdoutCaseCount": len(stdout_cases),
    "exportCaseCount": len(export_cases),
    "casesMatch": sorted(stdout_cases) == export_cases,
    "missingRequiredCaseFiles": missing,
}
print(json.dumps(summary, indent=2, ensure_ascii=False))
PY
