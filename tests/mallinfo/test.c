// SPDX-License-Identifier: Apache-2.0
#include "../../libc/libc-mallinfo.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    struct bionic_mallinfo before=bionic_mallinfo();
    size_t size=8*1024*1024;
    void *memory=malloc(size); assert(memory); memset(memory,1,size);
    struct bionic_mallinfo during=bionic_mallinfo();
    assert(during.uordblks>=before.uordblks+size);
    assert(during.hblkhd>=during.uordblks);
    free(memory);
    struct bionic_mallinfo after=bionic_mallinfo();
    assert(after.uordblks+size<=during.uordblks);
    printf("PASS: real jemalloc usage before=%zu allocated=%zu freed=%zu\n",before.uordblks,during.uordblks,after.uordblks);
}
