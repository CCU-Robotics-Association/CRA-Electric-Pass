# CRA Electric Pass External Device Overlays

Read this in other languages: [English](README_EN.md), [中文](README.md).

This directory contains the Linux device tree overlays for optional hardware connected to the CRA Electric Pass board.

These files are applied to the base device tree only when requested during U-Boot startup. The current project targets the Shirogane v0.6 hardware revision only.

## Files

| File | External Device | Bus and Address |
| --- | --- | --- |
| `cardkb.dts` | M5Stack Unit CardKB mini keyboard | Hardware I²C0 at address `0x5f` |
| `es8311_sound.dts` | Everest ES8311 audio codec | GPIO-driven I²C at address `0x18`; audio data is transferred over I²S0 |
| `lsm6ds3_pre0.4.dts` | ST LSM6DS3 six-axis inertial sensor | Hardware I²C0 at address `0x6a`; interrupt on PE2 |

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

- `base/` describes the core hardware that is always present on the board.
- `screen/` selects the installed LCD panel and its timing parameters.
- `interface/` enables SoC controllers and configures pin multiplexing for interfaces such as I²C0, I²S0, SPI1, and UART.
- `ext/` declares the specific external devices connected to buses that have already been enabled.

For example, CardKB is an `ext` device, but it depends on `interface/i2c0.dts` to enable the hardware I²C0 controller first.

## Basic Overlay Syntax

Every file in this directory is a Device Tree Overlay:

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@1 {
        target = <&some_node>;
        __overlay__ {
            /* Properties or nodes to add or override */
        };
    };
};
```

The main elements are:

- `/dts-v1/;` declares the device tree source format.
- `/plugin/;` declares that the file is an overlay that can be applied to a base device tree.
- `fragment@1` defines an overlay fragment. Its number only distinguishes it from other fragments.
- `target` identifies the base device tree node to modify by referencing its label.
- `target-path` identifies the node to modify by its path.
- `__overlay__` contains the properties and child nodes to add or override.
- `compatible` provides the compatibility string that Linux uses to match the device with a driver.
- `reg` specifies the device address on a bus such as I²C.
- `status = "okay"` explicitly enables a node. A newly created node without a `status` property is also treated as available by default.

In the following declaration, the name before the colon is a label and the name after it is the node name:

```dts
es8311: es8311@18
```

Other nodes can refer to this device as `&es8311`. The label is primarily an internal device tree reference and does not necessarily match the device name exposed by Linux.

## `cardkb.dts`

### Purpose

`cardkb.dts` describes an M5Stack Unit CardKB mini keyboard:

```dts
fragment@1 {
    target = <&i2c0>;
    __overlay__ {
        cardkb:cardkb@5f {
            compatible = "m5stack,cardkb";
            reg = <0x5f>;
            polling-interval = <50>;
        };
    };
};
```

### I²C0 Dependency

```dts
target = <&i2c0>;
```

This adds CardKB as a child of the F1C200S hardware I²C0 controller.

I²C0 is disabled by default in the base device tree:

```dts
&i2c0 {
    pinctrl-names = "default";
    pinctrl-0 = <&i2c0_pd_pins>;
    status = "disabled";
};
```

CardKB therefore requires both of the following settings:

```text
interface=i2c0
ext=cardkb
```

Hardware I²C0 uses:

```text
PD0  = SDA
PD12 = SCL
```

### Device Address

```dts
reg = <0x5f>;
```

The 7-bit I²C address of CardKB is `0x5f`. This value should not be changed unless the peripheral firmware actually uses a different address.

### Polling Interval

```dts
polling-interval = <50>;
```

The driver reads the keyboard every 50 ms, giving a polling rate of approximately 20 Hz:

- Decreasing this value reduces input latency but increases I²C traffic and CPU wakeups.
- Increasing this value reduces system overhead but makes key response slower.
- The current 50 ms interval is suitable for ordinary keyboard input.

### Linux Driver

The CardKB driver is added to the Linux kernel by:

```text
board/cra/epass/patch/linux/0009-m5stack-cardkb-driver.patch
```

The relevant kernel configuration options are:

```text
CONFIG_INPUT_EVDEV=y
CONFIG_SHIROGANE_KEYBOARD_CARDKB=m
```

The driver reads one byte at a time over I²C, converts CardKB codes into standard Linux key events, and registers an input device under `/dev/input/event*`. Its key map includes letters, numbers, punctuation, arrow keys, and Shift and Ctrl combinations.

`=m` means that the driver is built as a kernel module. Once the device tree node is present, Linux can match and load the module through `compatible = "m5stack,cardkb"`.

## `es8311_sound.dts`

### Structure

The ES8311 overlay adds two sections beneath the device tree root:

```text
/
├── sound_i2s
└── i2c_bitbang
    └── es8311@18
