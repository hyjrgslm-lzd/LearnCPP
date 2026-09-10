#!/usr/bin/env bash
set -u

RUN_ID="student-portability-20260910-175508-13ddcee8"
BUILD="/root/learncpp-c08-builds/${RUN_ID}/student-validation-cap3/Capstone3_parallel_compute/good/build"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
OUT="${EVIDENCE}/cap3-build-inspection.txt"

{
  echo "build=${BUILD}"
  echo
  echo "files"
  find "${BUILD}" -maxdepth 5 -type f | sed "s#^${BUILD}/##" | sort | head -200
  echo
  echo "depfiles"
  find "${BUILD}" -type f \( -name "*.d" -o -name "*.obj.d" -o -name "*.tlog" \) -print | sort
  echo
  echo "compile_commands"
  if [[ -f "${BUILD}/compile_commands.json" ]]; then
    cat "${BUILD}/compile_commands.json"
  else
    echo "missing"
  fi
} > "${OUT}" 2>&1
