# CRA Electric Pass Hardware Interface Overlays

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the Linux hardware interface overlays for CRA Electric Pass. They enable F1C200S controllers on demand, select the corresponding GPIO pin functions, and switch the USB operating mode.

The `interface` layer provides buses and hardware interfaces, while the `ext` layer declares the specific external devices connected to them. The current project targets the Shirogane v0.6 hardware revision only.

## Files

| File | Target Controller | Purpose | Pins Used |
| --- | --- | --- | --- |
| `adc_pa1.dts` | RTP/GPADC | Adds the PA1 ADC pin while retaining the default PA0 pin | PA0, PA1 |
| `adc_pa123.dts` | RTP/GPADC | Enables all four ADC pins | PA0, PA1, PA2, PA3 |
| `i2c0.dts` | I²C0 | Enables the hardware I²C0 controller | PD0, PD12 |
| `i2s0_pa.dts` | I²S0 | Enables I²S0 with the PA pin layout | PE3, PA1, PA2, PA3 |
| `i2s0_pe.dts` | I²S0 | Enables I²S0 with the PE pin layout | PE3, PE5, PE6, PA1 |
| `spi1.dts` | SPI1 | Enables SPI1 and the predefined Spidev child device | PE7, PE8, PE9, PE10 |
| `uart1.dts` | UART1 | Enables UART1 TX/RX | PA2, PA3 |
| `uart2.dts` | UART2 | Enables UART2 TX/RX | PE7, PE8 |
| `usbhost.dts` | USB OTG | Forces the USB controller into host mode | Dedicated USB pins |
| `usbhs.dts` | USB OTG | Requests the project-specific USB High-Speed mode | Dedicated USB pins |

## Relationship to the Other Device Tree Directories

The Linux device tree is assembled in the following order:

```text
Linux suniv-f1c100s.dtsi
        ↓
base/epass.dtsi
        ↓
base/devicetree.dts
        ↓
screen overlay
        ↓
interface overlay
        ↓
ext overlay
        ↓
U-Boot starts Linux
```

Each directory has a distinct role:

- `base/` describes the core hardware that is always present and predefines the pin groups and default states of optional controllers.
- `screen/` selects the installed LCD panel and its display timing.
- `interface/` enables SoC controllers, selects pin multiplexing, or changes a controller's operating mode.
- `ext/` declares specific devices such as CardKB, ES8311, and LSM6DS3 on buses that have already been enabled.

For example:

```text
interface=i2c0
ext=cardkb
```

Here, `i2c0` enables the hardware I²C0 controller, while `cardkb` declares a CardKB at address `0x5f` on that bus.

## Basic Overlay Structure

