# uMTP Responder 配置模板

本目录对应设备上的：

```text
/etc/umtprd/
```

它保存两份 uMTP Responder 模板。设备进入 MTP 模式时，`/bin/usbctl` 会根据 SD 卡状态选择其中一份，复制为运行时读取的：

```text
/etc/umtprd/umtprd.conf
```

## 两份模板的区别

`umtprd_nosd.conf` 暴露：

```text
/         → rootfs
/root     → main_app
/assets   → assets
/app      → app
```

`umtprd_sd.conf` 在此基础上增加：

```text
/sd       → sd
```

所有入口当前均标记为 `rw`，电脑可通过 MTP 修改相应设备路径。特别是 `rootfs` 和 `main_app` 入口权限很大，连接不可信电脑时存在误删系统文件或主程序的风险。

## 选择逻辑

`usbctl mtp` 检查：

```text
/tmp/sd_mounted
```

- 标记存在：复制 `umtprd_sd.conf`；
- 标记不存在：复制 `umtprd_nosd.conf`。

该标记由 `/root/.profile` 的 SD 挂载逻辑或 `format_sd` 创建。它只是运行时状态文件，不是实际挂载检查的唯一真相。

## USB 身份与协议参数

当前配置包括：

| 项目 | 无 SD 模板 | 有 SD 模板 |
| --- | --- | --- |
| manufacturer | `CCU Robotics Association` | `CCU Robotics Association` |
| product | `Electronic Pass` | `Electronic Pass(SD)` |
| serial | `CRAEPASS` | `CRAEPASS` |
| interface | `MTP` | `MTP` |

两份模板使用 FunctionFS 端点：

```text
/dev/ffs-mtp/ep0
/dev/ffs-mtp/ep1
/dev/ffs-mtp/ep2
/dev/ffs-mtp/ep3
```

这些路径需要与 `usbctl` 创建和挂载的 `ffs.mtp` 功能保持一致。
