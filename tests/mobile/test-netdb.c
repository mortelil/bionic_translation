// SPDX-License-Identifier: Apache-2.0
#include <arpa/inet.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
extern void *bionic_dlsym(void *, const char *);

int main(void) {
  int (*lookup)(const struct sockaddr *, socklen_t, char *, socklen_t, char *,
                socklen_t, int) = bionic_dlsym(RTLD_DEFAULT, "getnameinfo");
  assert(lookup);
  struct sockaddr_in address = {.sin_family = AF_INET,
                                .sin_port = htons(53317)};
  assert(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) == 1);
  char host[128], service[32];
  /* Android NI_NUMERICHOST alone must suppress reverse DNS. */
  assert(lookup((void *)&address, sizeof(address), host, sizeof(host), NULL, 0,
                2) == 0);
  assert(!strcmp(host, "127.0.0.1"));
  assert(lookup((void *)&address, sizeof(address), host, sizeof(host), service,
                sizeof(service), 2 | 8 | 16) == 0);
  assert(!strcmp(host, "127.0.0.1") && !strcmp(service, "53317"));
  struct sockaddr_in6 v6 = {.sin6_family = AF_INET6, .sin6_port = htons(443)};
  assert(inet_pton(AF_INET6, "::1", &v6.sin6_addr) == 1);
  assert(lookup((void *)&v6, sizeof(v6), host, sizeof(host), service,
                sizeof(service), 2 | 8) == 0);
  assert(!strcmp(host, "::1") && !strcmp(service, "443"));
  assert(lookup((void *)&address, sizeof(address), host, 1, NULL, 0, 2) == 14);
  assert(lookup((void *)&address, sizeof(address), host, sizeof(host), NULL, 0,
                0x4000) == 3);
  puts("PASS: Android getnameinfo flags, numeric IPv4/IPv6, port and error "
       "codes via linker lookup");
}
