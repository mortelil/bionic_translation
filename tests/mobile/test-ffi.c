// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
extern void *bionic_dlsym(void *, const char *);
int main(void) {
    void *(*alloc)(size_t,size_t) = bionic_dlsym(RTLD_DEFAULT, "calloc");
    void (*release)(void *) = bionic_dlsym(RTLD_DEFAULT, "free");
    assert(alloc && release);
    unsigned char *p = alloc(16, 1);
    assert(p);
    for (int i=0; i<16; ++i) assert(p[i] == 0);
    release(p);
    assert(bionic_dlsym(RTLD_DEFAULT, "pthread_mutex_lock") == dlsym(RTLD_DEFAULT, "bionic_pthread_mutex_lock"));
    assert(!bionic_dlsym(RTLD_DEFAULT, "atl_definitely_missing_symbol"));
    puts("PASS: Dart process-scope calloc/free, ABI override priority, missing symbol failure");
}
