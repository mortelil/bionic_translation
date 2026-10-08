#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
prefix=${ATL_PREFIX:-$(dirname "$repo")/.atl-runtime}
out="$repo/mobile-test-output"
mkdir -p "$out"
export LD_LIBRARY_PATH="$prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cc "$repo/tests/mobile/test-bionic.c" -L "$prefix/lib" -lc_bio -o "$out/test-bionic"
cc "$repo/tests/mobile/test-ffi.c" -L "$prefix/lib" -Wl,--no-as-needed -lc_bio -ldl_bio -lpthread_bio -o "$out/test-ffi"
timeout 30 "$out/test-bionic"
timeout 30 "$out/test-ffi"
