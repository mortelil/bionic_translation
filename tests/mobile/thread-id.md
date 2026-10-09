# Linux thread ID lookup

`pthread_gettid_np` uses the host `pthread_getcpuclockid` API and decodes the
Linux per-thread CPU clock ID. This avoids libc-private thread structure offsets
and handles threads created outside the Android wrapper, including ART threads.

The encoding is defined by Linux `make_thread_cpuclock` in
https://github.com/torvalds/linux/blob/master/include/linux/posix-timers.h
and is used by musl `pthread_getcpuclockid`:
https://git.musl-libc.org/cgit/musl/tree/src/thread/pthread_getcpuclockid.c

Run `sh tests/mobile/run.sh` inside the Alpine development container. The test
compares against the `gettid` syscall for the main thread and twenty live worker
threads alternating host and wrapper creation. Barriers keep the target threads
alive until the other thread has checked their IDs. Null returns -1. Arbitrary
invalid handles and handles after join/detach lifetime are not supported; callers
must obey the host pthread lifetime rules. ARM64 and glibc remain to be tested.

No libc implementation source was copied. AI-assisted implementation and tests.
