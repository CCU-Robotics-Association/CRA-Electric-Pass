# Built-In Fallback Theme

This directory appears on the target device as:

```text
/root/res/fallback/
```

## Current Configuration

The current `epconfig.json` declares:

| Field | Value | Meaning |
| --- | --- | --- |
| `version` | `1` | Theme configuration format version |
| `uuid` | `6752f6db-562e-42a7-819b-13a337208591` | CRA theme identifier |
| `name` | `CRA Electric Pass` | Theme name shown in the list |
| `screen` | `360x640` | Compatible display resolution |
| `loop.file` | `loop_1.mp4` | Loop video |
| `transition_in.type` | `none` | Do not play the upstream glitch-flash transition on entry |
| `transition_loop.type` | `none` | Do not add another transition when the video loops |
| `overlay.type` | `cra_pass` | Use the native CRA E-PASS overlay |
| `overlay.options.appear_time` | `100000` | Schedule the overlay 0.1 seconds after loading the theme; the overlay implementation retains its internal 1.5-second entrance delay |
| `overlay.options.top_left_text` | `CRA E-PASS` | Select the custom CRA template and display the left-side mark |
| `overlay.options.top_right_bar_text` | `CCU Robotics Association` | Organization name shown on the right-side bar |

## Load Logic

After scanning regular themes, the main program parses this directory directly when `theme_count == 0`. It still performs the normal validation steps, including checking:

- whether the JSON can be parsed;
- whether `version` is `1`;
- whether the UUID is valid;
- whether `screen` matches the firmware's current `360x640` resolution;
- whether `loop_1.mp4` exists and is readable;
- whether the CRA overlay configuration is complete and valid.

If the fallback theme itself is invalid, the current code still sets the theme count to 1. Another automatic fallback layer therefore cannot be relied upon to recover from a damaged fallback configuration.
