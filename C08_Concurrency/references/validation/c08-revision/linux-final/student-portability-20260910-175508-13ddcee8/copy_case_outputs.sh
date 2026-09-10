#!/usr/bin/env bash
set -u

RUN_ID="student-portability-20260910-175508-13ddcee8"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
DEST="${EVIDENCE}/case-outputs"
mkdir -p "${DEST}/i2-good" "${DEST}/cap3-good"

cp -f "/root/learncpp-c08-builds/${RUN_ID}/student-validation/I2_hazard_pointer/good/logs/"* "${DEST}/i2-good/" 2>/dev/null || true
cp -f "/root/learncpp-c08-builds/${RUN_ID}/student-validation-cap3/Capstone3_parallel_compute/good/logs/"* "${DEST}/cap3-good/" 2>/dev/null || true

{
  echo "copied case outputs"
  find "${DEST}" -type f -print | sort
} > "${EVIDENCE}/case-output-copy.txt"
