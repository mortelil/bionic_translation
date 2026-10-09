// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
extern void *bionic_dlopen(const char *,int);
extern void *bionic_dlsym(void *,const char *);
extern int bionic_dlclose(void *);
int main(int argc,char **argv) {
    assert(argc==3);
    void *module=bionic_dlopen(argv[1],RTLD_NOW); assert(module);
    int (*result)(void)=bionic_dlsym(module,"atl_runpath_result"); assert(result);
    assert(result()==atoi(argv[2]));
    assert(!bionic_dlclose(module));
    puts("PASS: direct dependency RUNPATH resolution and callable relocated symbol");
}
