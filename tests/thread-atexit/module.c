// SPDX-License-Identifier: Apache-2.0
extern int __cxa_thread_atexit_impl(void (*)(void *), void *, void *);
static char module_dso;
static void third(void *p) { int *n=p; *n=*n*10+3; }
static void first(void *p) { int *n=p; *n=*n*10+1; }
static void second(void *p) {
    int *n=p; *n=*n*10+2;
    if (__cxa_thread_atexit_impl(third,p,&module_dso)) __builtin_trap();
}
int atl_register_thread_destructors(int *result) {
    return __cxa_thread_atexit_impl(first,result,&module_dso) ||
           __cxa_thread_atexit_impl(second,result,&module_dso);
}
