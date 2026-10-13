# File-open family

Adds OMP Firefox candidate fa8af3e's openat/openat64 adapters with the same path
override callback as open. Relative paths retain dirfd semantics; overridden
absolute paths follow normal host openat semantics. Errors remain host errno.
This is path translation, not a filesystem sandbox.

Integration corrects the shared O_TMPFILE predicate (O_DIRECTORY alone must not
consume a variadic mode), and routes the fortified variants through the same
path translation. Both fortified variants abort when flags require a missing
mode, matching Android bionic's open.cpp contract (android-15.0.0_r1).

The mobile runner verifies relative file creation/read, modes, aliases,
absolute overrides, error codes, O_DIRECTORY without a mode, and child-process
SIGABRT for O_CREAT/O_TMPFILE calls without a mode. Tested on Alpine x86_64.
Actual O_TMPFILE creation depends on filesystem support and is not asserted.
