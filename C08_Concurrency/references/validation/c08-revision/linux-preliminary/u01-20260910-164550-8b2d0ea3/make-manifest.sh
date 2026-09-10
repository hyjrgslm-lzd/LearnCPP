#!/usr/bin/env bash
set -euo pipefail

snapshot="/root/learncpp-c08-src/u01-20260910-164550-8b2d0ea3"
evidence="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-preliminary/u01-20260910-164550-8b2d0ea3"

cd "${snapshot}"
find . -type f -print0 | sort -z | xargs -0 sha256sum > "${evidence}/manifest.sha256"
wc -l "${evidence}/manifest.sha256" > "${evidence}/manifest-count.txt"
sha256sum "${evidence}/manifest.sha256" > "${evidence}/manifest-file.sha256"
du -sh "${snapshot}" > "${evidence}/snapshot-size.txt"

{
    find "${snapshot}" -type d \( -name '.*' -o -name 'build*' -o -name '_deps' \)
    find "${snapshot}" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pdb' -o -iname '*.obj' -o -iname '*.o' -o -iname '*.lib' -o -iname '*.a' -o -iname '*.zip' -o -iname '*.log' \)
} | sort > "${evidence}/excluded-leak-check.txt"

{
    echo "fmt_head=$(git -C /root/learncpp-c08-deps/fmt-12.1.0 rev-parse HEAD)"
    git -C /root/learncpp-c08-deps/fmt-12.1.0 diff-index --quiet HEAD --
    echo "fmt_clean=1"
    echo "spdlog_head=$(git -C /root/learncpp-c08-deps/spdlog-1.17.0 rev-parse HEAD)"
    git -C /root/learncpp-c08-deps/spdlog-1.17.0 diff-index --quiet HEAD --
    echo "spdlog_clean=1"
} > "${evidence}/deps-state.txt"

cat "${evidence}/manifest-count.txt"
cat "${evidence}/manifest-file.sha256"
cat "${evidence}/snapshot-size.txt"
cat "${evidence}/deps-state.txt"
if [[ -s "${evidence}/excluded-leak-check.txt" ]]; then
    echo "leak_check=found"
    cat "${evidence}/excluded-leak-check.txt"
else
    echo "leak_check=clean"
fi
