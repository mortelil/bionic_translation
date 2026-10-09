# Loaded ELF enumeration

Run `sh tests/iterate-phdr/run.sh` in Alpine. A missing-symbol fixture fails to
load before enumeration; its unmapped program headers must never be passed to
the callback. A valid fixture is visible until unloaded. The test reads every
returned program header, checks early-stop return propagation, and enumerates
while another thread repeatedly loads and unloads the fixture.

Android enumeration holds the existing recursive loader lock, reports linked
mappings only, and advertises only the implemented dl_phdr_info prefix. Host
libraries are then enumerated by the host loader. This is not an atomic combined
snapshot of both loaders. Unloading the current module from inside the callback
and cross-loader lock-order stress need further review. Callback arguments are
only valid during the callback. AI-assisted implementation and tests.