```

Their roles are:

- `sound_i2s` combines the F1C200S I²S0 interface and the ES8311 into an ALSA sound card.
- `i2c_bitbang` uses ordinary GPIO pins to implement a software-driven I²C bus for configuring ES8311 registers.
- I²C carries control commands, while I²S carries digital audio data. Both connections are required.

### Sound Card Node

```dts
sound_i2s {
    compatible = "simple-audio-card";
    status = "okay";
    simple-audio-card,name = "es8311";
    simple-audio-card,format = "i2s";
    simple-audio-card,mclk-fs = <256>;
};
```

The properties mean:

- `simple-audio-card` selects the generic Linux ASoC simple sound card driver.
- `simple-audio-card,name` names the sound card `es8311`.
- `simple-audio-card,format = "i2s"` selects the standard I²S data format.
- `simple-audio-card,mclk-fs = <256>` sets the master clock frequency to 256 times the sample rate.

For example, at a 48 kHz sample rate:

```text
48000 × 256 = 12.288MHz
```

### CPU DAI and Codec DAI

```dts
simple-audio-card,cpu {
    sound-dai = <&i2s0>;
};

simple-audio-card,codec {
    sound-dai = <&es8311>;
};
```

These nodes connect the SoC I²S0 digital audio interface to the ES8311 codec:

```text
F1C200S I²S0 ←→ ES8311
```

### I²S0 Interface Dependency

I²S0 is not enabled by the base device tree. When using the ES8311, select exactly one of the following interface overlays according to the actual PCB routing:

```text
interface=i2s0_pa
```

or:

```text
interface=i2s0_pe
```

The two pin layouts are:

```text
i2s0_pa: PE3, PA2, PA3, PA1
i2s0_pe: PE3, PE5, PE6, PA1
```

They must not be enabled together, and the choice must not be inferred from the file name alone. Selecting the wrong pin group prevents the sound card from transferring audio correctly.

Both layouts use PA1 and therefore conflict with ADC interface configurations that also use PA1. `i2s0_pa` additionally uses PA2 and PA3, so it has a wider range of possible conflicts.

### GPIO-Driven I²C

```dts
i2c_bitbang {
    compatible = "i2c-gpio";
    sda-gpios = <&pio 3 0 (GPIO_ACTIVE_HIGH|GPIO_OPEN_DRAIN)>;
    scl-gpios = <&pio 3 12 (GPIO_ACTIVE_HIGH|GPIO_OPEN_DRAIN)>;
    i2c-gpio,delay-us = <5>;
    status = "okay";
};
```

This node uses:

```text
PD0  = SDA
PD12 = SCL
```

`GPIO_OPEN_DRAIN` configures the pins with the open-drain behavior required by I²C.

```dts
i2c-gpio,delay-us = <5>;
```

This introduces an approximate 5 μs delay per half-cycle. A full clock period is therefore about 10 μs, corresponding to an I²C frequency of roughly 100 kHz in standard mode.

The kernel must enable:

```text
CONFIG_I2C_GPIO=y
```

### ES8311 Node

```dts
es8311: es8311@18 {
    compatible = "everest,es8311";
    status = "okay";
    reg = <0x18>;
    pinctrl-names = "default";
    #sound-dai-cells = <0>;
};
```

The properties mean:

- `reg = <0x18>` specifies the 7-bit I²C address of the ES8311.
- `compatible = "everest,es8311"` matches the ES8311 ASoC codec driver.
- `es8311:` provides the label referenced by the sound card node.
- `#sound-dai-cells = <0>` means that no additional argument is required when referencing this digital audio interface.

