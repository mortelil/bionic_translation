// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "trap-signal.h"
extern void *bionic_dlopen(const char *, int);
extern void *bionic_dlsym(void *, const char *);
extern int bionic_dlclose(void *);
extern void bionic___cfi_slowpath(uint64_t, void *);
extern void bionic___cfi_slowpath_diag(uint64_t, void *, void *);
static void rejected(uint64_t type, void *target, int signal) {
    pid_t pid = fork(); assert(pid >= 0);
    if (!pid) { bionic___cfi_slowpath(type, target); _exit(0); }
    int status; assert(waitpid(pid, &status, 0) == pid);
    if (!WIFSIGNALED(status) || WTERMSIG(status) != signal) fprintf(stderr,"expected signal %d, status %#x for target %p\n",signal,status,target);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == signal);
}
int main(int argc, char **argv) {
    assert(argc == 3);
    struct rlimit limit = {0, 0}; setrlimit(RLIMIT_CORE, &limit);
    const uint64_t valid = UINT64_C(0x123456789abcdef0);
    for (int i=0; i<3; i++) {
        void *module = bionic_dlopen(argv[1], RTLD_NOW); assert(module);
        void *target = bionic_dlsym(module, "atl_cfi_target"); assert(target);
        int diag = 0;
        bionic___cfi_slowpath_diag(valid, target, &diag); assert(diag == 73);
        bionic___cfi_slowpath(valid, target);
        rejected(0, target, CFI_TRAP_SIGNAL);
        bionic_dlclose(module);
        rejected(valid, target, SIGABRT);
    }
    void *module = bionic_dlopen(argv[2], RTLD_NOW); assert(module);
    void *target = bionic_dlsym(module, "atl_cfi_target"); assert(target);
    bionic___cfi_slowpath(0, target); bionic_dlclose(module);
    module = dlopen(argv[1], RTLD_NOW); assert(module);
    target = dlsym(module, "atl_cfi_target"); assert(target);
    int diag = 0; bionic___cfi_slowpath_diag(valid, target, &diag); assert(diag == 73);
    rejected(0, target, CFI_TRAP_SIGNAL); dlclose(module);
    bionic___cfi_slowpath(0, (void *)puts);
    bionic___cfi_slowpath(0, (void *)main);
    rejected(valid, NULL, SIGABRT);
    rejected(valid, &diag, SIGABRT);
    void *heap = malloc(32); rejected(valid, heap, SIGABRT); free(heap);
    puts("PASS: CFI target dispatch, diagnostics, wrong type rejection, unload/reload, host and unchecked modules, invalid pointers");
}
