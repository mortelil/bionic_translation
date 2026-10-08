// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

/* Keep DSO ownership so finalization removes callbacks before unmapping code. */
struct fork_handler {
	void (*prepare)(void), (*parent)(void), (*child)(void);
	void *dso;
	struct fork_handler *prev, *next;
};
static struct fork_handler *first, *last;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int installed;

static void prepare(void)
{
	pthread_mutex_lock(&lock);
	for (struct fork_handler *h = last; h; h = h->prev)
		if (h->prepare) h->prepare();
}

static void parent(void)
{
	for (struct fork_handler *h = first; h; h = h->next)
		if (h->parent) h->parent();
	pthread_mutex_unlock(&lock);
}

static void child(void)
{
	for (struct fork_handler *h = first; h; h = h->next)
		if (h->child) h->child();
	pthread_mutex_unlock(&lock);
}

int bionic___register_atfork(void (*prep)(void), void (*par)(void),
		void (*ch)(void), void *dso)
{
	struct fork_handler *h = malloc(sizeof(*h));
	if (!h) return ENOMEM;
	pthread_mutex_lock(&lock);
	if (!installed) {
		int ret = pthread_atfork(prepare, parent, child);
		if (ret) {
			pthread_mutex_unlock(&lock);
			free(h);
			return ret;
		}
		installed = 1;
	}
	*h = (struct fork_handler){prep, par, ch, dso, last, NULL};
	if (last) last->next = h;
	else first = h;
	last = h;
	pthread_mutex_unlock(&lock);
	return 0;
}

void bionic___unregister_atfork(void *dso)
{
	pthread_mutex_lock(&lock);
	for (struct fork_handler *h = first, *next; h; h = next) {
		next = h->next;
		if (h->dso != dso) continue;
		if (h->prev) h->prev->next = h->next;
		else first = h->next;
		if (h->next) h->next->prev = h->prev;
		else last = h->prev;
		free(h);
	}
	pthread_mutex_unlock(&lock);
}

extern void __cxa_finalize(void *dso);
void bionic___cxa_finalize(void *dso)
{
	__cxa_finalize(dso);
	if (dso) bionic___unregister_atfork(dso);
}
