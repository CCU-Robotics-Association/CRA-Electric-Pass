<div align="center">

# Buildroot System Configuration

</div>

**Read this in other languages:** [English](README_EN.md) · [中文](README.md)

> [!NOTE]
> `system/` is Buildroot 2020.02.7's generic system-policy layer. It defines the base root filesystem skeleton, init and device management choices, accounts, consoles, permissions, and post-processing hooks.

<p align="center">
  <a href="#directory-layout">Layout</a> ·
  <a href="#place-in-the-build">Build flow</a> ·
  <a href="#file-reference">Files</a> ·
  <a href="#base-root-filesystem-skeleton">Skeleton</a> ·
  <a href="#current-cra-electric-pass-configuration">CRA configuration</a> ·
  <a href="#development-guidelines">Development</a>
</p>

---

> [!IMPORTANT]
> Most of this directory is upstream Buildroot infrastructure. CRA-specific behavior should normally live in `board/cra/epass/`, a project package, or the board rootfs overlay rather than in this shared skeleton.

## Directory layout

```text
system/
├── Config.in
├── system.mk
├── device_table.txt
├── device_table_dev.txt
└── skeleton/
    ├── dev/        standard descriptor symlinks
    └── etc/        accounts, profile, hosts, services, and runtime symlinks
```

Paths shown as links in Git must remain symbolic links, not small regular files containing the link target.

## Place in the build

The default root filesystem is assembled approximately as follows:

1. `package/skeleton-init-common` copies `system/skeleton/` into `output/target/`.
2. The selected init system contributes its own skeleton.
3. packages install target files.
4. `BR2_ROOTFS_OVERLAY` directories are applied in order.
5. post-build scripts run.
6. users, permissions, and device tables are applied under fakeroot.
7. selected rootfs formats are packaged.
8. post-image scripts create final deployable images.

The system skeleton is therefore a minimal common baseline that later packages and board overlays may extend or replace.

## File reference

### `Config.in`

Defines the **System configuration** menu: skeleton selection, hostname and issue text, password hashing and root password, init and `/dev` management, permission/device tables, merged `/usr`, shell and getty settings, writable-root behavior, network defaults, locale/timezone data, overlays, and post-build/fakeroot/image hooks.

### `system.mk`

Provides shared Make logic for `/usr` layout, skeleton copying, architecture-specific library links, getty values, and root remount behavior. It is included early by the top-level build and affects every target, so board-local requirements should not be placed here casually.

### `device_table.txt`

The generic fakeroot permission table sets modes and ownership for directories and regular files such as `/tmp`, `/root`, `/etc/shadow`, and `/etc/passwd`. Board-specific permissions should normally be added through a separate table referenced by the defconfig.

### `device_table_dev.txt`

Defines static character and block device nodes using:

```text
<path> <type> <mode> <uid> <gid> <major> <minor> <start> <increment> <count>
```

The CRA target uses dynamic eudev device management, so this table is not used to pre-create its complete `/dev` tree.

## Base root filesystem skeleton

`skeleton/dev/` provides `/dev/fd`, `/dev/stdin`, `/dev/stdout`, and `/dev/stderr` links to `/proc/self/fd`. `skeleton/etc/` supplies the base account databases, shell profile, hosts, protocol/service data, and runtime links for `mtab` and `resolv.conf`.

The skeleton's root password entry is only a placeholder. Buildroot rewrites `/etc/shadow` during target finalization according to `BR2_TARGET_GENERIC_ROOT_PASSWD`.

## Current CRA Electric Pass configuration

The active configuration is `board/cra/epass/cra_epass_defconfig`:

```text
BR2_TARGET_GENERIC_HOSTNAME="epass"
BR2_TARGET_GENERIC_ISSUE="Welcome to CRA Electric Pass"
BR2_ROOTFS_DEVICE_CREATION_DYNAMIC_EUDEV=y
BR2_TARGET_GENERIC_ROOT_PASSWD="toor"
BR2_ROOTFS_OVERLAY="board/allwinner/generic/rootfs board/allwinner/suniv-f1c100s/rootfs board/cra/epass/rootfs"
BR2_ROOTFS_POST_IMAGE_SCRIPT="board/cra/epass/scripts/mknanduboot.sh board/cra/epass/scripts/mkdt.sh board/cra/epass/scripts/buildimage.sh"
```

It uses the default Buildroot skeleton, BusyBox init, devtmpfs + eudev, ordered Allwinner/SUNIV/CRA overlays, and three post-image scripts.

> [!WARNING]
> The development password `toor` is stored in plaintext in the defconfig. It may be acceptable for controlled development, but not for a public or production device. Production images should use a strong hash, disable password login, or use key-based access, and should review Dropbear root-login policy.

## Windows symbolic-link caveat

Git records the descriptor links and `etc/mtab`/`etc/resolv.conf` with mode `120000`. A Windows checkout may restore them as ordinary files containing only the target path. Building from such a tree can produce broken stdin, mount-table, or DNS behavior. Use a Linux checkout that preserves symbolic links, LF line endings, and executable bits.

## Syntax and comments

| File | Language | Convention |
| --- | --- | --- |
| `Config.in` | Kconfig | `#` comments; preserve indentation below `help` |
| `system.mk` | GNU Make | `#` comments; recipe commands require tabs |
| `device_table*.txt` | `makedevs` table | `#` comments |
| `skeleton/etc/profile*` | POSIX Shell | `#` comments |
| `passwd`, `group`, `shadow` | Colon-delimited databases | Do not add explanatory comments casually |
| `protocols`, `services` | Whitespace-delimited databases | `#` comments |

## Development guidelines

- Prefer the CRA defconfig, overlay, board tables, and packages for project-specific behavior.
- Assess security implications before changing accounts, passwords, permissions, or device nodes.
- Do not hard-code dynamically managed `/dev` nodes into the static table.
- Check overwrite order when changing rootfs overlays.
- Keep `output/target/` distinct from deployable files in `output/images/`.
- Preserve the syntax and indentation rules of Kconfig, Make, Shell, and `makedevs`.
- Preserve symbolic links and executable bits in a Linux build tree.

---

<div align="center">

<sub><b>system/</b> · generic root filesystem and system policy for Buildroot</sub>

</div>
