# readdir storage lifetime

Inside the build container:

```
cc tests/readdir/streams.c -ldl -o /tmp/readdir-streams
LD_LIBRARY_PATH="$PWD/build-review" /tmp/readdir-streams "$PWD/build-review/libc_bio.so.0"
```

Retain an entry from directory A while enumerating, rewinding and closing B.
A single shared conversion buffer fails deterministically; per-stream storage
passes. This matches readdir's contract: another call on the same stream may
invalidate an entry, but calls on a different stream must not overwrite it.

Firefox's recursive URL-classifier directory scan retains a parent entry during
nested enumeration. Corrupting it caused recursion into the wrong path and a
stack overflow. The test deliberately avoids relying on Firefox or its data.
