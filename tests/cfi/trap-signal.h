// SPDX-License-Identifier: Apache-2.0
#include <signal.h>

/* GCC/Clang emit BRK for a trap on AArch64 and UD2 on x86. Keep
 * compiler traps distinct from the loader's deliberate SIGABRT failures. */
#if defined(__aarch64__)
#define CFI_TRAP_SIGNAL SIGTRAP
#else
#define CFI_TRAP_SIGNAL SIGILL
#endif
