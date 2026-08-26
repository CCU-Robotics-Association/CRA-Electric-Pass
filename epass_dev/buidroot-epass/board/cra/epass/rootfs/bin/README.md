# 设备辅助程序

本目录中的文件会覆盖或安装到目标设备的：

```text
/bin/
```

这里混合了 POSIX/BusyBox Shell 脚本和面向设备帧缓冲的 ARM 二进制程序。

## 启动与登录

### `autologin`

脚本最终执行：

```sh
exec /bin/login -f root
```

`/etc/inittab` 把它交给 `tty0` 上的 `getty`，因此本地主控制台不要求输入 root 密码。登录后 root Shell 会读取 `/root/.profile`，进而进入主程序启动流程。

## 背光控制

### `brightness_down` 与 `brightness_up`

两个脚本读写：

```text
/sys/class/backlight/backlight/brightness
```

它们借助 `bc` 做一次减 1 或加 1 运算，再通过 `tee` 写回 sysfs。脚本没有读取 `max_brightness`，也没有对最小值和最大值做边界限制；调用方需要避免越界。

## 存储维护

### `format_sd`

该脚本固定把：

```text
/dev/mmcblk0
```

视为 SD 卡。执行流程为：

1. 检查块设备是否存在；
2. 请求键盘输入 `1` 进行确认；
3. 卸载旧分区和 `/sd`；
4. 使用 `fdisk` 清空分区表；
5. 建立一个占满可用空间的 FAT32 LBA 主分区；
6. 使用 `mkdosfs -F 32` 格式化 `/dev/mmcblk0p1`；
7. 挂载到 `/sd`，创建 `/tmp/sd_mounted` 与 `/sd/assets/`；
8. 如果 MTP 正在运行，则重启 USB MTP 模式。

这是破坏性操作，会删除目标卡上的全部数据。脚本只适用于已经确认设备节点映射的实体通行证，不应在开发电脑或未知 Linux 设备上执行。

当前脚本实际创建 `/sd/assets/`，而写入 `/sd/README.txt` 的提示写成了 `/assets/`；两者含义不同。

挂载失败分支当前使用顶层 `return 1`。由于 `format_sd` 通常作为独立脚本执行，更稳妥的退出方式是 `exit 1`；该问题本轮只记录、未修改。

### `mount_boot`

脚本执行：

```sh
ubiattach -m 1
mount -t ubifs ubi1:boot /boot
```

它把 MTD 分区 1 附加为 UBI 设备，再挂载名为 `boot` 的 UBIFS 卷。MTD 编号、UBI 编号和卷名都与本项目 NAND 分区布局绑定。

## 内存检查

### `memcheck`

脚本从 `/proc/meminfo` 读取 `MemTotal`。低于 `46080 KiB` 时，它认为设备可能使用只有 32 MiB RAM 的 F1C100s 冒充 64 MiB F1C200s，输出警告并等待 10 秒。

该检查是经验性阈值，不是芯片身份的硬件鉴定；正常的内核保留区也会影响可见内存。

## USB 模式控制

### `usbctl`

`usbctl` 使用 Linux ConfigFS 组装 USB Gadget，支持：

| 命令 | 功能 |
| --- | --- |
| `usbctl mtp` | 启动 uMTP Responder 文件传输 |
| `usbctl serial` | 建立 USB ACM 串口并启动 `getty` |
| `usbctl rndis` | 建立 RNDIS 网卡并执行 `/sbin/ifup -a` |
| `usbctl epass` / `usbctl responder` | 建立自定义 FunctionFS 接口并启动 `usb_responder` |
| `usbctl none` / `usbctl stop` | 停止相关守护程序并拆除 Gadget |
| `usbctl start` | 兼容入口，等价于启动 MTP |

字符串为 `Electric Pass`。MTP 模式会根据 `/tmp/sd_mounted` 是否存在，选择 `umtprd_sd.conf` 或 `umtprd_nosd.conf` 并复制为运行时配置 `/etc/umtprd/umtprd.conf`。

## 随机关机提示程序

`shutdown_message*` 是同一 ARM 帧缓冲辅助程序的三个图像变体，内嵌 360×129 RGB888 位图：

| 程序 | 当前文字 |
| --- | --- |
| `shutdown_message` | 要走了吗，不再看看 |
| `shutdown_message_2` | 再见，祝愿未来 |
| `shutdown_message_3` | 别忘记这里 |

### 调用关系

`/root/.profile` 中的 `randomly_show_shutdown_message` 会以 20% 的概率进入提示分支，再从以下三个绝对路径中等概率选择一个执行：

```text
/bin/shutdown_message
/bin/shutdown_message_2
/bin/shutdown_message_3
```