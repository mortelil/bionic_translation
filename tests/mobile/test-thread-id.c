// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
extern pid_t bionic_pthread_gettid_np(long);
extern int bionic_pthread_create(long *, const void *, void *(*)(void *), void *);
static pthread_barrier_t ready, release;
static pid_t worker_tid;
static void *worker(void *unused) {
	(void)unused;
	worker_tid = syscall(SYS_gettid);
	assert(bionic_pthread_gettid_np((long)pthread_self()) == worker_tid);
	pthread_barrier_wait(&ready);
	pthread_barrier_wait(&release);
	return NULL;
}
int main(void) {
	assert(bionic_pthread_gettid_np((long)pthread_self()) == syscall(SYS_gettid));
	assert(bionic_pthread_gettid_np(0) == -1);
	assert(!pthread_barrier_init(&ready,NULL,2));
	assert(!pthread_barrier_init(&release,NULL,2));
	for (int i=0; i<20; i++) {
		pthread_t thread;
		if (i & 1) {
			long android_thread;
			assert(!bionic_pthread_create(&android_thread,NULL,worker,NULL));
			thread=(pthread_t)android_thread;
		} else assert(!pthread_create(&thread,NULL,worker,NULL));
		pthread_barrier_wait(&ready);
		assert(worker_tid != syscall(SYS_gettid));
		assert(bionic_pthread_gettid_np((long)thread) == worker_tid);
		pthread_barrier_wait(&release);
		assert(!pthread_join(thread,NULL));
	}
	pthread_barrier_destroy(&ready); pthread_barrier_destroy(&release);
	puts("PASS: kernel TID agrees for main, host and Android-created live threads");
}
