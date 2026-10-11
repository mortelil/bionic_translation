// Scheduler policy/parameter queries and setters.
//
// AOSP bionic does not hand-write these functions: libc/SYSCALLS.TXT declares
//
//	sched_getparam(pid_t, sched_param*)			all
//	sched_getscheduler(pid_t)				all
//	sched_setparam(pid_t, const sched_param*)		all
//	sched_setscheduler(pid_t, int, const sched_param*)	all
//
// so the Android implementations are the generated raw-syscall stubs: the
// kernel performs all validation (negative pid -> EINVAL, unknown/dead tid ->
// ESRCH, NULL param -> EINVAL) and errno comes straight from the kernel, with
// no bionic-side remapping or policy filtering. A raw syscall is therefore
// exactly bionic-equivalent.
//
// The host libc must not be used for these: musl's wrappers are deliberate
// __syscall_ret(-ENOSYS) stubs and never issue the syscall, which is why ATL
// prints "sched_getscheduler(<tid>): Function not implemented" during thread
// dumps - libart.so resolves the symbols into the global scope (musl) and app
// native libs fall through the bionic namespace to the same host symbols.
//
// struct sched_param: bionic declares it as a single int (4 bytes) while the
// musl layout is much larger. The kernel only ever reads/writes
// sched_priority, so passing the caller's pointer through unchanged is
// correct for callers built against either layout (bionic callers pass their
// own 4-byte struct, musl callers a larger one).

#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

int sched_getscheduler(pid_t pid)
{
	return (int)syscall(SYS_sched_getscheduler, pid);
}

int sched_getparam(pid_t pid, struct sched_param *param)
{
	return (int)syscall(SYS_sched_getparam, pid, param);
}

int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
	return (int)syscall(SYS_sched_setscheduler, pid, policy, param);
}

int sched_setparam(pid_t pid, const struct sched_param *param)
{
	return (int)syscall(SYS_sched_setparam, pid, param);
}
