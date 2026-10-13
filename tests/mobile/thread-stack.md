# Thread stack defaults

The bridge initializes pthread attributes with Android's default stack size:
992 KiB on LP64 and 1008 KiB on 32-bit. Explicit stack sizes and caller-owned
stacks are passed unchanged to the host. This does not implement Android's
separate alternate signal-stack allocation.

Reference: AOSP Bionic tag `android-15.0.0_r1`,
`libc/bionic/pthread_internal.h` (`PTHREAD_STACK_SIZE_DEFAULT`) and
`libc/bionic/pthread_attr.cpp` (`pthread_attr_init`, `pthread_attr_setstacksize`).

Run `tests/mobile/run.sh` with `ATL_PREFIX` pointing to the candidate install,
or build `tests/mobile/test-thread-stack.c` against its `libpthread_bio`.
The test checks defaults, explicit smaller sizes, invalid-size rejection,
unchanged attributes after creation, and caller-owned memory. Thread stack
bounds are compared with a directly created host thread with identical
attributes because musl reserves TLS within caller-owned memory.

Integration validation: Alpine x86_64 stack test and existing thread-ID test
pass. ARM execution is not claimed.
