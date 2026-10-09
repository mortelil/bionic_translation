# Cross-DSO CFI tests

Run `sh tests/cfi/run.sh` inside Alpine after building and installing the private
Bionic runtime. Requires a C compiler, Clang and LLD. `ATL_PREFIX` selects the
runtime under test. Logs include expected abort/trap messages from child tests.

Tests load independent libraries through both the Android and host loaders.
They check type/target/diagnostic dispatch, invalid pointers, wrong type IDs,
known non-instrumented modules and repeated Android unload/reload. A second
pair of libraries is instrumented by Clang itself: a correct cross-library
function signature succeeds and a mismatched signature traps. The compiler
fixture uses no standard library and no sanitizer ignorelist.

The implementation resolves the target library on each check. It does not
implement LLVM's shadow-memory acceleration. Android loader locking and host
RTLD_NOLOAD references keep modules alive during checks. Host executable targets
that cannot be pinned as a DSO currently fail closed; this is a compatibility
limitation. Concurrent unload stress, ARM64 and other host C runtimes need
additional validation. Passing this suite is not a security audit.

ABI reference: https://clang.llvm.org/docs/ControlFlowIntegrityDesign.html#shared-library-support
Implementation and test fixtures were authored with AI assistance; no LLVM or
AOSP implementation was copied.
