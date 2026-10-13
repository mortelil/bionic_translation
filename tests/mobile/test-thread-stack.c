// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
/* Bionic's public pthread_attr_t storage; the bridge uses its first pointer. */
typedef union { void *alignment; unsigned char bytes[128]; } Attr;
extern int bionic_pthread_attr_init(void *);
extern int bionic_pthread_attr_destroy(void *);
extern int bionic_pthread_attr_setstacksize(void *, size_t);
extern int bionic_pthread_attr_getstacksize(const void *, size_t *);
extern int bionic_pthread_attr_setstack(void *, void *, size_t);
extern int bionic_pthread_create(long *, const void *, void *(*)(void *), void *);
static size_t actual_size;
static void *actual_base;
static void *worker(void *unused) {
 (void)unused;
 pthread_attr_t attr;
 assert(!pthread_getattr_np(pthread_self(), &attr));
 assert(!pthread_attr_getstack(&attr, &actual_base, &actual_size));
 pthread_attr_destroy(&attr);
 return NULL;
}
static void run(const void *attr, size_t expected, void *base) {
 long thread;
 assert(!bionic_pthread_create(&thread, attr, worker, NULL));
 assert(!pthread_join((pthread_t)thread, NULL));
 /* Compare with the same explicit host attributes: musl reserves TLS inside
  * a caller-owned stack, but adds alignment padding to an allocated stack. */
 const size_t translated_size = actual_size;
 if (base) assert(actual_base == base);
 pthread_attr_t host;
 assert(!pthread_attr_init(&host));
 if (base) assert(!pthread_attr_setstack(&host,base,expected));
 else assert(!pthread_attr_setstacksize(&host,expected));
 pthread_t control;
 assert(!pthread_create(&control,&host,worker,NULL));
 assert(!pthread_join(control,NULL));
 assert(actual_size == translated_size);
 if (base) assert(actual_base == base);
 assert(!pthread_attr_destroy(&host));
}
int main(void) {
 const size_t def = sizeof(void*) == 8 ? 992*1024 : 1008*1024;
 Attr attr; size_t size;
 run(NULL, def, NULL);
 assert(!bionic_pthread_attr_init(&attr));
 assert(!bionic_pthread_attr_getstacksize(&attr,&size) && size == def);
 run(&attr, def, NULL);
 assert(bionic_pthread_attr_setstacksize(&attr,1) == EINVAL);
 assert(!bionic_pthread_attr_setstacksize(&attr,128*1024));
 assert(!bionic_pthread_attr_getstacksize(&attr,&size) && size == 128*1024);
 run(&attr,128*1024,NULL);
 assert(!bionic_pthread_attr_getstacksize(&attr,&size) && size == 128*1024);
 void *stack=mmap(NULL,256*1024,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 assert(stack!=MAP_FAILED);
 assert(!bionic_pthread_attr_setstack(&attr,stack,256*1024));
 run(&attr,256*1024,stack);
 assert(!bionic_pthread_attr_destroy(&attr));
 assert(!munmap(stack,256*1024));
 puts("PASS: default, explicit, caller-owned stacks and invalid size");
}
