#!/usr/bin/env bash
set -u

RUN_ID="students-r2-full-20260910-182216-452abc20"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
SNAPSHOT="/root/learncpp-c08-src/${RUN_ID}"
BUILD="/root/learncpp-c08-builds/${RUN_ID}/student-validation"
OUT="${EVIDENCE}/verify-students-r2-full-ninja.stdout.txt"
ERR="${EVIDENCE}/verify-students-r2-full-ninja.stderr.txt"
META="${EVIDENCE}/verify-students-r2-full-ninja.command.json"

start="$(date --iso-8601=seconds)"
C08_STUDENT_VALIDATION_GENERATOR=Ninja \
python3 "${SNAPSHOT}/C08_Concurrency/exercises/tools/verify_students.py" \
  --course "${SNAPSHOT}/C08_Concurrency" \
  --build-root "${BUILD}" \
  --config Release \
  --timeout 120 > "${OUT}" 2> "${ERR}"
rc=$?
end="$(date --iso-8601=seconds)"

python3 - <<PY > "${META}"
import json
print(json.dumps({
    "name": "verify-students-r2-full-ninja",
    "start": "${start}",
    "end": "${end}",
    "exit_code": ${rc},
    "generator": "Ninja",
    "course": "${SNAPSHOT}/C08_Concurrency",
    "build_root": "${BUILD}",
    "stdout": "verify-students-r2-full-ninja.stdout.txt",
    "stderr": "verify-students-r2-full-ninja.stderr.txt"
}, indent=2))
PY
exit "${rc}"
