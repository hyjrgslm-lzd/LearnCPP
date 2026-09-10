#!/usr/bin/env bash
set -u

RUN_ID="student-portability-20260910-175508-13ddcee8"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
SNAPSHOT="/root/learncpp-c08-src/${RUN_ID}"
BUILD="/root/learncpp-c08-builds/${RUN_ID}/student-validation"
OUT="${EVIDENCE}/verify-students-ninja-good.stdout.txt"
ERR="${EVIDENCE}/verify-students-ninja-good.stderr.txt"
META="${EVIDENCE}/verify-students-ninja-good.command.json"

start="$(date --iso-8601=seconds)"
C08_STUDENT_VALIDATION_GENERATOR=Ninja \
python3 "${SNAPSHOT}/C08_Concurrency/exercises/tools/verify_students.py" \
  --course "${SNAPSHOT}/C08_Concurrency" \
  --build-root "${BUILD}" \
  --target I2_hazard_pointer \
  --target Capstone3_parallel_compute \
  --mode good \
  --config Release \
  --timeout 120 > "${OUT}" 2> "${ERR}"
rc=$?
end="$(date --iso-8601=seconds)"

python3 - <<PY > "${META}"
import json
print(json.dumps({
    "name": "verify-students-ninja-good",
    "start": "${start}",
    "end": "${end}",
    "exit_code": ${rc},
    "generator": "Ninja",
    "course": "${SNAPSHOT}/C08_Concurrency",
    "build_root": "${BUILD}",
    "stdout": "verify-students-ninja-good.stdout.txt",
    "stderr": "verify-students-ninja-good.stderr.txt"
}, indent=2))
PY

exit "${rc}"
