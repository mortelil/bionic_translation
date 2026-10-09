// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
extern void *bionic_dlopen(const char *, int);
extern void *bionic_dlsym(void *, const char *);
extern int bionic_dlclose(void *);
extern int bionic___cxa_thread_atexit_impl(void (*)(void *),void *,void *);
static pthread_barrier_t barrier;
static int (*register_dtors)(int *);
static int result;
static void *worker(void *unused) {
    assert(!register_dtors(&result));
    pthread_barrier_wait(&barrier);
    pthread_barrier_wait(&barrier);
    return NULL;
}
static void report_exit(void *p) { int fd=*(int *)p; assert(write(fd,"x",1)==1); }
int main(int argc,char **argv) {
    assert(argc==2);
    void *module=bionic_dlopen(argv[1],RTLD_NOW); assert(module);
    register_dtors=bionic_dlsym(module,"atl_register_thread_destructors"); assert(register_dtors);
    assert(!pthread_barrier_init(&barrier,NULL,2));
    pthread_t thread; assert(!pthread_create(&thread,NULL,worker,NULL));
    pthread_barrier_wait(&barrier);
    bionic_dlclose(module); // The registered callbacks must retain their DSO.
    pthread_barrier_wait(&barrier);
    assert(!pthread_join(thread,NULL)); fprintf(stderr,"destructor order=%d\n",result); assert(result==231);
    pthread_barrier_destroy(&barrier);
    int pipefd[2]; assert(!pipe(pipefd));
    pid_t child=fork(); assert(child>=0);
    if (!child) { close(pipefd[0]); assert(!bionic___cxa_thread_atexit_impl(report_exit,&pipefd[1],NULL)); exit(0); }
    close(pipefd[1]); char c=0; assert(read(pipefd[0],&c,1)==1 && c=='x'); close(pipefd[0]);
    int status; assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
    puts("PASS: thread destructor LIFO, registration during teardown, DSO retained across dlclose and main-thread exit");
}
