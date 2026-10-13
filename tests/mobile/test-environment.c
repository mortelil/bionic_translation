// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern char *bionic_getenv(const char *);
extern int bionic_setenv(const char *,const char *,int);
extern int bionic_unsetenv(const char *);
extern int bionic_putenv(char *);
int main(void) {
 assert(!setenv("HOME","host-home",1));assert(!bionic_getenv("HOME"));
 assert(!bionic_setenv("HOME","app-home",0));assert(!strcmp(bionic_getenv("HOME"),"app-home"));
 assert(!bionic_setenv("HOME","other",0));assert(!strcmp(bionic_getenv("HOME"),"app-home"));
 assert(!bionic_unsetenv("HOME"));assert(!bionic_getenv("HOME"));
 char home[]="HOME=borrowed";assert(!bionic_putenv(home));home[5]='B';
 assert(!strcmp(bionic_getenv("HOME"),"Borrowed"));assert(!bionic_putenv("HOME"));assert(!bionic_getenv("HOME"));
 assert(!strcmp(bionic_getenv("ANDROID_ROOT"),"/system"));
 assert(!bionic_setenv("ANDROID_ROOT","ignored",0));assert(!strcmp(bionic_getenv("ANDROID_ROOT"),"/system"));
 assert(!bionic_setenv("ANDROID_ROOT","/app-root",1));assert(!strcmp(bionic_getenv("ANDROID_ROOT"),"/app-root"));
 assert(!bionic_unsetenv("ANDROID_ROOT"));assert(!bionic_getenv("ANDROID_ROOT"));
 assert(bionic_setenv("bad=name","x",1)==-1&&errno==EINVAL);
 assert(bionic_unsetenv("")==-1&&errno==EINVAL);
 for(int i=0;i<100;i++){char key[160];memset(key,'a',sizeof(key));snprintf(key+130,30,"_%d",i);
  assert(!bionic_setenv(key,"value",0));assert(!strcmp(bionic_getenv(key),"value"));assert(!bionic_unsetenv(key));}
 puts("PASS: filtered launch defaults, app writes/removal, overwrite, putenv buffer, long names");
}
