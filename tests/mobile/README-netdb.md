# Numeric network addresses

Android/BSD `NI_NUMERICHOST` is 0x02 while musl/glibc use a different bit.
Passing Android's flags directly to host getnameinfo can return reverse-DNS
names instead of numeric addresses. LocalSend's Dart NetworkInterface.list()
exposed this mismatch. libc-netdb.c translates the five portable NI_* flags and
maps host failures to Android's positive EAI_* values. NI_WITHSCOPEID and unknown
flags are rejected rather than silently ignored. This does not complete the
separate getaddrinfo/gai_strerror ABI translation.

Run `sh tests/mobile/run.sh` in the Alpine build environment with ATL_PREFIX
pointing to the installed private overlay. test-netdb.c resolves the function
through bionic_dlsym, then checks IPv4/IPv6 numeric addresses, numeric service
ports, datagram flags, overflow and invalid flags without external DNS.
It passed on x86_64 and aarch64 on 2026-10-08.

ABI reference: AOSP platform/bionic libc/include/netdb.h (NI_* and EAI_*).
The implementation and synthetic test were written with AI assistance and are
not copied from AOSP. No app-specific address or interface is hardcoded.
