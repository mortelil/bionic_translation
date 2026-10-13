#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// this seems to not be a stable ABI, so hopefully nobody treats it as such
// FIXME: move this out into it's own file whenever we get to implementing it more properly
struct prop_info {
	const char *name;
};

#define PROP_NAME_MAX  32
#define PROP_VALUE_MAX 92
extern int __system_property_get(const char *, char *);
static const struct prop_info sdk_property = {"ro.build.version.sdk"};
static const struct prop_info fingerprint_property = {"ro.build.fingerprint"};

const struct prop_info *bionic___system_property_find(const char *name)
{
	if (!strcmp(name, sdk_property.name)) return &sdk_property;
	if (!strcmp(name, fingerprint_property.name)) return &fingerprint_property;
	return NULL;
}

int bionic___system_property_read(const struct prop_info *prop_info, char *name, char *value)
{
	if (name)
		strncpy(name, prop_info->name, PROP_NAME_MAX);

	if (!value)
		return 0;

	if (!strcmp(prop_info->name, sdk_property.name)) {
		return __system_property_get(prop_info->name, value);
	} else if (!strcmp(prop_info->name, "ro.build.fingerprint")) {
		strncpy(value, "", PROP_VALUE_MAX); // there is no good reason that apps should need this, so just return an empty string
	} else {
		printf("__system_property_find: >%s< not handled yet\n", prop_info->name);
		strncpy(value, "", PROP_VALUE_MAX);
	}

	return strlen(value);
}

extern int bionic_read_property_snapshot(const char *, char *, uint32_t *);

void bionic___system_property_read_callback(const struct prop_info *property,
        void (*callback)(void *, const char *, const char *, uint32_t), void *cookie)
{
	char value[PROP_VALUE_MAX];
	uint32_t serial;
	bionic_read_property_snapshot(property->name, value, &serial);
	// No lock is held across user code: callbacks may reenter the property API.
	callback(cookie, property->name, value, serial);
}

// actually misc stuff

void android_set_abort_message(const char *msg)
{
	printf("android_set_abort_message called: '%s'\n", msg);
	//	exit(1);
}

// setlocale is a bit special on bionic, try to mimic the behavior
char *bionic_setlocale(int category, const char *locale)
{
	if(!locale)
		return setlocale(category, NULL);

	// list of allowed locales from bionic
	if (!strcmp(locale, "") ||
	    !strcmp(locale, "C.UTF-8") ||
	    !strcmp(locale, "en_US.UTF-8")) {
		return setlocale(category, "C.UTF-8");
	} else if (!strcmp(locale, "C") ||
		   !strcmp(locale, "POSIX")) {
		return setlocale(category, "C");
	} else {
		errno = ENOENT;
		return NULL;
	}
}

/* Launch defaults filter HOME and supply ANDROID_ROOT until the app changes
 * them. No app-name checks, fixed-length name table, or inferred sandboxing:
 * ordinary variables still share the host process environment. */
static pthread_mutex_t app_env_lock = PTHREAD_MUTEX_INITIALIZER;
static int home_state, root_state; /* 0: launch default, 1: app-set, -1: removed */

static int *filtered_env_state(const char *name)
{
	if (!strcmp(name, "HOME")) return &home_state;
	if (!strcmp(name, "ANDROID_ROOT")) return &root_state;
	return NULL;
}

static char *app_getenv_locked(const char *name)
{
	int *state = filtered_env_state(name);
	if (state && *state != 1)
		return state == &root_state && *state == 0 ? "/system" : NULL;
	return getenv(name);
}

char *bionic_getenv(const char *name)
{
	if (!name) return NULL;
	pthread_mutex_lock(&app_env_lock);
	char *value = app_getenv_locked(name);
	pthread_mutex_unlock(&app_env_lock);
	return value;
}

int bionic_setenv(const char *name, const char *value, int overwrite)
{
	if (!name || !*name || strchr(name, '=') || !value) { errno = EINVAL; return -1; }
	pthread_mutex_lock(&app_env_lock);
	int ret = 0;
	if (overwrite || !app_getenv_locked(name)) {
		ret = setenv(name, value, 1);
		int *state = filtered_env_state(name);
		if (ret == 0 && state) *state = 1;
	}
	pthread_mutex_unlock(&app_env_lock);
	return ret;
}

int bionic_unsetenv(const char *name)
{
	if (!name || !*name || strchr(name, '=')) { errno = EINVAL; return -1; }
	pthread_mutex_lock(&app_env_lock);
	int ret = unsetenv(name);
	int *state = filtered_env_state(name);
	if (ret == 0 && state) *state = -1;
	pthread_mutex_unlock(&app_env_lock);
	return ret;
}

int bionic_putenv(char *entry)
{
	if (!entry || !*entry || *entry == '=') { errno = EINVAL; return -1; }
	char *equals = strchr(entry, '=');
	if (!equals) return bionic_unsetenv(entry);
	pthread_mutex_lock(&app_env_lock);
	int ret = putenv(entry); /* Preserve putenv's borrowed-buffer semantics. */
	if (ret == 0) {
		size_t length = (size_t)(equals - entry);
		if (length == 4 && !memcmp(entry, "HOME", 4)) home_state = 1;
		if (length == 12 && !memcmp(entry, "ANDROID_ROOT", 12)) root_state = 1;
	}
	pthread_mutex_unlock(&app_env_lock);
	return ret;
}

/* __gnu_strerror_r emulates glibc's strerror_r.
 * to make sure we get that, we define _GNU_SOURCE.
 * musl doesn't have any way to get the glibc behavior. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

char* bionic___gnu_strerror_r(int error_number, char* buf, size_t buf_len) {
#ifdef __GLIBC__
	return strerror_r(error_number, buf, buf_len);
#else // musl, or something else POSIX compliant
	int saved_errno = errno;
	strerror_r(error_number, buf, buf_len);
	errno = saved_errno;
	return buf;
#endif
}

/* Android exports the large-file alias even where host off_t is already 64 bit. */
int bionic_mkstemp64(char *path)
{
#ifdef __GLIBC__
	return mkstemp64(path);
#else
	return mkstemp(path);
#endif
}

/* getentropy accepts at most 256 bytes. It either fills the entire request or
 * fails; never return partial or predictable output from the arc4random API. */
void bionic_arc4random_buf(void *buffer, size_t length)
{
	unsigned char *bytes = buffer;
	while (length != 0) {
		size_t chunk = length > 256 ? 256 : length;
		if (getentropy(bytes, chunk) != 0) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "arc4random_buf: getentropy failed\n");
			abort();
		}
		bytes += chunk;
		length -= chunk;
	}
}

uint32_t bionic_arc4random(void)
{
	uint32_t value;
	bionic_arc4random_buf(&value, sizeof(value));
	return value;
}

uint32_t bionic_arc4random_uniform(uint32_t upper_bound)
{
	if (upper_bound < 2)
		return 0;
	/* Reject the uneven prefix instead of introducing modulo bias. */
	uint32_t minimum = -upper_bound % upper_bound;
	uint32_t value;
	do { value = bionic_arc4random(); } while (value < minimum);
	return value % upper_bound;
}
