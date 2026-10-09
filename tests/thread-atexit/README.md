# C++ thread-exit callbacks

Run `sh tests/thread-atexit/run.sh` inside Alpine after building and installing
the private Bionic runtime. `ATL_PREFIX` selects the installation.

The adapter uses the host GCC C++ runtime (`libstdc++.so.6`) for worker/main
thread exit notification. Android callback registrations retain their object
and destructor DSOs until execution finishes. A per-thread pending list preserves
Android destructor order when a callback registers another callback, even when
the host drains callbacks in batches. Host callback tokens remain alive until
the host consumes them, avoiding dangling callback data.

Tests cover LIFO order, registration during teardown, an explicit dlclose before
thread exit, and main-thread callbacks at process exit. Cancellation, recursive
process exit, mixed native/Android destructor interleaving, memory exhaustion
and ARM64 are not validated here. This is not a complete C++ runtime conformance
claim. Implementation and fixtures were authored with AI assistance.
