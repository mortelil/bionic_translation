# Experimental bionic_translation for Linux mobile

Unofficial companion fork for the experimental ATL mobile branch. Upstream:
https://gitlab.com/android_translation_layer/bionic_translation

Changes add missing fortified/file adapters, atfork registration/cleanup and
process-scope symbol lookup used by Dart. These were developed with Codex
assistance. Existing source history and notices are retained. This is not a fork
of Google's full Bionic libc and is not a claim of complete ABI compatibility.

Build using the sibling ATL checkout's `scripts/mobile/build.sh`. Run the local
native regression cases with `sh tests/mobile/run.sh` inside Alpine after that
build. The default private installation is `../.atl-runtime`; `ATL_PREFIX` can
select another prefix. Tests cover I/O, rejected bounds, callback order and
symbol precedence; concurrent/reentrant atfork behavior remains a review need.

New atfork and mobile-test files are Apache-2.0 (see their SPDX headers and
`LICENSES/Apache-2.0.txt`). Existing files retain their individual licenses; this
statement does not relicense the repository. Upstream's README follows.

---

### a set of libraries for loading bionic-linked .so files on musl/glibc

- the bionic linker under `bionic_translation/linker/` is taken from https://github.com/Cloudef/android2gnulinux and partly modified for our purposes
- the pthread wrapper under `bionic_translation/pthread_wrapper/` is taken from the same place, and augmented with additional missing wrapper functions
same with `bionic_translation/libc/`
- `bionic_translation/wrapper/` is a helper for these, also from android2gnulinux
- `bionic_translation/libstdc++_standalone` is taken from bionic sources and coerced to compile; it's just "a minimum implementation of libc++ functionality not provided by compiler",
and things break when it's not linked in and android libs try to call into it and instead end up in the glibc or llvm libc++ implementations

### main_executable

`main_executable/bionic_compat.c` contains things which need to be linked into the main executable
for various reasons. Currently, it contains:
- `_r_debug` hacks for musl (although the gdb hooks don't seem to work particularly well, see below)
- for arm(64), static initialization for bionic TLS slots 2-7

For now, this needs to be copied into your project.
(TODO: build a static library?)

### environment variables

```
BIONIC_LD_LIBRARY_PATH=... - An alternative to calling `dl_parse_library_path`. It contains
                             colon-separated paths of directories which the linker will assume
                             contain libraries that it's meant to link; all other libraries will be
                             passed to the platform linker.
LINKER_DIE_AT_RUNTIME= - When a function is not found during linking, link in a stub that will print the function name and abort if it's ever actually called.
LINKER_PRINT_MISSING_SYMBOLS_AND_DIE= - When a non-function symbol is not found during linking, we can't stub it, but we can wait and abort only after we print all the missing symbols for that .so
LINKER_IGNORE_UNKNOWN_RELOCS= - Don't set this. It's used when we run tests, because we know the missing relocs are only needed by a few tests which will correctly be marked as failing with this set.
```

### dependencies

the -dev(el) packages for: `egl libelf libbsd libunwind`

example of installing these on alpine (which supports using pkgconfig names):
`apk add pc:egl pc:libelf pc:libbsd pc:libunwind`

the suspicious egl dependency is necessary in order to resolve gl* functions which are always
exported on android but pretty much never exported on desktop Linux.

### note on debugging with gdb

At least with glibc, the following sometimes fixes issues with shared library load notifications:
```
objcopy -R .note.stapsdt /usr/lib64/ld-linux-x86-64.so.2 ld-linux-x86-64.so.2-nostap
gdb --args ./ld-linux-x86-64.so.2-nostap debugged-executable
```
This ensures that gdb is unable to use the stap-probe-based system for communication with
the system linker, and will fall back to using the r_debug mechanism.

If this doesn't help, you can also add the libraries manually (once they are loaded), like this:
`eval "add-symbol-file %s -o apkenv_sopool[0].base", apkenv_sopool[0].fullpath` (repeat for `[1]` etc
depending on how many libraries have been loaded)

A less annoying option is to automate the manual method:
```
hb apkenv_insert_soinfo_into_debug_map
commands
eval "add-symbol-file %s -o info->base", info->fullpath
c
end
```
This will break in the function that is trying to notify gdb, and use the information that's passed
to this function to effectively do what the function should be doing already but for whatever reason
fails at.

### Experimental mobile compatibility tests

After installing the private runtime, run `sh tests/mobile/run.sh` inside Alpine.
The suite now also requires `clang` and `lld` for real cross-DSO CFI fixtures.
See `tests/cfi/README.md` and `tests/thread-atexit/README.md` for scope and known
limits. The thread-exit adapter uses the host GCC `libstdc++.so.6` runtime.

The property callback adapter reads the existing ATL SDK/fingerprint properties.
SDK value and revision share an atomic snapshot; callbacks run without a held
lock and may reenter the property API. Tests check cookie delivery, value/serial
consistency, unchanged-value revision stability and reentrant reads. This is not
a general Android property service: dynamic property creation, persistence,
notifications and waits remain unsupported. API contract reference:
https://android.googlesource.com/platform/bionic/+/master/libc/include/sys/system_properties.h
