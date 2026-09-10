#!/usr/bin/env bash
set -euo pipefail

snapshot="/root/learncpp-c08-src/baseline-20260910-162842-fc6bb723"
evidence="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-preliminary/baseline-20260910-162842-fc6bb723"

cd "${snapshot}"
find . -type f -print0 | sort -z | xargs -0 sha256sum > "${evidence}/manifest.sha256"
wc -l "${evidence}/manifest.sha256" > "${evidence}/manifest-count.txt"
sha256sum "${evidence}/manifest.sha256" > "${evidence}/manifest-file.sha256"
du -sh "${snapshot}" > "${evidence}/snapshot-size.txt"

{
    find "${snapshot}" -type d \( -name '.*' -o -name 'build*' -o -name '_deps' \)
    find "${snapshot}" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pdb' -o -iname '*.obj' -o -iname '*.o' -o -iname '*.lib' -o -iname '*.a' -o -iname '*.zip' -o -iname '*.log' \)
} | sort > "${evidence}/excluded-leak-check.txt"

cat "${evidence}/manifest-count.txt"
cat "${evidence}/manifest-file.sha256"
cat "${evidence}/snapshot-size.txt"
if [[ -s "${evidence}/excluded-leak-check.txt" ]]; then
    echo "leak_check=found"
    cat "${evidence}/excluded-leak-check.txt"
else
    echo "leak_check=clean"
fi
