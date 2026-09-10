#!/usr/bin/env bash
set -u

RUN_ID="students-r2-20260910-180658-457446af"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
BUILD="/root/learncpp-c08-builds/${RUN_ID}/student-validation"
DEST="${EVIDENCE}/case-outputs"
mkdir -p "${DEST}"

find "${BUILD}" -path "*/logs/*" -type f | while read -r file; do
  rel="${file#${BUILD}/}"
  target="${DEST}/${rel}"
  mkdir -p "$(dirname "${target}")"
  cp -f "${file}" "${target}"
done

find "${DEST}" -type f -name "*.log" | while read -r file; do
  mv "${file}" "${file}.txt"
done

find "${DEST}" -type f -print | sort > "${EVIDENCE}/case-output-files.txt"
