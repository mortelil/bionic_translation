// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <link.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
extern void *bionic_dlopen(const char *, int);
extern int bionic_dlclose(void *);
extern int bionic_dl_iterate_phdr(int (*)(struct dl_phdr_info *,size_t,void *),void *);
static const char *fixture;
static atomic_int done;
static unsigned observations;
static int inspect(struct dl_phdr_info *info, size_t size, void *data) {
    assert(size >= offsetof(struct dl_phdr_info, dlpi_phnum) + sizeof(info->dlpi_phnum));
    assert(info->dlpi_name && !strstr(info->dlpi_name,"broken.so"));
    if (strstr(info->dlpi_name,"valid.so")) (*(int*)data)++;
    for (unsigned i=0; i<info->dlpi_phnum; i++) {
        volatile ElfW(Word) type=info->dlpi_phdr[i].p_type;
        (void)type;
    }
    observations++;
    return 0;
}
static int stop(struct dl_phdr_info *info,size_t size,void *data) {
    (void)info; (void)size; (*(int*)data)++; return 73;
}
static void *reload(void *unused) {
    (void)unused;
    for (int i=0; i<200; i++) {
        void *handle=bionic_dlopen(fixture,RTLD_NOW); assert(handle);
        assert(!bionic_dlclose(handle));
    }
    atomic_store(&done,1); return NULL;
}
int main(int argc,char **argv) {
    assert(argc==3); fixture=argv[1];
    assert(!bionic_dlopen(argv[2],RTLD_NOW));
    void *handle=bionic_dlopen(fixture,RTLD_NOW); assert(handle);
    int found=0;
    assert(!bionic_dl_iterate_phdr(inspect,&found) && found==1 && observations>1);
    int stopped=0; assert(bionic_dl_iterate_phdr(stop,&stopped)==73 && stopped==1);
    assert(!bionic_dlclose(handle)); found=0;
    assert(!bionic_dl_iterate_phdr(inspect,&found) && found==0);
    pthread_t thread; assert(!pthread_create(&thread,NULL,reload,NULL));
    do { found=0; assert(!bionic_dl_iterate_phdr(inspect,&found)); } while (!atomic_load(&done));
    assert(!pthread_join(thread,NULL));
    puts("PASS: failed mappings omitted, valid headers readable, early stop and concurrent load/unload enumeration");
}
