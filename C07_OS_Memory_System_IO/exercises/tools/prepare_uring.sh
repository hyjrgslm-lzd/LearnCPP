#!/usr/bin/env bash
set -euo pipefail
# Isolated existing-course dependency; never install into system directories.
base=${C07_LINUX_ROOT:-/root/learncpp-c07}
case "$base" in /root/learncpp-c07|/root/learncpp-c07-*) ;; *) echo 'C07 dependency prefix must be /root/learncpp-c07 or a task-suffixed sibling' >&2; exit 1;; esac
source_dir="$base/deps/liburing-2.15"
prefix="$base/deps/install-liburing-2.15"
expected=d41bf9220ec39277ff235379e9089d9e0fd6c2a5
mkdir -p "$base/deps"
if [ ! -d "$source_dir/.git" ]; then
    git -c credential.helper= clone --depth 1 --branch liburing-2.15 https://github.com/axboe/liburing.git "$source_dir"
fi
actual=$(git -C "$source_dir" rev-parse HEAD)
[ "$actual" = "$expected" ] || { echo 'liburing source fingerprint mismatch' >&2; exit 1; }
[ -z "$(git -C "$source_dir" status --porcelain --untracked-files=no)" ] || { echo 'liburing tracked source modified' >&2; exit 1; }
cd "$source_dir"
./configure --prefix="$prefix"
make -C src -j4
make -C src install
printf '%s\n' "$actual" > "$prefix/c07-source-commit.txt"
cat "$prefix/include/liburing/io_uring_version.h"
printf 'C07_LIBURING_COMMIT=%s\nC07_LIBURING_PREFIX=%s\n' "$actual" "$prefix"
