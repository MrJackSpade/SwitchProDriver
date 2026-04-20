# SwitchProDriver

Windows user-mode service that translates Nintendo Switch Pro Controller HID input into a virtual XInput (Xbox 360) gamepad via [ViGEmBus](https://github.com/nefarius/ViGEmBus). Works over USB or Bluetooth. No driver signing, no reboot, no anti-cheat incompatibility.

Originally scoped as a UMDF2 driver (see issue #1) — pivoted to a service because driver signing requires either test-signing mode (breaks kernel anti-cheat) or a paid EV cert (impractical for a personal project). The service approach delivers the same user-facing experience (install, auto-start on boot, no UI) without any of those costs.

## Status

MVP working:
- Service enumerates the Pro Controller on USB or Bluetooth via HID APIs.
- Sends mode-switch subcommand `0x03/0x30` to put the controller into full-report mode.
- Pended overlapped `ReadFile` loop on the HID device; parses report ID `0x30`.
- Translates buttons (physical-position map: Nintendo B → XInput A, etc.), 12-bit-packed sticks with a deadzone, and ZL/ZR as digital 0-or-255 triggers.
- Feeds `XUSB_REPORT` to ViGEmBus on every parsed input.
- Clean reconnect on disconnect / BT dropout.
- Installs as a Windows Service with auto-start and proper SCM stop handling.

What's not wired up yet (see [Out of scope](#out-of-scope-tracked-separately)):
- Rumble (ViGEm X360 notification → Pro Controller HD Rumble subcommand).
- IMU passthrough.
- Player LED 1 on connect.
- Report descriptor rewrite — the real Pro Controller still shows as a separate HID gamepad in `joy.cpl` alongside the virtual Xbox pad. Hide it with [HidHide](https://github.com/nefarius/HidHide) if that bothers you.
- Capture button is unmapped (XInput has no equivalent).

## Build

Requires:
- Visual Studio 2026 Community (or 2022) with the C++ workload
- Windows SDK 10.0.26100 (or compatible)
- [ViGEmBus](https://github.com/nefarius/ViGEmBus/releases) runtime installed (provides the virtual Xbox 360 pad)

```cmd
scripts\setup.cmd
```

Builds `external/ViGEmClient/lib/release/x64/ViGEmClient.lib`, `build\test\vigem_smoke.exe`, `build\test\input_test.exe`, and `build\svc\switchprosvc.exe`.

## Install

```cmd
scripts\install_service.cmd
```

Self-elevates, installs the service with `SERVICE_AUTO_START`, kicks it off immediately, and renames the virtual pad to "Switch Pro Controller" in `joy.cpl` (per-user registry entry).

Service management:
```cmd
sc query switchprosvc
sc stop switchprosvc
sc start switchprosvc
```

Uninstall:
```cmd
scripts\uninstall_service.cmd
```

Sweeps any orphaned virtual pads ViGEm may have left behind and restores the default joy.cpl name.

## Use

1. Pair the Pro Controller via Bluetooth (Settings → Bluetooth & devices → Add → Bluetooth → "Pro Controller"), **or** plug it in via USB (requires "Pro Controller Wired Communication" ON in the Switch's System Settings → Controllers and Sensors).
2. The service auto-discovers the controller, sends the mode-switch subcommand, and attaches a virtual Xbox 360 pad. Games that use XInput see it as controller slot 0.

Verify with:
```cmd
build\test\input_test.exe
```
Move the sticks / press buttons — you should see the mapped XInput state change.

## Button mapping

Physical-position mapping (bottom face button is always XInput A regardless of what Nintendo labels it):

| Pro Controller | XInput | Notes |
|---|---|---|
| B (bottom face) | A | |
| A (right face) | B | |
| Y (left face) | X | |
| X (top face) | Y | |
| L / R | LB / RB | |
| ZL / ZR | Left / Right Trigger | digital → 0 or 255 |
| Minus | Back | |
| Plus | Start | |
| L stick click | Left Thumb | |
| R stick click | Right Thumb | |
| D-pad | D-pad | |
| Home | Guide | invisible via `XInputGetState`; visible via `XInputGetStateEx` |
| Capture | (unmapped) | |

## Repository layout

```
svc/          User-mode service (main.c, worker.c, hid_io.c + .h)
driver/       Original UMDF2 driver sources — retained for reference.
              Pure-logic files (common.h, input.c, mapping.c) are shared with svc/.
test/         vigem_smoke.c (ViGEm smoke test), input_test.c (XInput poller)
scripts/      Build + install helpers
external/     Third-party checkouts (ViGEmClient is cloned here by setup; ignored by git)
```

The `driver/` directory contains a functional UMDF2 HID upper filter that compiles to a signed `.dll` + `.cat`. It's retained as a reference for anyone who wants to revive the driver path with a real code-signing certificate. The service is the recommended approach.

## Out of scope (tracked separately)

To be filed as individual GitHub issues:
- **Rumble** — ViGEm X360 notification callback → HD Rumble 4-byte-per-motor subcommand `0x01`
- **IMU parsing** — bytes 13–48 (accel + gyro, three frames per report)
- **Player LED 1 on connect** — subcommand `0x30`
- **HidHide integration** — hide the real Pro Controller HID device from DirectInput / joy.cpl so only the virtual pad is visible
- **Configurable Capture bind** — config file, defaults to `Win+Alt+PrtScn`
- **Sleep/wake recovery validation** — confirm the service cleanly re-attaches after system resume / controller reconnect
- **DS4 emulation mode** — optional output profile that exposes a DualShock 4 instead of Xbox 360 (so Capture can map to Share)

## License

TBD.