Every file in this directory is a Device Tree Overlay:

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@1 {
        target = <&some_controller>;
        __overlay__ {
            status = "okay";
        };
    };
};
```

The main elements are:

- `/dts-v1/;` declares the device tree source format.
- `/plugin/;` declares that the file is an overlay applied to a base device tree.
- `fragment@1` defines an overlay fragment. The number only distinguishes it from other fragments.
- `target` identifies the base device tree node to modify by referencing its label.
- `__overlay__` contains the properties to add or override.
- `status = "okay"` enables a controller that is disabled by default in the base device tree.
- `pinctrl-0` selects the controller's default pin group.
- `pinctrl-names = "default"` identifies `pinctrl-0` as the default pin state.

## `adc_pa1.dts`

### Purpose

```dts
fragment@1 {
    target = <&rtp>;
    __overlay__ {
        pinctrl-0 = <&rtp_pins_01>;
    };
};
```

The F1C200S RTP controller was originally intended for resistive touch panels, but it also provides GPADC functionality.

The base device tree enables it with:

```dts
&rtp {
    status = "okay";
    pinctrl-0 = <&rtp_pins_0>;
    pinctrl-names = "default";
};
```

The default pin group is:

```dts
rtp_pins_0: rtp-pins-0 {
    pins = "PA0";
    function = "rtp";
};
```

PA0 is therefore already configured for RTP/ADC use when no optional ADC interface overlay is enabled.

`adc_pa1.dts` replaces that pin group with:

```dts
rtp_pins_01: rtp-pins-01 {
    pins = "PA0", "PA1";
    function = "rtp";
};
```

Its actual effect is:

```text
Retain PA0 and additionally enable PA1
```

The `pa1` suffix refers to the additional PA1 pin; it does not mean that PA1 is used alone.

### Linux Driver

The relevant kernel configuration options are:

```text
CONFIG_MFD_SUN4I_GPADC=y
CONFIG_SUN4I_GPADC=y
```

The project also includes:

```text
board/cra/epass/patch/linux/0000-f1c100s-gpadc-regs.patch
board/cra/epass/patch/linux/0005-gpadc-low-freq.patch
```

The first patch adjusts the F1C100S/F1C200S GPADC register bit definitions. The second changes the ADC sampling frequency and filtering configuration.

### Conflicts

PA1 is also used by both I²S0 pin layouts:

```text
adc_pa1 + i2s0_pa = conflict on PA1
adc_pa1 + i2s0_pe = conflict on PA1
```

UART1 uses PA2 and PA3, so `adc_pa1` does not directly conflict with `uart1`.

## `adc_pa123.dts`

### Purpose

```dts
fragment@1 {
    target = <&rtp>;
    __overlay__ {
        pinctrl-0 = <&rtp_pins>;
    };
};
```

This overlay selects the complete RTP pin group from the common SoC device tree:

```dts
rtp_pins: rtp-pins {
    pins = "PA0", "PA1", "PA2", "PA3";
    function = "rtp";
};
```

The resulting pin set is:

```text
PA0, PA1, PA2, PA3
```

The name `adc_pa123` means that PA1, PA2, and PA3 are added to the default PA0 pin.

### Conflicts

This overlay occupies the entire PA0–PA3 range and therefore has a wider set of conflicts:

```text
adc_pa123 + i2s0_pa = conflict on PA1, PA2, and PA3
adc_pa123 + i2s0_pe = conflict on PA1
adc_pa123 + uart1   = conflict on PA2 and PA3
```

`adc_pa1` and `adc_pa123` must not be used together. Both modify the `pinctrl-0` property of `&rtp`; the later overlay replaces the earlier value rather than merging the pin groups.

## `i2c0.dts`

### Purpose

```dts
fragment@1 {
    target = <&i2c0>;
    __overlay__ {
        status = "okay";
    };
};
```

The base device tree already provides:

```dts
&i2c0 {
    pinctrl-names = "default";
    pinctrl-0 = <&i2c0_pd_pins>;
    status = "disabled";
};
```

The selected pins are:

```text
PD0  = SDA
PD12 = SCL
```

PD0 and PD12 are deliberately absent from the RGB LCD pin group, so hardware I²C0 can operate while the display is active.

### Linux Driver

The relevant kernel configuration options are:

```text
CONFIG_I2C_CHARDEV=y
CONFIG_I2C_MV64XXX=y
```

Once enabled, Linux will normally expose:

```text
/dev/i2c-0
```

Confirm the final number from `/dev/i2c-*` and the boot log on the physical device.

`i2c0.dts` enables only the controller. It does not declare any particular chip on the bus; those devices require their corresponding `ext` overlays.

### Electrical Requirements

I²C uses open-drain signalling and requires pull-up resistors on SDA and SCL. The current `i2c0_pd_pins` group does not request internal pull-ups, so suitable pull-ups must be provided by the PCB or an external module.

### Conflicts

`ext/es8311_sound.dts` uses the same PD0 and PD12 pins to create a GPIO-driven I²C bus.

The current implementation must not use:

```text
interface=i2c0
ext=es8311_sound
```

Otherwise, two Linux I²C controllers would compete for the same physical pins.

## `i2s0_pa.dts`

### Purpose

```dts
fragment@1 {
    target = <&i2s0>;
    __overlay__ {
        status = "okay";
        pinctrl-0 = <&i2s_pins_pa>;
        pinctrl-names = "default";
    };
};
```

This overlay:

1. Enables the F1C200S I²S0 controller.
2. Selects the PA-oriented I²S0 pin layout.

The pins are:

```text
PE3, PA2, PA3, PA1
```

I²S0 transfers digital audio data only. Enabling it alone does not create a complete ALSA sound card; a codec and sound-card device tree node are still required, for example:

```text
ext=es8311_sound
```

### Linux Driver

The relevant kernel configuration options are:

```text
CONFIG_SND_SOC=m
CONFIG_SND_SUN4I_I2S=m
CONFIG_SND_SIMPLE_CARD=m
```

The I²S clock changes and audio drivers are provided by:

```text
board/cra/epass/patch/linux/0010-i2s-and-es-driver.patch
```

### Conflicts

```text
i2s0_pa + adc_pa1   = conflict on PA1
i2s0_pa + adc_pa123 = conflict on PA1, PA2, and PA3
i2s0_pa + uart1     = conflict on PA2 and PA3
```

Of the two I²S0 layouts, `i2s0_pa` has the wider conflict range.

## `i2s0_pe.dts`

### Purpose

```dts
fragment@1 {
    target = <&i2s0>;
    __overlay__ {
        status = "okay";
        pinctrl-0 = <&i2s_pins_pe>;
        pinctrl-names = "default";
    };
};
```

The selected pins are:

```text
PE3, PE5, PE6, PA1
```

This layout does not use the UART1 pins PA2 and PA3, but it still uses PA1.

### Conflicts

```text
i2s0_pe + adc_pa1   = conflict on PA1
i2s0_pe + adc_pa123 = conflict on PA1
```

`i2s0_pa` and `i2s0_pe` cannot be active layouts at the same time. If both appear in `interface=`, the later overlay replaces `pinctrl-0`, making the final configuration dependent on their order.

## `spi1.dts`

### Purpose

```dts
fragment@1 {
    target = <&spi1>;
    __overlay__ {
        status = "okay";
    };
};
```

The base device tree already provides:

```dts
&spi1 {
    pinctrl-names = "default";
    pinctrl-0 = <&spi1_pins>;
    status = "disabled";

    spidev@0 {
        compatible = "rohm,dh2228fv";
        spi-max-frequency = <80000000>;
        reg = <0>;
    };
};
```

The selected pins are:

```text
PE7, PE8, PE9, PE10
```

`spi1_pins` also enables pull-ups on this pin group.

### Spidev

`spidev@0` means that:

- The device is connected to SPI1.
- It uses chip select 0.
- The device tree permits a maximum clock frequency of 80 MHz.
- Enabling the controller will normally create `/dev/spidev1.0`.

The 80 MHz value is only an upper limit declared by the device tree. It does not guarantee that an external chip, connector, or flying-wire setup will operate reliably at that frequency. Begin validation at a lower frequency.

### `rohm,dh2228fv`

This does not mean that the board contains a Rohm DH2228FV.

Linux discourages declaring a generic userspace SPI device as:

```dts
compatible = "spidev";
```

Doing so produces a `buggy DT` warning. The original project reuses `rohm,dh2228fv`, which is present in the Spidev driver's match table, to create a generic userspace SPI device.

The string is a compatibility workaround, not the model of a physical component.

### Linux Driver

The relevant kernel configuration options are:

```text
CONFIG_SPI=y
CONFIG_SPI_SUN6I=y
CONFIG_SPI_SPIDEV=y
```

### Conflicts

UART2 uses PE7 and PE8:

```text
spi1 + uart2 = conflict on PE7 and PE8
```

If both are enabled, the final device tree marks both controllers as available. The Linux pinctrl subsystem will normally cause one driver to fail when claiming the pins, but the result depends on probe order and must not be relied upon.

## `uart1.dts`

### Purpose

```dts
fragment@1 {
    target = <&uart1>;
    __overlay__ {
        status = "okay";
    };
};
```

The base device tree already specifies:

```dts
&uart1 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart1_pa_pins>;
    status = "disabled";
};
```

The selected pins are:

```text
PA2, PA3
```

This configuration enables only normal TX/RX operation. It does not select the UART1 RTS/CTS pin group.

UART0 already uses PE0 and PE1 as the system console. `uart1.dts` does not replace the boot console; it enables a second serial port.

Once enabled, it will normally appear as:

```text
/dev/ttyS1
```

Confirm the final number from `/dev/ttyS*` and the boot log on the physical device.

### Linux Driver

The relevant kernel configuration options are:

```text
CONFIG_SERIAL_8250=y
CONFIG_SERIAL_8250_CONSOLE=y
CONFIG_SERIAL_8250_NR_UARTS=3
CONFIG_SERIAL_8250_RUNTIME_UARTS=3
CONFIG_SERIAL_8250_DW=y
```

This is a 3.3 V logic-level SoC UART, not a traditional RS-232 interface using positive and negative voltages. Do not connect it directly to an RS-232 port.

### Conflicts

```text
uart1 + adc_pa123 = conflict on PA2 and PA3
uart1 + i2s0_pa   = conflict on PA2 and PA3
```

`uart1` does not directly conflict with `adc_pa1`.

## `uart2.dts`

### Purpose

```dts
fragment@1 {
    target = <&uart2>;
    __overlay__ {
        status = "okay";
    };
};
```

The base device tree specifies:

```dts
&uart2 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart2_pe_pins>;
    status = "disabled";
};
```

The selected pins are:

```text
PE7, PE8
```

Once enabled, it will normally appear as:

```text
/dev/ttyS2
```

Confirm the final number on the physical device.

### Conflicts

SPI1 uses PE7 through PE10, including UART2's PE7 and PE8:

```text
uart2 + spi1 = conflict on PE7 and PE8
```

UART2 does not directly overlap either of the current I²S0 pin layouts.

## `usbhost.dts`

### Purpose

```dts
fragment@1 {
    target = <&usb_otg>;
    __overlay__ {
        dr_mode = "host";
    };
};
```

The base device tree already enables the USB OTG controller, PHY, and OTG SRAM:

```dts
&otg_sram {
    status = "okay";
};

