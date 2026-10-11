// H-SCHED frozen self-check probe (ATL bionic scheduler translation).
//
// Exercises the four scheduler functions through whatever implementation the
// dynamic linker gives them (private-prefix libc_bio.so.0 for the GREEN run,
// host musl libc for the RED baseline run) and compares return value + errno
// with the expected Linux/bionic behaviour.
//
// Expected values come from AOSP bionic semantics (raw generated syscall
// stubs, kernel-side validation, no bionic remapping) and were measured on
// this host kernel.
//
// Build GREEN: cc probe.c -o probe-green -L$P/lib -Wl,-rpath,$P/lib -l:libc_bio.so.0
// Build RED  : cc probe.c -o probe-red            (host musl only)
// Run        : LD_LIBRARY_PATH=$P/lib ./probe-green

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static pid_t self_tid(void)
{
	return (pid_t)syscall(SYS_gettid);
}

static pid_t worker_tid;
static void *worker(void *arg)
{
	(void)arg;
	worker_tid = self_tid();
	return NULL;
}

static int cases;
static int failures;

static void report(const char *name, long ret, int err,
                   long exp_ret, int exp_err, const char *extra, int extra_ok)
{
	int ok = (ret == exp_ret) && (err == exp_err) && extra_ok;
	cases++;
	if (!ok) failures++;
	printf("CASE %-42s ret=%-3ld errno=%-3d (%-12s) expect_ret=%-3ld expect_errno=%-3d (%s)%s%s %s\n",
	       name, ret, err, strerror(err), exp_ret, exp_err, strerror(exp_err),
	       extra ? " extra=" : "", extra ? extra : "", ok ? "VERDICT=PASS" : "VERDICT=FAIL");
	fflush(stdout);
}

static void impl_of(const char *sym, void *addr)
{
	Dl_info info;
	if (dladdr(addr, &info) && info.dli_fname)
		printf("IMPL %-22s -> %s\n", sym, info.dli_fname);
	else
		printf("IMPL %-22s -> <unresolved>\n", sym);
	fflush(stdout);
}

