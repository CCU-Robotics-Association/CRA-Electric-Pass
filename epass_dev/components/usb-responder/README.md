# CRA USB responder

This component is CRA's FunctionFS service for host-to-device management.  It
is a new implementation of the version-1 Electric Pass USB framing contract;
it does not import the deleted standalone repository.

## Capabilities

- FunctionFS vendor interface with full-speed and high-speed bulk endpoints
- WinUSB compatibility descriptor and a CRA-specific interface GUID
- streaming frame reassembly with a fixed 16 KiB kernel I/O request size
- CRC32 and strict little-endian frame validation
- key/value request encoding
- atomic uploads through a temporary file followed by `rename`
- download, list, stat, recursive delete, rename, and directory creation
- storage-intent validation for NAND and `/sd`
- bounded shell command execution with separate stdout/stderr, timeout, and
  process-group termination
- device, kernel, application, and filesystem information

Relative paths are required and `.`/`..` traversal components are rejected.
Existing paths are canonicalized against the configured service root.  SD
writes on the device are refused unless `/sd` is backed by an MMC mount.

## Build and test

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The installed executable remains `/usr/bin/usb_responder`, so the existing
`usbctl epass` integration does not need a command-name change.

```text
usb_responder --ffs /dev/ffs-epass [-v]
  [--timeout-ms N] [--max-stdout N] [--max-stderr N] [--no-command]
```

Command execution remains enabled by default for protocol compatibility.  A
deployment that only needs file transfer can disable it with `--no-command`.

## Compatibility contract

Frames use a 24-byte little-endian header: `EPAS` magic, protocol version,
message type, flags, request ID, payload length, and payload CRC32.  Supported
message numbers remain 1/2/3, 10 through 18, 20/21, and 30.  Protocol tests
exercise frame corruption rejection, KV round trips, file lifecycle operations,
and path traversal rejection.
