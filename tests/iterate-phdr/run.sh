#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
prefix=${ATL_PREFIX:-$(dirname "$repo")/.atl-runtime}
out="$repo/mobile-test-output/iterate-phdr"
mkdir -p "$out"
export LD_LIBRARY_PATH="$prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cc -shared -fPIC -nostdlib "$repo/tests/iterate-phdr/module.c" -o "$out/valid.so"
cc -shared -fPIC -nostdlib -DBROKEN "$repo/tests/iterate-phdr/module.c" -o "$out/broken.so"
cc "$repo/tests/iterate-phdr/test.c" "$repo/main_executable/bionic_compat.c" -L "$prefix/lib" -Wl,--no-as-needed -ldl_bio -lc_bio -lpthread_bio -pthread -o "$out/test"
BIONIC_LD_LIBRARY_PATH="$out" timeout 30 "$out/test" "$out/valid.so" "$out/broken.so"
