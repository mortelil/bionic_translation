# Android mallinfo on musl

The experimental adapter requires jemalloc to be preloaded when the process
starts. Merely dlopening it later is rejected: querying an unused allocator
would produce misleading numbers. This is opt-in, not a replacement of the
container's or host's default allocator.

The Android ABI uses ten size_t fields. Supported statistics are mapped bytes
(hblkhd), allocated bytes (uordblks) and unallocated capacity inside active
allocation pages (fordblks). The latter excludes retained/dirty extents and is
not a promise that all those bytes can satisfy one allocation. Thread caches
and concurrent allocations make these approximate snapshots. Other legacy
counters are not provided and remain zero; this is a partial allocator-specific
implementation, not glibc heap/chunk compatibility. A backend without available
statistics fails explicitly. No host RSS or guessed allocation totals are used.

Install `jemalloc` inside the development container and run
`sh tests/mallinfo/run.sh`. The test checks a real 8 MiB malloc/free delta and
explicit failure without the required process allocator. `ATL_PREFIX` and
`ATL_JEMALLOC_LIBRARY` select the runtime and preload library.

References: [jemalloc statistics](https://jemalloc.net/jemalloc.3.html),
[Android ABI](https://android.googlesource.com/platform/bionic/+/9af13d2/libc/include/malloc.h).
Authored with AI assistance; no allocator implementation was copied.
