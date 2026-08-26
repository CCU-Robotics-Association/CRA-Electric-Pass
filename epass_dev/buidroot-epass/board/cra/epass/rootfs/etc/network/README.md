# 网络接口配置

本目录会合并到设备上的：

```text
/etc/network/
```

## 当前网络参数

```text
lo    127.0.0.1 回环接口
usb0  192.168.137.2/24 静态地址
网关  192.168.137.1
```

`usbctl rndis` 建立 `rndis.usb0` Gadget 并调用：

```sh
/sbin/ifup -a
```

之后本文件把设备端 `usb0` 配置为 `192.168.137.2`。`192.168.137.1` 是 Windows 网络共享常见的电脑端地址，但仍需要电脑端 RNDIS 驱动和对应静态地址或 Internet Connection Sharing 配置。

## 文件内容说明

```text
auto lo
iface lo inet loopback
```

表示启动回环接口。

```text
auto usb0
iface usb0 inet static
```

表示自动配置 `usb0` 并使用固定 IPv4 地址。`network` 和 `broadcast` 字段与 `/24` 掩码一致。
