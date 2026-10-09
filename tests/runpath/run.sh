#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
prefix=${ATL_PREFIX:-$(dirname "$repo")/.atl-runtime}
out="$repo/mobile-test-output/runpath"
mkdir -p "$out/deps" "$out/priority"
export LD_LIBRARY_PATH="$prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cc -shared -fPIC -nostdlib -Wl,-soname,libatl_runpath_child.so "$repo/tests/runpath/child.c" -o "$out/deps/libatl_runpath_child.so"
cc -shared -fPIC -nostdlib -DVALUE=73 -Wl,-soname,libatl_runpath_child.so "$repo/tests/runpath/child.c" -o "$out/priority/libatl_runpath_child.so"
cc -shared -fPIC -nostdlib "$repo/tests/runpath/parent.c" -L "$out/deps" -latl_runpath_child -Wl,-rpath,'$ORIGIN/absent:$ORIGIN/deps' -o "$out/parent.so"
cc -shared -fPIC -nostdlib "$repo/tests/runpath/parent.c" -L "$out/deps" -latl_runpath_child -Wl,-rpath,'${ORIGIN}/deps' -o "$out/braced.so"
cc "$repo/tests/runpath/test.c" "$repo/main_executable/bionic_compat.c" -L "$prefix/lib" -Wl,--no-as-needed -ldl_bio -lc_bio -lpthread_bio -o "$out/test"
BIONIC_LD_LIBRARY_PATH="$out" timeout 30 "$out/test" "$out/parent.so" 42
BIONIC_LD_LIBRARY_PATH="$out" timeout 30 "$out/test" "$out/braced.so" 42
BIONIC_LD_LIBRARY_PATH="$out:$out/priority" timeout 30 "$out/test" "$out/parent.so" 73
