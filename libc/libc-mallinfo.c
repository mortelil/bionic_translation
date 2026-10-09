// SPDX-License-Identifier: MIT AND Apache-2.0
#include "libc-mallinfo.h"
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct bionic_mallinfo bionic_mallinfo(void)
{
    int (*control)(const char *, void *, size_t *, void *, size_t) = dlsym(RTLD_DEFAULT, "mallctl");
    void *allocate = dlsym(RTLD_DEFAULT, "malloc");
    Dl_info allocator, statistics;
    // Merely loading jemalloc is insufficient: it must own the malloc domain.
    if (!control || !dladdr(allocate, &allocator) || !dladdr((void *)control, &statistics) ||
        allocator.dli_fbase != statistics.dli_fbase) {
        fprintf(stderr, "mallinfo requires jemalloc as the process allocator (LD_PRELOAD); musl has no equivalent statistics API\n");
        abort();
    }
    uint64_t epoch = 1;
    size_t length = sizeof(epoch), allocated, active, mapped;
    if (control("epoch", &epoch, &length, &epoch, sizeof(epoch))) goto unavailable;
    length = sizeof(size_t);
    if (control("stats.allocated", &allocated, &length, NULL, 0)) goto unavailable;
    length = sizeof(size_t);
    if (control("stats.active", &active, &length, NULL, 0)) goto unavailable;
    length = sizeof(size_t);
    if (control("stats.mapped", &mapped, &length, NULL, 0)) goto unavailable;
    struct bionic_mallinfo info = {0};
    info.hblkhd = mapped;
    info.uordblks = allocated;
    // Unallocated capacity in active allocation pages; excludes retained and
    // dirty extents. Thread caching makes these counters approximate snapshots.
    info.fordblks = active > allocated ? active - allocated : 0;
    return info;
unavailable:
    fprintf(stderr, "mallinfo: the active jemalloc backend does not provide statistics\n");
    abort();
}
