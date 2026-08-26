# System Configuration Overrides

This directory corresponds to:

```text
/etc/
```

on the target device.

## `cedarx.conf`

Its current contents are:

```ini
[paramter]
log_level = 6
```

## `inittab`

This project uses BusyBox init. During startup it primarily:

1. Mounts `/proc` and remounts the root filesystem read-write.
2. Creates `/dev/pts`, `/dev/shm`, and `/run/lock/subsys`.
3. Runs `/bin/mount -a`, enables swap, and creates the standard input/output links.
4. Sets the hostname from `/etc/hostname`.
5. Runs `/etc/init.d/rcS`.
6. Starts automatic root login on `tty0`.
7. Keeps a serial `getty` on `ttyS0`.
8. Runs `rcK`, disables swap, and unmounts filesystems during shutdown.

The key login line is:

```text
tty0::respawn:/sbin/getty -L tty0 0 vt100 -n -l /bin/autologin
```

After `/bin/autologin` logs in as root, the shell loads `/root/.profile`. The resulting main-program startup chain is therefore `inittab → autologin → .profile → epass_drm_app`.
