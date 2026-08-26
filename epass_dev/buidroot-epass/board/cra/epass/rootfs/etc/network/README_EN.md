# Network Interface Configuration

This directory is merged into:

```text
/etc/network/
```

on the device.

## Current Network Parameters

```text
lo       127.0.0.1 loopback interface
usb0     192.168.137.2/24 static address
gateway  192.168.137.1
```

`usbctl rndis` creates the `rndis.usb0` Gadget and runs:

```sh
/sbin/ifup -a
```

The configuration then assigns `192.168.137.2` to the device-side `usb0` interface. `192.168.137.1` is a common host address for Windows network sharing, but the computer still requires an RNDIS driver and the corresponding static address or Internet Connection Sharing configuration.

## File Contents

```text
auto lo
iface lo inet loopback
```

This brings up the loopback interface.

```text
auto usb0
iface usb0 inet static
```

This automatically configures `usb0` with a fixed IPv4 address. The `network` and `broadcast` fields are consistent with the `/24` subnet mask.
