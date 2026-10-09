# ELF direct-dependency RUNPATH

Run `sh tests/runpath/run.sh` in Alpine. The fixtures have direct dependencies
outside BIONIC_LD_LIBRARY_PATH and no host counterpart. Tests call the relocated
function after loading, cover both `$ORIGIN` and `${ORIGIN}`, an absent directory
before the real directory, and an explicit library-path override with a different
return value. The test fails against the previous loader.

DT_RUNPATH is evaluated for each requesting object's direct DT_NEEDED entries,
after BIONIC_LD_LIBRARY_PATH and before configured system paths/host fallback.
ORIGIN uses the resolved directory of the loaded object's path; paths are not
added globally or inherited by grandchildren. The loader's existing 512-byte
path capacity remains in force. Entries with unsupported dynamic tokens are
skipped; `$LIB`, `$PLATFORM` and legacy inherited DT_RPATH are not implemented.
Existing explicit platform-library overrides and loaded-library reuse remain.
This is not a loader namespace or sandbox implementation.

AI-assisted implementation and tests; no app-specific library names are used.