int main(void)
{
	pid_t tid = self_tid();
	pid_t dead;
	pthread_t th;
	struct sched_param sp;
	struct sched_param p0;
	struct sched_param p7;
	long r;
	int e;

	printf("HSCHED_BEGIN pid=%d tid=%d\n", (int)getpid(), (int)tid);
	fflush(stdout);

	impl_of("sched_getscheduler", (void *)sched_getscheduler);
	impl_of("sched_getparam", (void *)sched_getparam);
	impl_of("sched_setscheduler", (void *)sched_setscheduler);
	impl_of("sched_setparam", (void *)sched_setparam);

	memset(&sp, 0x5a, sizeof(sp));
	memset(&p0, 0, sizeof(p0));
	memset(&p7, 0, sizeof(p7));
	p7.sched_priority = 7;

	/* getter: valid TID (self) */
	errno = 0;
	r = sched_getscheduler(tid);
	e = errno;
	report("sched_getscheduler(self_tid)", r, e, 0, 0, NULL, 1);

	/* getter: pid 0 == calling thread */
	errno = 0;
	r = sched_getscheduler(0);
	e = errno;
	report("sched_getscheduler(0)", r, e, 0, 0, NULL, 1);

	/* getter: negative TID */
	errno = 0;
	r = sched_getscheduler(-1);
	e = errno;
	report("sched_getscheduler(-1)", r, e, -1, EINVAL, NULL, 1);

	/* getter: nonexistent TID */
	errno = 0;
	r = sched_getscheduler(999999);
	e = errno;
	report("sched_getscheduler(999999)", r, e, -1, ESRCH, NULL, 1);

	/* getter: dead thread TID */
	if (pthread_create(&th, NULL, worker, NULL) != 0) return 2;
	if (pthread_join(th, NULL) != 0) return 2;
	dead = worker_tid;
	printf("INFO dead_worker_tid=%d\n", (int)dead);
	fflush(stdout);
	errno = 0;
	r = sched_getscheduler(dead);
	e = errno;
	report("sched_getscheduler(dead_tid)", r, e, -1, ESRCH, NULL, 1);

	/* getparam: valid TID, priority must be written */
	sp.sched_priority = -12345;
	errno = 0;
	r = sched_getparam(tid, &sp);
	e = errno;
	{
		char extra[64];
		snprintf(extra, sizeof(extra), "sched_priority=%d", sp.sched_priority);
		report("sched_getparam(self_tid,&sp)", r, e, 0, 0, extra, sp.sched_priority == 0);
	}

	/* getparam: pid 0 */
	sp.sched_priority = -12345;
	errno = 0;
	r = sched_getparam(0, &sp);
	e = errno;
	{
		char extra[64];
		snprintf(extra, sizeof(extra), "sched_priority=%d", sp.sched_priority);
		report("sched_getparam(0,&sp)", r, e, 0, 0, extra, sp.sched_priority == 0);
	}

	/* getparam: negative TID */
	errno = 0;
	r = sched_getparam(-1, &sp);
	e = errno;
	report("sched_getparam(-1,&sp)", r, e, -1, EINVAL, NULL, 1);

	/* getparam: nonexistent TID */
	errno = 0;
	r = sched_getparam(999999, &sp);
	e = errno;
	report("sched_getparam(999999,&sp)", r, e, -1, ESRCH, NULL, 1);

	/* getparam: dead thread TID */
	errno = 0;
	r = sched_getparam(dead, &sp);
	e = errno;
	report("sched_getparam(dead_tid,&sp)", r, e, -1, ESRCH, NULL, 1);

	/* getparam: NULL param (kernel checks !param before pid) */
	errno = 0;
	r = sched_getparam(tid, NULL);
	e = errno;
	report("sched_getparam(self_tid,NULL)", r, e, -1, EINVAL, NULL, 1);

	/* setscheduler: SCHED_OTHER, priority 0 */
	errno = 0;
	r = sched_setscheduler(tid, SCHED_OTHER, &p0);
	e = errno;
	report("sched_setscheduler(self,SCHED_OTHER,{0})", r, e, 0, 0, NULL, 1);

	/* setscheduler: negative TID */
	errno = 0;
	r = sched_setscheduler(-1, SCHED_OTHER, &p0);
	e = errno;
	report("sched_setscheduler(-1,SCHED_OTHER,{0})", r, e, -1, EINVAL, NULL, 1);

	/* setscheduler: SCHED_FIFO with invalid priority 0 */
	errno = 0;
	r = sched_setscheduler(tid, SCHED_FIFO, &p0);
	e = errno;
	report("sched_setscheduler(self,SCHED_FIFO,{0})", r, e, -1, EINVAL, NULL, 1);

	/* setparam: invalid priority 7 for SCHED_OTHER */
	errno = 0;
	r = sched_setparam(tid, &p7);
	e = errno;
	report("sched_setparam(self,{7})", r, e, -1, EINVAL, NULL, 1);

	/* setparam: NULL param */
	errno = 0;
	r = sched_setparam(tid, NULL);
	e = errno;
	report("sched_setparam(self,NULL)", r, e, -1, EINVAL, NULL, 1);

	/* setparam: nonexistent TID */
	errno = 0;
	r = sched_setparam(999999, &p0);
	e = errno;
	report("sched_setparam(999999,{0})", r, e, -1, ESRCH, NULL, 1);

	/* Android's sched_param contains one int. Guard the following bytes so a
	 * host-layout write cannot silently corrupt a bionic caller's stack. */
	struct { int priority; unsigned guard; } android_param = {-1, 0xa5a55a5a};
	errno = 0;
	r = sched_getparam(tid, (struct sched_param *)&android_param);
	report("Android 4-byte sched_param", r, errno, 0, 0, NULL,
	       android_param.priority == 0 && android_param.guard == 0xa5a55a5a);

	printf("HSCHED_SUMMARY cases=%d pass=%d fail=%d\n", cases, cases - failures, failures);
	fflush(stdout);
	printf("HSCHED_END\n");
	return failures == 0 ? 0 : 1;
}
