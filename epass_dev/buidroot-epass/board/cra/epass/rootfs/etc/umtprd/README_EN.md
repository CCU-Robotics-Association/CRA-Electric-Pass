# uMTP Responder Configuration Templates

This directory corresponds to:

```text
/etc/umtprd/
```

on the device. It contains two uMTP Responder templates. When the device enters MTP mode, `/bin/usbctl` selects one according to the SD-card state and copies it to the runtime path:

```text
/etc/umtprd/umtprd.conf
```

## Differences Between the Templates

`umtprd_nosd.conf` exposes:

```text
/         → rootfs
/root     → main_app
/assets   → assets
/app      → app
```

`umtprd_sd.conf` additionally exposes:

```text
/sd       → sd
```

Every entry is currently marked `rw`, allowing a computer to modify the corresponding device path over MTP. The `rootfs` and `main_app` entries in particular grant extensive access; connecting the device to an untrusted computer risks accidental deletion of system files or the main program.

## Selection Logic

`usbctl mtp` checks:

```text
/tmp/sd_mounted
```

- If the marker exists, it copies `umtprd_sd.conf`.
- If the marker does not exist, it copies `umtprd_nosd.conf`.

The marker is created by the SD-mount logic in `/root/.profile` or by `format_sd`. It is only a runtime state file and is not, by itself, definitive proof that the card remains mounted.

## USB Identity and Protocol Parameters

The current configuration contains:

| Item | Without SD | With SD |
| --- | --- | --- |
| manufacturer | `CCU Robotics Association` | `CCU Robotics Association` |
| product | `Electronic Pass` | `Electronic Pass(SD)` |
| serial | `CRAEPASS` | `CRAEPASS` |
| interface | `MTP` | `MTP` |

Both templates use these FunctionFS endpoints:

```text
/dev/ffs-mtp/ep0
/dev/ffs-mtp/ep1
/dev/ffs-mtp/ep2
/dev/ffs-mtp/ep3
```

These paths must remain consistent with the `ffs.mtp` function created and mounted by `usbctl`.
