// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern size_t bionic___strlcpy_chk(char *,const char *,size_t,size_t);
extern size_t bionic___strlcat_chk(char *,const char *,size_t,size_t);
extern int bionic_mkstemp64(char *);
extern void bionic_arc4random_buf(void *,size_t);
extern uint32_t bionic_arc4random(void),bionic_arc4random_uniform(uint32_t);
static void overflow(int cat) {
 pid_t pid=fork(); assert(pid>=0);
 if(pid==0) {struct rlimit limit={0,0};setrlimit(RLIMIT_CORE,&limit); char b[4]="";
  if(cat)bionic___strlcat_chk(b,"test",5,sizeof(b));
  else bionic___strlcpy_chk(b,"test",5,sizeof(b));
  _exit(42);
 }
 int status;assert(waitpid(pid,&status,0)==pid);
 assert(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT);
}
int main(void) {
 char a[8]="",b[8]="";
 assert(bionic___strlcpy_chk(a,"123456789",sizeof(a),sizeof(a))==strlcpy(b,"123456789",sizeof(b)));
 assert(!memcmp(a,b,sizeof(a)));
 assert(bionic___strlcpy_chk(a,"abc",0,sizeof(a))==3);
 strcpy(a,"ab");strcpy(b,"ab");
 assert(bionic___strlcat_chk(a,"cdefghi",sizeof(a),sizeof(a))==strlcat(b,"cdefghi",sizeof(b)));
 assert(!memcmp(a,b,sizeof(a)));overflow(0);overflow(1);
 char path[]="/tmp/atl-mkstemp-XXXXXX";
 mode_t saved=umask(0);int fd=bionic_mkstemp64(path);umask(saved);assert(fd>=0);
 struct stat st;assert(!fstat(fd,&st)&&((st.st_mode&0777)==0600));close(fd);unlink(path);
 unsigned char bytes[1026];memset(bytes,0xa5,sizeof(bytes));
 bionic_arc4random_buf(bytes+1,1024);assert(bytes[0]==0xa5&&bytes[1025]==0xa5);
 bionic_arc4random_buf(NULL,0);
 assert(bionic_arc4random_uniform(0)==0&&bionic_arc4random_uniform(1)==0);
 for(int i=0;i<2000;i++){assert(bionic_arc4random_uniform(7)<7);assert(bionic_arc4random_uniform(UINT32_MAX)<UINT32_MAX);}
 (void)bionic_arc4random();
 puts("PASS: fortify bounds/truncation, temporary-file mode, random buffer bounds and uniform range");
}
