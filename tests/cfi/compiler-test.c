// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "trap-signal.h"
extern void *bionic_dlopen(const char *, int);
extern void *bionic_dlsym(void *, const char *);
extern int bionic_dlclose(void *);
int main(int argc, char **argv) {
    assert(argc == 3);
    struct rlimit limit = {0,0}; setrlimit(RLIMIT_CORE,&limit);
    void *callee = bionic_dlopen(argv[1], RTLD_NOW); assert(callee);
    void *caller = bionic_dlopen(argv[2], RTLD_NOW); assert(caller);
    int (*invoke)(int (*)(int)) = bionic_dlsym(caller,"atl_cfi_invoke"); assert(invoke);
    int (*valid)(int) = bionic_dlsym(callee,"atl_cfi_add"); assert(valid);
    int (*wrong)(int) = bionic_dlsym(callee,"atl_cfi_wrong"); assert(wrong);
    assert(invoke(valid)==42);
    pid_t child=fork(); assert(child>=0);
    if (!child) { invoke(wrong); _exit(0); }
    int status; assert(waitpid(child,&status,0)==child);
    assert(WIFSIGNALED(status) && WTERMSIG(status)==CFI_TRAP_SIGNAL);
    bionic_dlclose(caller); bionic_dlclose(callee);
    puts("PASS: Clang cross-DSO CFI allows correct signature and traps wrong signature");
}