The node currently contains `pinctrl-names = "default"` without a corresponding `pinctrl-0`, so it does not assign any additional pins. The property may remain, but it has little effect in its current form.

### Linux Driver

The relevant kernel configuration options are:

```text
CONFIG_SOUND=m
CONFIG_SND=m
CONFIG_SND_SOC=m
CONFIG_SND_SUN4I_I2S=m
CONFIG_SND_SOC_ES8311=m
CONFIG_SND_SIMPLE_CARD=m
CONFIG_I2C_GPIO=y
```

The I²S clock changes and ES codec drivers are added by:

```text
board/cra/epass/patch/linux/0010-i2s-and-es-driver.patch
```

The patch contains drivers not only for the ES8311 but also for codecs such as the ES8156, ES8375, and ES8389. The current kernel configuration selects only the ES8311.

### Conflict with Hardware I²C0

Both the F1C200S hardware I²C0 controller and the software-driven I²C bus in this overlay use PD0 and PD12.

The following combination must therefore not be used with the current implementation:

```text
interface=i2c0
ext=es8311_sound
```

Otherwise, two separate Linux I²C controllers would compete for the same physical pins.

This also means that the current ES8311 overlay cannot be enabled directly alongside CardKB or LSM6DS3, both of which depend on hardware I²C0. If these devices must coexist in the future, the device tree should be redesigned so that they share the same hardware I²C0 controller instead of using hardware I²C0 and GPIO-driven I²C simultaneously.

## `lsm6ds3_pre0.4.dts`

### Purpose

This overlay describes an ST LSM6DS3 six-axis inertial sensor:

```dts
fragment@1 {
    target = <&i2c0>;
    __overlay__ {
        lsm6ds3:lsm6ds3@6a {
            compatible = "st,lsm6ds3";
            reg = <0x6a>;
            interrupt-parent = <&pio>;
            interrupts = <4 2 2>;
        };
    };
};
```

The LSM6DS3 contains:

- A three-axis accelerometer.
- A three-axis gyroscope.

Linux manages this device through the IIO (Industrial I/O) subsystem rather than registering it as an ordinary keyboard-style input device.

### I²C0 Dependency

```dts
target = <&i2c0>;
reg = <0x6a>;
```

The LSM6DS3 is connected to hardware I²C0 at address `0x6a`.

On the older hardware for which this overlay was designed, it requires:

```text
interface=i2c0
ext=lsm6ds3_pre0.4
```

CardKB uses address `0x5f`, while the LSM6DS3 uses `0x6a`. Based on their I²C addresses alone, both devices can reside on the same hardware I²C0 bus.

### Interrupt Configuration

```dts
interrupt-parent = <&pio>;
interrupts = <4 2 2>;
```

The three values mean:

```text
4 = GPIO bank E
2 = pin 2
2 = falling-edge trigger
```

The sensor therefore uses PE2 as a falling-edge interrupt input.

The last `2` is a raw numeric value. For better readability, a future revision could include the interrupt type header and express the same setting as `IRQ_TYPE_EDGE_FALLING`; the current form is nevertheless valid in both syntax and value.

### Linux Driver

The relevant kernel configuration options are:

```text
CONFIG_IIO=y
CONFIG_IIO_ST_LSM6DSX=m
```

The LSM6DS3 uses the ST LSM6DSX IIO driver already present in Linux 5.4.99. It does not depend on a project-specific sensor driver patch.

### Power-Off Pin Conflict

The `pre0.4` suffix indicates that this overlay is intended only for hardware revisions earlier than 0.4.

