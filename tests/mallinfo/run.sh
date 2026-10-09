#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
prefix=${ATL_PREFIX:-$(dirname "$repo")/.atl-runtime}
out="$repo/mobile-test-output/mallinfo"
mkdir -p "$out"
export LD_LIBRARY_PATH="$prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cc "$repo/tests/mallinfo/test.c" -L "$prefix/lib" -lc_bio -o "$out/test"
LD_PRELOAD=${ATL_JEMALLOC_LIBRARY:-/usr/lib/libjemalloc.so.2} timeout 30 "$out/test"
status=0
(ulimit -c 0; LD_PRELOAD= "$out/test") > "$out/unsupported.log" 2>&1 || status=$?
test "$status" -eq 134
grep -q 'mallinfo requires jemalloc' "$out/unsupported.log"
echo 'PASS: unavailable statistics abort explicitly instead of fabricating heap usage'
