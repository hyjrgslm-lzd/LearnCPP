#!/usr/bin/env bash
set -euo pipefail

build_root="/root/learncpp-c08-builds/baseline-20260910-162842-fc6bb723"
evidence="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-preliminary/baseline-20260910-162842-fc6bb723"

ctest --test-dir "${build_root}/linux-core" --show-only=json-v1 > "${evidence}/linux-core-ctest-list.json"
ctest --test-dir "${build_root}/linux-debug" --show-only=json-v1 > "${evidence}/linux-debug-ctest-list.json"
ctest --test-dir "${build_root}/linux-asan" --show-only=json-v1 > "${evidence}/linux-asan-ctest-list.json"
ctest --test-dir "${build_root}/linux-tsan" --show-only=json-v1 > "${evidence}/linux-tsan-ctest-list.json"

ls -l "${evidence}"/*-ctest-list.json
