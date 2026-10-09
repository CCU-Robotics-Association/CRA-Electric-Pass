# Local IPC compatibility ABI

`ipc_v1.h` describes the native ABI currently used by `drm_app_neo` on the
device.  It exists here so CRA auxiliary programs share one reviewed definition
instead of importing headers from another repository.

This is intentionally called a **compatibility ABI**, not a portable wire
protocol.  Messages contain native 32-bit integers and C structure layout and
are transported as individual `SOCK_SEQPACKET` records.  Both peers must use the
same 32-bit little-endian ABI.  A future version should use an explicitly
serialized header and payload before compatibility with other architectures is
claimed.
