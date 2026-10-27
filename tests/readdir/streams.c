/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct android_dirent { uint64_t ino; int64_t off; unsigned short reclen; unsigned char type; char name[256]; };
int main(int argc,char **argv) {
    assert(argc==2);
    void *library=dlopen(argv[1],RTLD_LAZY|RTLD_LOCAL); if(!library){puts(dlerror());return 2;}
    struct android_dirent *(*read_entry)(DIR *)=dlsym(library,"bionic_readdir"); assert(read_entry);
    DIR *a=opendir("/tmp"),*b=opendir("/tmp"); assert(a&&b);
    struct android_dirent *entry=read_entry(a); assert(entry);
    char saved[256]; strcpy(saved,entry->name);
    struct android_dirent *other;
    int different=0;
    while((other=read_entry(b))) {
        different |= strcmp(saved,other->name)!=0;
        if(strcmp(saved,entry->name)!=0){fprintf(stderr,"one DIR overwrote another DIR's entry\n");return 1;}
        assert(other->reclen >= offsetof(struct android_dirent,name)+strlen(other->name)+1);
    }
    assert(different);
    rewinddir(b); assert(read_entry(b)); assert(strcmp(saved,entry->name)==0);
    closedir(b); assert(strcmp(saved,entry->name)==0); closedir(a);
    puts("READDIR_STREAM_PASS"); return 0;
}