The current base device tree already assigns PE2 to the power-off circuit:

```dts
poweroff: gpio-poweroff {
    compatible = "gpio-poweroff";
    gpios = <&pio 4 2 GPIO_ACTIVE_HIGH>; // PE2
    timeout-ms = <3000>;
};
```

PE2 would therefore be declared simultaneously as:

```text
LSM6DS3 interrupt input
        and
device power-off control output
```

These two uses cannot coexist on the current device. Enabling the overlay may result in:

- Failure to request the LSM6DS3 interrupt GPIO.
- Failure of `gpio-poweroff` to claim PE2.
- The device remaining powered after Linux completes its shutdown sequence.
- A pin direction or signal level that does not match the hardware design.

For this reason, `lsm6ds3_pre0.4` must not be enabled on the current physical device, even though the overlay can be compiled and applied successfully.

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
output/images/dt/ext/cardkb.dtbo
output/images/dt/ext/es8311_sound.dtbo
output/images/dt/ext/lsm6ds3_pre0.4.dtbo
```

The `-@` option preserves the symbols and fixup information required by overlays, allowing U-Boot to resolve labels such as `&i2c0`, `&pio`, and `&i2s0` from the base device tree.

`board/cra/epass/scripts/kernel.its` then packages these files into the FIT image as:

```text
fdt-ext-cardkb
fdt-ext-es8311_sound
fdt-ext-lsm6ds3_pre0.4
```

## Selecting Overlays at Boot

The current project template at `board/cra/epass/uEnv.txt` contains:

```text
interface=
ext=
```

No interface or external-device overlay is enabled by default.

U-Boot accepts space-separated overlay names and applies the `interface` overlays before the `ext` overlays:

```text
patchinterface
        ↓
patchext
```

Each overlay name must exactly match the corresponding FIT node suffix. For example:

```text
interface=i2c0
ext=cardkb
```

Do not enable an external-device overlay merely for testing until the physical wiring, power supply, signal levels, and pin multiplexing have all been verified.

## Dependency and Conflict Summary

| Extension | Required Interface | Pins Used | Main Conflicts |
| --- | --- | --- | --- |
| `cardkb` | `i2c0` | PD0, PD12 | Conflicts with the GPIO-driven I²C bus in the current `es8311_sound` overlay. |
| `es8311_sound` | `i2s0_pa` or `i2s0_pe` | I²C: PD0, PD12; I²S: determined by the selected interface | Conflicts with hardware I²C0; its I²S pins also conflict with some ADC configurations. |
| `lsm6ds3_pre0.4` | `i2c0` | I²C: PD0, PD12; interrupt: PE2 | PE2 conflicts with the device's `gpio-poweroff` node. |

## Build Warning Notes

When an overlay is compiled on its own, `dtc` may emit warnings about `reg`, `#address-cells`, or parent-bus information. These warnings can occur because the standalone overlay compiler cannot see the complete context of the target node in the base device tree.

To determine whether an overlay is genuinely usable, verify all of the following:

1. The `.dts` is successfully compiled into a `.dtbo`.
2. The overlay can be applied correctly to the base `.dtb`.
3. U-Boot applies `interface` and `ext` in the correct order.
4. Linux successfully matches the device with its driver.
5. The physical pins, signal levels, addresses, and interrupts agree with the device tree.

Successful compilation alone does not mean that an overlay is safe to use on the current hardware.

## Modification Guidelines

Confirm the schematic and PCB routing before modifying this directory:

- Never change an I²C address arbitrarily.
- Never assign the same GPIO group to two controllers.
- Never reuse one pin as both an interrupt input and a power-control output.
- Before changing `compatible`, confirm the driver's device match table.
- After adding a peripheral, update `kernel.its` as well; otherwise, the generated `.dtbo` will not be packaged into the FIT image.
- After adding a driver, review `linux.defconfig` and the corresponding kernel patches.
- Before deleting a historical overlay, confirm that compatibility with the relevant older hardware revision is no longer required.

