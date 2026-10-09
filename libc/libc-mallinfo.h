// SPDX-License-Identifier: MIT AND Apache-2.0
#pragma once
#include <stddef.h>
// Android uses size_t fields even for the original mallinfo ABI.
struct bionic_mallinfo {
    size_t arena, ordblks, smblks, hblks, hblkhd;
    size_t usmblks, fsmblks, uordblks, fordblks, keepcost;
};
struct bionic_mallinfo bionic_mallinfo(void);