&usb_otg {
    dr_mode = "otg";
    status = "okay";
};

&usbphy {
    status = "okay";
};
```

`usbhost.dts` changes:

```text
Dual-role OTG mode
```

to:

```text
Fixed host mode
```

### Impact on USB Gadget Functions

Host mode is used to connect USB devices such as flash drives and USB-to-serial adapters, but it disables USB Gadget operation.

After enabling `usbhost`, the following PC-facing functions can no longer operate in their current form:

- RNDIS USB networking.
- USB ACM serial Gadget.
- USB Mass Storage Gadget.
- FunctionFS Gadget.

Do not enable `usbhost` if the device still needs to connect to a PC through RNDIS.

This overlay does not configure a 5 V VBUS supply or power switch. If the PCB cannot actively supply VBUS to a USB peripheral, setting `dr_mode = "host"` does not create a 5 V supply by itself.

## `usbhs.dts`

### Purpose

```dts
fragment@1 {
    target = <&usb_otg>;
    __overlay__ {
        srgn,usb-hs-enabled;
    };
};
```

`srgn,usb-hs-enabled` is not a standard Linux device tree property. It is a project-specific boolean property added by the original implementation.

Its intended behavior is:

- Property present: request USB High-Speed operation.
- Property absent: the project patch disables High-Speed and uses Full-Speed.

It does not select the USB host or peripheral role:

```text
usbhost = role selection
usbhs   = speed selection
```

The two overlays can theoretically be selected together:

```text
interface=usbhost usbhs
```

This requests fixed host mode with High-Speed enabled.

### Speed Difference

```text
USB Full-Speed: 12 Mbit/s
USB High-Speed: 480 Mbit/s
```

High-Speed has much stricter requirements for differential routing, impedance, length matching, and signal integrity than Full-Speed.

### Uninitialized Variable in the Kernel Patch

The corresponding kernel patch contains a real C code defect.

It comments out the original initialization:

```c
power = MUSB_POWER_ISOUPDATE;
```

but later performs:

```c
power |= MUSB_POWER_HSENAB;
```

or:

```c
power &= ~MUSB_POWER_HSENAB;
```

The local `power` variable is not initialized before these bitwise operations. Other bits written to the MUSB `POWER` register may therefore come from undefined stack data. Both the default Full-Speed branch and the optional High-Speed branch are affected.

The patch also retains a debug message that is unsuitable for production logs:

```c
printk(KERN_INFO "Conclusion: SRGN SUXX!\n");
```

These are kernel patch issues rather than syntax errors in `usbhs.dts`. Do not enable `usbhs` merely to increase the nominal transfer rate before the patch has been corrected.

### Custom Property Name

`srgn` is the vendor prefix used by the original project. If it is later changed to:

```dts
cra,usb-hs-enabled;
```

the following driver lookup must be changed at the same time:

```c
of_property_read_bool(np, "srgn,usb-hs-enabled")
```

Changing only the device tree or only the driver prevents the property from matching.

## Build Process

`board/cra/epass/scripts/mkdt.sh` processes every `.dts` file in this directory:

```bash
cpp -nostdinc \
    -I "${BUILD_DIR}/linux-5.4.99/include/" \
    -I "${BUILD_DIR}/linux-5.4.99/arch/arm/boot/dts" \
    -P -undef -x assembler-with-cpp

