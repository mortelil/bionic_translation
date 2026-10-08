// SPDX-License-Identifier: Apache-2.0
#include <netdb.h>
#include <sys/socket.h>

/* Android uses the BSD NI_* bit assignments and positive EAI_* codes.
 * Both musl and glibc use different flag values and negative error codes. */
int bionic_getnameinfo(const struct sockaddr *address, socklen_t address_length,
                       char *host, socklen_t host_length, char *service,
                       socklen_t service_length, int flags) {
  if (flags & ~0x1f)
    return 3; /* Android EAI_BADFLAGS; NI_WITHSCOPEID is unsupported. */
  int native_flags = 0;
  if (flags & 0x01)
    native_flags |= NI_NOFQDN;
  if (flags & 0x02)
    native_flags |= NI_NUMERICHOST;
  if (flags & 0x04)
    native_flags |= NI_NAMEREQD;
  if (flags & 0x08)
    native_flags |= NI_NUMERICSERV;
  if (flags & 0x10)
    native_flags |= NI_DGRAM;

  switch (getnameinfo(address, address_length, host, host_length, service,
                      service_length, native_flags)) {
  case 0:
    return 0;
  case EAI_AGAIN:
    return 2;
  case EAI_BADFLAGS:
    return 3;
  case EAI_FAIL:
    return 4;
  case EAI_FAMILY:
    return 5;
  case EAI_MEMORY:
    return 6;
  case EAI_NONAME:
    return 8;
  case EAI_SERVICE:
    return 9;
  case EAI_SOCKTYPE:
    return 10;
  case EAI_SYSTEM:
    return 11;
  case EAI_OVERFLOW:
    return 14;
  default:
    return 4;
  }
}
