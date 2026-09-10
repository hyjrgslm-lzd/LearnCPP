#!/usr/bin/env bash
set -u

work_dir="/root/learncpp-c08-builds/tsan-diagnosis-20260910-1658"
repo_report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/agent-run-20260910-1658"
exe="/root/learncpp-c08-builds/baseline-20260910-162842-fc6bb723/linux-tsan/F3_seqcst_fence/F3_seqcst_fence_reference"
ctest_dir="/root/learncpp-c08-builds/baseline-20260910-162842-fc6bb723/linux-tsan"

cd "$work_dir" || exit 2
rm -f rerun2-f3-*.txt status-rerun2-f3-*.txt ctest-rerun2-f3.txt status-ctest-rerun2-f3.txt rerun2-f3-status-summary.txt

for n in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20; do
    /usr/bin/timeout 15 "$exe" > "rerun2-f3-$n.txt" 2>&1
    printf '%s\n' "$?" > "status-rerun2-f3-$n.txt"
done

/usr/bin/timeout 60 ctest --test-dir "$ctest_dir" -R '^F3_seqcst_fence_reference$' --output-on-failure > ctest-rerun2-f3.txt 2>&1
printf '%s\n' "$?" > status-ctest-rerun2-f3.txt

for f in status-rerun2-f3-*.txt status-ctest-rerun2-f3.txt; do
    printf '%s=' "$f"
    cat "$f"
done | sort > rerun2-f3-status-summary.txt

cp rerun2-f3-*.txt status-rerun2-f3-*.txt ctest-rerun2-f3.txt status-ctest-rerun2-f3.txt rerun2-f3-status-summary.txt "$repo_report"/
cat rerun2-f3-status-summary.txt
