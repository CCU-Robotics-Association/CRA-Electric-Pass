# 系统配置覆盖

本目录对应目标设备的：

```text
/etc/
```

## `cedarx.conf`

当前内容为：

```ini
[paramter]
log_level = 6
```

## `inittab`

本项目使用 BusyBox init。启动时主要执行：

1. 挂载 `/proc`，并把根文件系统重新挂载为可写；
2. 创建 `/dev/pts`、`/dev/shm` 和 `/run/lock/subsys`；
3. 执行 `/bin/mount -a`、启用 swap，并建立标准输入输出链接；
4. 从 `/etc/hostname` 设置主机名；
5. 执行 `/etc/init.d/rcS`；
6. 在 `tty0` 启动自动 root 登录；
7. 在 `ttyS0` 保留串口 `getty`；
8. 关机时执行 `rcK`、关闭 swap 并卸载文件系统。

关键登录行是：

```text
tty0::respawn:/sbin/getty -L tty0 0 vt100 -n -l /bin/autologin
```

`/bin/autologin` 登录 root 后会加载 `/root/.profile`，因此 `inittab → autologin → .profile → epass_drm_app` 构成主程序的启动链。
