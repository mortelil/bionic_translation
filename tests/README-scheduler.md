# Scheduler translation regression

The wrappers use Linux syscalls, as generated AOSP Bionic stubs do. The kernel
validates policy, priority, permissions and pid; host musl's ENOSYS wrappers
must not intercept these symbols. The kernel reads/writes only sched_priority,
including when Android passes its four-byte sched_param.

Run in the existing Alpine Toolbox (P is an installed private build prefix):

```
cc tests/scheduler.c -o /tmp/scheduler -L"$P/lib" -Wl,-rpath,"$P/lib" -l:libc_bio.so.0
LD_LIBRARY_PATH="$P/lib" /tmp/scheduler
```

2026-10-11 x86_64: 18/18 checks pass; the same source linked to host musl
rejects 18/18 with ENOSYS. A four-byte-layout canary checks for overwrite.
The test only sets its own thread to the ordinary scheduler with priority zero;
it does not request real-time privileges. No ARM device or live ART thread-dump
verification is claimed. Original implementation/probe supplied by OMP;
integration adds the ABI canary and reproducible test instructions.