dtc -@ -I dts -O dtb
```

The generated files are placed in:

```text
output/images/dt/interface/adc_pa1.dtbo
output/images/dt/interface/adc_pa123.dtbo
output/images/dt/interface/i2c0.dtbo
output/images/dt/interface/i2s0_pa.dtbo
output/images/dt/interface/i2s0_pe.dtbo
output/images/dt/interface/spi1.dtbo
output/images/dt/interface/uart1.dtbo
output/images/dt/interface/uart2.dtbo
output/images/dt/interface/usbhost.dtbo
output/images/dt/interface/usbhs.dtbo
```

The `-@` option preserves the symbols and fixup information required by overlays, allowing U-Boot to resolve labels such as `&rtp`, `&i2c0`, `&i2s0`, `&spi1`, `&uart1`, `&uart2`, and `&usb_otg` from the base device tree.

`board/cra/epass/scripts/kernel.its` then packages these files into the FIT image using node names of the form:

```text
fdt-iface-<interface-name>
```

For example:

```text
fdt-iface-i2c0
fdt-iface-spi1
fdt-iface-uart1
```

## Selecting Interfaces at Boot

The current project template at `board/cra/epass/uEnv.txt` contains:

```text
interface=
ext=
```

No optional interface or external-device overlay is enabled by default.

U-Boot accepts space-separated interface names:

```text
interface=i2c0 uart1
```

The processing sequence is:

```text
for ov in ${interface}
        ↓
