#!/usr/bin/env bash
set -u

RUN_ID="students-r2-full-20260910-182216-452abc20"
BUILD="/root/learncpp-c08-builds/${RUN_ID}/student-validation"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
OUT="${EVIDENCE}/compile-command-samples.txt"

{
  echo "build=${BUILD}"
  echo
  for target in I2_hazard_pointer Capstone3_parallel_compute I3_rcu R2_qsbr; do
    cc="${BUILD}/${target}/good/build/compile_commands.json"
    echo "target=${target}"
    if [[ -f "${cc}" ]]; then
      python3 - <<PY
import json
data=json.load(open("${cc}", encoding="utf-8"))
for item in data:
    cmd=item.get("command","")
    print(cmd)
    print("has_O3=", "-O3" in cmd, sep="")
    print("has_DNDEBUG=", "-DNDEBUG" in cmd, sep="")
PY
    else
      echo "missing_compile_commands"
    fi
    echo
  done
} > "${OUT}" 2>&1

if [[ -f "${BUILD}/summary.json" ]]; then
  cp -f "${BUILD}/summary.json" "${EVIDENCE}/script-summary.json"
fi
