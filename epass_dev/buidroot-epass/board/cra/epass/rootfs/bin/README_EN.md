# Device Utility Programs

Files in this directory are installed into, or override files under:

```text
/bin/
```

on the target device. The directory contains a mixture of POSIX/BusyBox shell scripts and ARM binaries that draw directly to the device framebuffer.

## Startup and Login

### `autologin`

The script ultimately executes:

```sh
exec /bin/login -f root
```

`/etc/inittab` passes it to `getty` on `tty0`, so the local main console does not ask for the root password. After login, the root shell reads `/root/.profile`, which continues the main-program startup sequence.

## Backlight Control

### `brightness_down` and `brightness_up`

These two scripts read and write:

```text
/sys/class/backlight/backlight/brightness
```

They use `bc` to subtract or add 1, then write the result back to sysfs through `tee`. They do not read `max_brightness` or enforce lower and upper bounds, so callers must avoid out-of-range values.

## Storage Maintenance

### `format_sd`

This script always treats:

```text
/dev/mmcblk0
```

as the SD card. Its procedure is:

1. Check whether the block device exists.
2. Ask the user to enter `1` for confirmation.
3. Unmount the old partition and `/sd`.
4. Use `fdisk` to clear the partition table.
5. Create one FAT32 LBA primary partition spanning the available space.
6. Format `/dev/mmcblk0p1` with `mkdosfs -F 32`.
7. Mount it at `/sd`, then create `/tmp/sd_mounted` and `/sd/assets/`.
8. Restart USB MTP mode if MTP is currently running.

This is a destructive operation that erases all data on the target card. It is intended only for a physical Electric Pass whose device-node mapping has already been verified. Do not run it on a development computer or an unknown Linux device.

The script currently creates `/sd/assets/`, but the message written to `/sd/README.txt` refers to `/assets/`. Those paths have different meanings.

The mount-failure branch currently uses a top-level `return 1`. Because `format_sd` normally runs as a standalone script, `exit 1` would be more robust. This issue is documented here but has not been changed in the current revision.

### `mount_boot`

The script executes:

```sh
ubiattach -m 1
mount -t ubifs ubi1:boot /boot
```

It attaches MTD partition 1 as a UBI device, then mounts the UBIFS volume named `boot`. The MTD number, UBI number, and volume name are all tied to this project's NAND partition layout.

## Memory Check

### `memcheck`

This script reads `MemTotal` from `/proc/meminfo`. If the value is below `46080 KiB`, it warns that the device may contain an F1C100s with only 32 MiB of RAM presented as an F1C200s, then waits for 10 seconds.

This is an empirical threshold, not a hardware-level chip identification. Normal kernel reservations also affect the amount of visible memory.

## USB Mode Control

### `usbctl`

`usbctl` assembles a USB Gadget through Linux ConfigFS and supports:

| Command | Function |
| --- | --- |
| `usbctl mtp` | Start uMTP Responder file transfer |
| `usbctl serial` | Create a USB ACM serial interface and start `getty` |
| `usbctl rndis` | Create the RNDIS network interface and run `/sbin/ifup -a` |
| `usbctl epass` / `usbctl responder` | Create the custom FunctionFS interface and start `usb_responder` |
| `usbctl none` / `usbctl stop` | Stop related daemons and remove the Gadget configuration |
| `usbctl start` | Compatibility entry point equivalent to starting MTP |

The displayed USB string is `Electric Pass`. In MTP mode, the script selects `umtprd_sd.conf` or `umtprd_nosd.conf` according to the presence of `/tmp/sd_mounted`, then copies it to the runtime configuration path `/etc/umtprd/umtprd.conf`.

## Random Shutdown Message Programs

`shutdown_message*` consists of three image variants of the same ARM framebuffer utility. Each binary embeds a 360×129 RGB888 bitmap:

| Program | Current Text |
| --- | --- |
| `shutdown_message` | 要走了吗，不再看看 (“Leaving already? Won't you stay a little longer?”) |
| `shutdown_message_2` | 再见，祝愿未来 (“Goodbye, and best wishes for the future.”) |
| `shutdown_message_3` | 别忘记这里 (“Don't forget this place.”) |

### Call Relationship

`randomly_show_shutdown_message` in `/root/.profile` enters the message branch with a 20% probability, then selects one of these three absolute paths with equal probability:

```text
/bin/shutdown_message
/bin/shutdown_message_2
/bin/shutdown_message_3
```
