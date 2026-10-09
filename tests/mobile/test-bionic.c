// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern int bionic___openat_2(int, const char *, int);
extern ssize_t bionic___pread_chk(int, void *, size_t, off_t, size_t);
extern ssize_t bionic___readlink_chk(const char *, char *, size_t, size_t);
extern int bionic___poll_chk(struct pollfd *, nfds_t, int, size_t);
extern int bionic_fstatat64(int, const char *, struct stat *, int);
extern ssize_t bionic_sendfile64(int, int, off_t *, size_t);
extern int bionic___register_atfork(void (*)(void), void (*)(void), void (*)(void), void *);
extern void bionic___cxa_finalize(void *);
extern char *bionic___strncat_chk(char *, const char *, size_t, size_t);
static int order;
static void prep1(void) { order = order * 10 + 1; }
static void prep2(void) { order = order * 10 + 2; }
static void par1(void) { order = order * 10 + 3; }
static void par2(void) { order = order * 10 + 4; }
static void ch1(void) { order = order * 10 + 5; }
static void ch2(void) { order = order * 10 + 6; }
static void invalid_concat(void) { char b[3]="ab"; bionic___strncat_chk(b,"c",1,sizeof b); }
static void unterminated_concat(void) { char b[2]={'a','b'}; bionic___strncat_chk(b,"",0,sizeof b); }
static void zero_concat(void) { char b[1]; bionic___strncat_chk(b,"",0,0); }
static void invalid_open(void) { bionic___openat_2(AT_FDCWD, "bad", O_CREAT); }
static void invalid_pread(void) { char b[1]; bionic___pread_chk(-1,b,2,0,1); }
static void invalid_poll(void) { struct pollfd p; bionic___poll_chk(&p,2,0,sizeof p); }
static void invalid_link(void) { char b[1]; bionic___readlink_chk("missing",b,2,1); }
static void aborts(void (*f)(void)) {
	pid_t p = fork(); assert(p >= 0);
	if (!p) { f(); _exit(99); }
	int s; assert(waitpid(p,&s,0)==p); assert(WIFSIGNALED(s) && WTERMSIG(s)==SIGABRT);
}
int main(void) {
	struct rlimit r = {0,0}; setrlimit(RLIMIT_CORE,&r);
	char cat[6]="ab";
	assert(bionic___strncat_chk(cat,"cdef",3,sizeof cat)==cat && !strcmp(cat,"abcde"));
	assert(bionic___strncat_chk(cat,"x",0,sizeof cat)==cat && !strcmp(cat,"abcde"));
	assert(bionic___strncat_chk(cat,"",(size_t)-1,sizeof cat)==cat && !strcmp(cat,"abcde"));
	char shortcat[4]="a";
	assert(bionic___strncat_chk(shortcat,"b",(size_t)-1,sizeof shortcat)==shortcat && !strcmp(shortcat,"ab"));
	char raw[2]={'x','y'}, exact[3]="";
	assert(bionic___strncat_chk(exact,raw,2,sizeof exact)==exact && !strcmp(exact,"xy"));
	aborts(invalid_concat); aborts(unterminated_concat); aborts(zero_concat);
	puts("PASS: fortified strncat exact fit, truncation, zero count, unterminated source and overflow rejection");
	FILE *file = tmpfile(); assert(file);
	assert(write(fileno(file),"abcdef",6)==6);
	char b[16]={0}; assert(bionic___pread_chk(fileno(file),b,3,2,sizeof b)==3);
	assert(!memcmp(b,"cde",3)); assert(lseek(fileno(file),0,SEEK_CUR)==6);
	int dir = open("/dev",O_DIRECTORY); assert(dir>=0);
	int fd = bionic___openat_2(dir,"null",O_RDONLY); assert(fd>=0); close(fd);
	struct stat st; assert(!bionic_fstatat64(dir,"null",&st,0)); assert(S_ISCHR(st.st_mode)); close(dir);
	errno=0; assert(bionic___openat_2(-1,"relative",O_RDONLY)==-1 && errno==EBADF);
	assert(bionic___poll_chk(NULL,0,0,0)==0);
	assert(bionic___readlink_chk("/proc/self/exe",b,sizeof b,sizeof b)>0);
	FILE *out = tmpfile(); assert(out); off_t off=1;
	assert(bionic_sendfile64(fileno(out),fileno(file),&off,3)==3 && off==4);
	assert(pread(fileno(out),b,3,0)==3 && !memcmp(b,"bcd",3));
	aborts(invalid_open); aborts(invalid_pread); aborts(invalid_poll); aborts(invalid_link);
	static int dso1,dso2;
	assert(!bionic___register_atfork(prep1,par1,ch1,&dso1));
	assert(!bionic___register_atfork(prep2,par2,ch2,&dso2));
	pid_t p=fork(); assert(p>=0); if(!p) _exit(order==2156 ? 0 : 1);
	int s; waitpid(p,&s,0); assert(WIFEXITED(s)&&WEXITSTATUS(s)==0); assert(order==2134);
	bionic___cxa_finalize(&dso2); order=0;
	p=fork(); assert(p>=0); if(!p) _exit(order==15 ? 0 : 1);
	waitpid(p,&s,0); assert(WIFEXITED(s)&&WEXITSTATUS(s)==0); assert(order==13);
	bionic___cxa_finalize(&dso1);
	fclose(file); fclose(out); puts("PASS: file operations, bounds checks, atfork ordering and DSO cleanup");
}