Extract fdt-iface-${ov} from the FIT image
        ↓
Run fdt apply in the listed order
```

Each interface name must exactly match the corresponding FIT node suffix.

U-Boot does not detect pin conflicts. If several overlays modify the same property, a later value may replace an earlier one. If several controllers claim the same physical pins, the final device tree may mark all of them as enabled and leave the conflict to fail during Linux driver probing.

## Dependency and Conflict Summary

| Combination | Conflict or Reason |
| --- | --- |
| `adc_pa1` + `i2s0_pa` | PA1 |
| `adc_pa1` + `i2s0_pe` | PA1 |
| `adc_pa123` + `i2s0_pa` | PA1, PA2, PA3 |
| `adc_pa123` + `i2s0_pe` | PA1 |
| `adc_pa123` + `uart1` | PA2, PA3 |
| `i2s0_pa` + `uart1` | PA2, PA3 |
| `spi1` + `uart2` | PE7, PE8 |
| `i2c0` + `es8311_sound` | PD0 and PD12 are claimed by hardware I²C0 and GPIO-driven I²C |
| `usbhost` + RNDIS/USB Gadget | The USB host role is incompatible with the USB Gadget peripheral role |
| `usbhs` | High-Speed signal-integrity risk on the original board, plus an uninitialized variable in the corresponding kernel patch |

The following overlays are alternative selections and must not be listed together:

```text
adc_pa1 / adc_pa123
i2s0_pa / i2s0_pe
```

## Further Development Guidelines

Before carrying out further development in this directory, verify the hardware schematic, PCB routing, and base device tree:

- Do not assign the same GPIO pins to two controllers.
- When changing `pinctrl-0`, check every node that uses the same PA, PD, or PE pins.
- ADC inputs must remain within the voltage and electrical limits of the F1C200S.
- The UART pins use SoC logic levels and must not be connected directly to a traditional RS-232 interface.
- The SPI `spi-max-frequency` value is an upper limit, not a guarantee of stable operation.
- Before forcing USB host mode, confirm both the VBUS power arrangement and the need for USB Gadget functions.
- Before enabling USB High-Speed, correct the corresponding kernel patch and verify PCB signal integrity.
- When renaming `srgn,usb-hs-enabled`, update the kernel driver at the same time.
- After adding an interface overlay, update `kernel.its`; otherwise, the generated `.dtbo` will not be packaged into the FIT image.
- After adding a controller driver, review `linux.defconfig` and the corresponding kernel patches.
- Before removing an interface overlay, confirm that no external device or older hardware revision still depends on it.
