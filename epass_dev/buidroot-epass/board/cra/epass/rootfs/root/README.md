# root 用户主目录覆盖

本目录中的文件会进入目标设备的：

```text
/root/
```

它保存 root 登录后的启动逻辑、终端字符 Logo 和主程序固定资源。CRA Electric Pass 主程序本体 `/root/epass_drm_app` 不存放在这里的源码树中，而是由 Buildroot 的 `epass_drm_app` 软件包在构建时安装。

## `.profile` 的进入条件

`/etc/inittab` 在 `tty0` 上通过 `/bin/autologin` 登录 root，登录 Shell 随后读取 `.profile`。

如果当前终端不是 `/dev/tty0`，脚本只显示 CRA 调试 Shell 欢迎语后返回，不启动全屏主程序：

```text
串口、USB ACM 等调试终端
    → 保留交互 Shell

/dev/tty0 本地控制台
    → 执行完整设备启动流程
```

## 主启动流程

`.profile` 在 `tty0` 上依次：

1. 调用 `memcheck` 检查可见内存；
2. 尝试把 `/dev/mmcblk0p1` 挂载到 `/sd`；
3. 用 `/tmp/sd_mounted` 记录挂载结果；
4. 检查当前目录中的 `epass_drm_app`；
5. 缺少主程序时启动 `usbctl mtp`，供电脑传文件；
6. 主程序存在时显示 `logo.txt`、程序版本和 `/etc/os-release`；
7. 循环启动主程序，并根据退出码执行系统动作。

`.profile` 使用相对路径 `./epass_drm_app`，依赖登录后的当前目录为 `/root`。

## 主程序退出码协议

| 退出码 | `.profile` 的动作 |
| --- | --- |
| `0` 或其他未处理值 | 结束监督循环 |
| `1` | 重新挂载 SD 卡，等待 2 秒后重启主程序 |
| `2` | 执行主程序生成的 `/tmp/appstart` 扩展应用脚本，结束后重启主程序 |
| `3` | 按概率尝试显示一条关机提示，然后调用 `poweroff` |
| `4` | 清空输入、执行 `format_sd`，结束后重启主程序 |
| `5` | 挂载 boot 卷并进入 `srgn_config`，结束后重启主程序 |

该表必须和 `drm_app_neo/src/config.h` 中的 `EXITCODE_*` 定义保持一致。

## 随机关机提示

`.profile` 中的 `randomly_show_shutdown_message` 以 20% 的概率显示关机提示，并从以下三个程序中等概率选择一个：

```text
/bin/shutdown_message
/bin/shutdown_message_2
/bin/shutdown_message_3
```

函数名、调用路径与 `/bin/` 中的实际文件名已经统一。函数内使用 BusyBox `ash` 提供的 `$RANDOM` 生成分支值。

## 运行时生成文件

以下文件不属于本 overlay 的静态内容：

| 文件 | 生成者 |
| --- | --- |
| `/root/epass_drm_app` | Buildroot 软件包安装 |
| `/root/asset.log` | 主题扫描器 |
| `/root/apps.log` | 应用扫描器 |
| `/tmp/appstart` | 主程序的扩展应用启动逻辑 |
| `/tmp/sd_mounted` | `.profile` 或 `format_sd` |
