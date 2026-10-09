# CRA Device Config

`cra_device_config` manages the v0.6 device-tree overlays selected in
`/boot/uEnv.txt`.  It is a new CRA implementation and has no dependency on the
former AnbUI-based configuration repository.

## Design

- targets the only supported CRA hardware profile: Electric Pass v0.6
- reads device metadata from `/dev/mtdblock0` for display information
- presents a small ANSI/termios interface suitable for the device console
- also provides deterministic `list`, `show`, `enable`, and `disable` commands
- preserves every unrelated `uEnv.txt` line and unknown overlay token
- writes through a same-directory temporary file, `fsync`, and atomic `rename`
- enforces pin conflicts and extension dependencies
- only exposes overlays that are present in this repository

The obsolete `lsm6ds3_pre0.4` overlay is recognized so an existing token is not
silently discarded, but it cannot be newly enabled on v0.6 because PE2 is used
for power-off control.

## Usage

```text
cra_device_config [--uenv PATH]
cra_device_config [--uenv PATH] list
cra_device_config [--uenv PATH] show
cra_device_config [--uenv PATH] enable <overlay>
cra_device_config [--uenv PATH] disable <overlay>
```

With no command, the interactive screen opens.  Use arrow keys or `j`/`k` to
move, Enter/Space to toggle, `S` to save, and `Q` to exit.  A reboot is required
before U-Boot applies changed overlays.

For tests, `CRA_UENV_PATH` can override the default file.

## Build and test

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
