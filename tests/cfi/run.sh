#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
prefix=${ATL_PREFIX:-$(dirname "$repo")/.atl-runtime}
out="$repo/mobile-test-output/cfi"
mkdir -p "$out"
export LD_LIBRARY_PATH="$prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cc -shared -fPIC -nostdlib -Wl,-Bsymbolic "$repo/tests/cfi/module.c" -o "$out/checked.so"
cc -shared -fPIC -nostdlib -DUNCHECKED "$repo/tests/cfi/module.c" -o "$out/unchecked.so"
cc "$repo/tests/cfi/test.c" "$repo/main_executable/bionic_compat.c" -L "$prefix/lib" -Wl,--no-as-needed -ldl_bio -lc_bio -lpthread_bio -o "$out/test-cfi"
BIONIC_LD_LIBRARY_PATH="$out" timeout 30 "$out/test-cfi" "$out/checked.so" "$out/unchecked.so"
cc -shared -fPIC -nostdlib -Wl,-Bsymbolic -Wl,--hash-style=sysv "$repo/tests/cfi/module.c" -o "$out/checked-sysv.so"
cc -shared -fPIC -nostdlib -DUNCHECKED -Wl,--hash-style=sysv "$repo/tests/cfi/module.c" -o "$out/unchecked-sysv.so"
BIONIC_LD_LIBRARY_PATH="$out" timeout 30 "$out/test-cfi" "$out/checked-sysv.so" "$out/unchecked-sysv.so"
clang -shared -fPIC -nostdlib -flto -fuse-ld=lld -fvisibility=hidden -fno-sanitize-ignorelist -fsanitize=cfi-icall -fsanitize-cfi-cross-dso "$repo/tests/cfi/instrumented.c" -o "$out/callee.so"
clang -shared -fPIC -nostdlib -flto -fuse-ld=lld -fvisibility=hidden -fno-sanitize-ignorelist -fsanitize=cfi-icall -fsanitize-cfi-cross-dso -DCALLER "$repo/tests/cfi/instrumented.c" -o "$out/caller.so"
cc "$repo/tests/cfi/compiler-test.c" "$repo/main_executable/bionic_compat.c" -L "$prefix/lib" -Wl,--no-as-needed -ldl_bio -lc_bio -lpthread_bio -o "$out/compiler-test"
BIONIC_LD_LIBRARY_PATH="$out" timeout 30 "$out/compiler-test" "$out/callee.so" "$out/caller.so"
