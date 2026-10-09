# epassctl

`epassctl` is the CRA-maintained command-line client for the Electric Pass main
display process.  This implementation was written for the monorepo and talks to
`/tmp/epass_drm_app.sock` using the current native compatibility ABI.

## Build

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Buildroot installs the program as `/usr/bin/epassctl`.

## Usage

```text
epassctl [json] <module> <operation> [arguments...]
```

Run `epassctl help` for the current module list.  Supported functions are based
on request handlers that actually exist in the bundled main application:

- UI warning and screen query/switching
- theme status, switching, information, automatic-switch lock, and rescan
- settings query and read-modify-write update
- media path query and update
- overlay transition scheduling
- controlled main-process exit

`prts` is retained as a compatibility alias for the clearer `theme` module.
Commands from older clients that have no handler in the current main process
are deliberately not advertised.

For host-side testing, `CRA_EPASS_SOCKET` can override the socket path.

## Compatibility note

The v1 interface mirrors the device's existing native C ABI.  It is suitable
for the current 32-bit target but should not be treated as a network protocol.
See `../protocol/README.md`.
