#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OPEN_NEEDS_MODE(flags) (((flags) & O_CREAT) || (((flags) & O_TMPFILE) == O_TMPFILE))

/* nop */
static bool apply_path_overrides(char **path) {
	return false;
}

typedef bool (apply_path_overrides_func_type)(char **);
static apply_path_overrides_func_type *apply_path_overrides_func = apply_path_overrides;

/* call this to set an overrides function */
void libc_bio_set_apply_path_overrides_func(apply_path_overrides_func_type *func)
{
	apply_path_overrides_func = func;
}

/* open and fopen overrides */
int bionic_open(char *path, int oflag, ...)
{
	int fd;
	int mode;

	bool free_path = (*apply_path_overrides_func)(&path);

	// Hide TracerPid from /proc/self/status for hideous apps that check for debugger.
	// Note, since /proc/self/status doesn't get updated anymore, this may break some stuff.
	// XXX: Turn this ON/OFF with env var maybe?
	if (!strcmp(path, "/proc/self/status")) {
		static FILE *faked = NULL;

		if (!faked) {
			static char status[4096];

			{
				FILE *f = fopen(path, "rb");
				assert(f && "/proc/self/status failed to open :/");
				const size_t ret = fread(status, 1, sizeof(status), f);
				assert(ret <= sizeof(status) && "/proc/self/status doesn't fit in 4096 bytes :/");
				fclose(f);
			}

			for (char *s, *e; (s = strstr(status, "TracerPid:\t"));) {
				for (e = s; (size_t)(e - status) < sizeof(status) && *e && *e != '\n'; ++e)
					;
				memmove(s, e, sizeof(status) - (e - status));
				break;
			}

			faked = fmemopen(status, sizeof(status), "rb");
			assert(faked && "fmemopen failed :/");
		}

		return fileno(faked);
	}

	if(OPEN_NEEDS_MODE(oflag)) {
		va_list arg;
		va_start(arg, oflag);
		mode = va_arg(arg, int);
		va_end(arg);

		fd = open(path, oflag, mode);
	} else {
		fd = open(path, oflag);
	}

	if(free_path)
		free(path);

	return fd;
}

/* bionic's openat() family. Android native libraries reference openat() and
 * openat64() (Firefox's libcrashhelper.so/libcrashtools.so call openat64 for
 * every crash-file operation), and they go through the same path overrides as
 * open(): an app that hardcodes an Android path has to be redirected no matter
 * which variant it used. */
static int openat_common(int dirfd, const char *pathname, int oflag, va_list arg)
{
	int fd;
	int mode;
	char *path = (char *)pathname;

	bool free_path = (*apply_path_overrides_func)(&path);

	if (OPEN_NEEDS_MODE(oflag)) {
		mode = va_arg(arg, int);
		fd = openat(dirfd, path, oflag, mode);
	} else {
		fd = openat(dirfd, path, oflag);
	}

	int saved_errno = errno;
	if (free_path)
		free(path);
	errno = saved_errno;

	return fd;
}

int bionic_openat(int dirfd, const char *pathname, int oflag, ...)
{
	va_list arg;
	int fd;

	va_start(arg, oflag);
	fd = openat_common(dirfd, pathname, oflag, arg);
	va_end(arg);

	return fd;
}

int bionic_openat64(int dirfd, const char *pathname, int oflag, ...)
{
	/* off_t is 64-bit on the LP64 targets ATL builds for, bionic's openat64 is openat. */
	va_list arg;
	int fd;

	va_start(arg, oflag);
	fd = openat_common(dirfd, pathname, oflag, arg);
	va_end(arg);

	return fd;
}

FILE *bionic_fopen(char *path, const char *restrict mode)
{
	FILE *file;
	bool free_path = (*apply_path_overrides_func)(&path);

	file = fopen(path, mode);
	if(free_path)
		free(path);
	return file;
}
