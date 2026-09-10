#!/usr/bin/env bash
set -u

RUN_ID="final-20260910-173010-d5356576"
BUILD="/root/learncpp-c08-builds/${RUN_ID}/frontier-clang18"
EVIDENCE="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-final/${RUN_ID}"
OUT="${EVIDENCE}/frontier-clang18-explicit-target-build.txt"
EXTRA="${EVIDENCE}/commands-extra.jsonl"

: > "${EXTRA}"
{
  echo "name=frontier-clang18-explicit-target-build"
  echo "start=$(date --iso-8601=seconds)"
  echo "command=cmake --build ${BUILD} --parallel 4 --target F01_thread_attributes F02_hazard_pointer_batches F01_thread_attributes_reference F02_hazard_pointer_batches_reference F03_std_senders_reference F03_std_hazard_pointer_reference F03_std_rcu_reference"
  echo
} > "${OUT}"

timeout 600 cmake --build "${BUILD}" --parallel 4 --target \
  F01_thread_attributes \
  F02_hazard_pointer_batches \
  F01_thread_attributes_reference \
  F02_hazard_pointer_batches_reference \
  F03_std_senders_reference \
  F03_std_hazard_pointer_reference \
  F03_std_rcu_reference >> "${OUT}" 2>&1
rc=$?

{
  echo
  echo "exit_code=${rc}"
  echo "end=$(date --iso-8601=seconds)"
} >> "${OUT}"

printf '{"name":"frontier-clang18-explicit-target-build","exit_code":%s,"output":"frontier-clang18-explicit-target-build.txt"}\n' "${rc}" >> "${EXTRA}"
exit "${rc}"
