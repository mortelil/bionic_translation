/* SPDX-License-Identifier: Apache-2.0 */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern int bionic_openat(int,const char *,int,...);
extern int bionic_openat64(int,const char *,int,...);
extern int bionic_open64(char *,int,...);
extern int bionic___openat_2(int,const char *,int);
extern int bionic___open_2(const char *,int);
extern void libc_bio_set_apply_path_overrides_func(bool (*)(char **));
static char mapped[256];
static bool map(char **path) {
 if (strcmp(*path,"/android-test/fixture")) return false;
 *path=strdup(mapped);assert(*path);return true;
}
static void content(int fd) {char b[4]={0};assert(fd>=0);assert(read(fd,b,3)==3&&!strcmp(b,"abc"));close(fd);}
int main(void) {
 struct rlimit lim={0,0};setrlimit(RLIMIT_CORE,&lim);
 char dir[]="/tmp/atl-open-XXXXXX";assert(mkdtemp(dir));
 int root=open(dir,O_RDONLY|O_DIRECTORY);assert(root>=0);
 int fd=bionic_openat(root,"file",O_CREAT|O_EXCL|O_WRONLY,0600);assert(fd>=0);
 assert(write(fd,"abc",3)==3);struct stat st;assert(!fstat(fd,&st)&&(st.st_mode&0777)==0600);close(fd);
 content(bionic_openat64(root,"file",O_RDONLY));
 content(bionic___openat_2(root,"file",O_RDONLY));
 fd=bionic_openat(root,".",O_RDONLY|O_DIRECTORY);assert(fd>=0);close(fd);
 fd=bionic_open64(dir,O_RDONLY|O_DIRECTORY);assert(fd>=0);close(fd);
 assert(bionic_openat(root,"absent",O_RDONLY)==-1&&errno==ENOENT);
 assert(bionic_openat(-1,"file",O_RDONLY)==-1&&errno==EBADF);
 snprintf(mapped,sizeof(mapped),"%s/file",dir);libc_bio_set_apply_path_overrides_func(map);
 content(bionic_openat(-1,"/android-test/fixture",O_RDONLY));
 content(bionic_openat64(-1,"/android-test/fixture",O_RDONLY));
 content(bionic___openat_2(-1,"/android-test/fixture",O_RDONLY));
 content(bionic___open_2("/android-test/fixture",O_RDONLY));
 for(int i=0;i<4;i++){pid_t p=fork();assert(p>=0);if(!p){int flags=i&1?O_TMPFILE|O_RDWR:O_CREAT|O_WRONLY;if(i<2)bionic___open_2(mapped,flags);else bionic___openat_2(root,"file",flags);_exit(1);}int status;assert(waitpid(p,&status,0)==p&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT);}
 assert(!unlinkat(root,"file",0));close(root);assert(!rmdir(dir));
 puts("PASS: relative openat, aliases, path overrides, errno, directory flags and fortify");
}
