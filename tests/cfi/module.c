// SPDX-License-Identifier: Apache-2.0
#include <stdint.h>
__attribute__((visibility("default"))) int atl_cfi_target(void) { return 42; }
#ifndef UNCHECKED
__attribute__((visibility("default"))) void __cfi_check(uint64_t type, void *target, void *diag) {
    if (type != UINT64_C(0x123456789abcdef0) || target != (void *)atl_cfi_target)
        __builtin_trap();
    if (diag) *(int *)diag = 73;
}
#endif
