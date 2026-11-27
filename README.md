# Wireless HID

An **ESP32-C3** that acts as a Bluetooth Low Energy **keyboard + mouse**, controlled over WiFi from any browser. Hosts its own captive-portal access point *and* joins your home network, so you can drive the HID from a phone or laptop with no app to install.

<p align="center">
  <img src="docs/ui-hero.png" alt="Wireless HID web control panel" width="340">
</p>

<details>
<summary><b>Full control panel screenshot</b></summary>

<p align="center">
  <img src="docs/ui-full.png" alt="Full Wireless HID control panel" width="420">
</p>

</details>

## Features

- **BLE HID** (NimBLE) — types text, presses keys/combos, moves/clicks/scrolls the mouse on a paired device
- **Browser control** — served web UI at `http://hid.local` (or `192.168.4.1`), plus a REST endpoint and a low-latency WebSocket for mouse movement
- **Dual WiFi** — device AP (`Wireless-HID`) + connects to your home WiFi; captive portal auto-opens the config page
- **Mouse jiggler** — configurable min/max interval, step size, randomised direction, with history and count; runs in firmware independently of the browser
- **Sequences** — up to 5 named sequences of text rows, each followed by a configurable pause, with repeat count; run by the firmware so they keep going if the browser closes. Shortcuts: Ctrl+Shift+Enter (run selected), Ctrl+Shift+F1–F5 (run 1–5), Ctrl+Shift+X (stop)
- **US / UK keyboard layouts**, switchable and persisted
- **BLE bond management** — list, rename, and forget paired devices
- All settings persisted to flash (NVS) via `Preferences`

## Hardware

- ESP32-C3 dev board (`esp32-c3-devkitm-1`)
- USB-C for power, flashing, and serial console

## Setup

This project keeps credentials out of source. Before building:

```bash
cp src/secrets.h.example src/secrets.h
# edit src/secrets.h and set your own AP_SSID / AP_PASS
```

`src/secrets.h` is gitignored and never committed. Your **home WiFi** credentials are *not* stored in source — you enter them at runtime through the web UI, and they live only in the device's NVS flash.

## Build & flash

Requires [PlatformIO](https://platformio.org/). The platform is pinned to `espressif32@7.0.1`: 8.0.0 crashes in BLE controller init.

```bash
pio run              # compile
pio run -t upload    # flash over USB (esp-builtin, /dev/ttyACM0)
pio device monitor   # serial console @ 115200
```

## Troubleshooting (Linux)

- **Boot loop with `Guru Meditation Error … Instruction access fault` right after `btdm:` lines.** The decoded crash is in `r_lld_env_init` (BLE controller). This happens with `espressif32` 8.0.0, so keep the platform pinned to `7.0.1` in `platformio.ini`.
- **`Permission denied` on `/dev/ttyACM0`.** Add yourself to the serial group (`uucp` on Arch, `dialout` on Debian/Ubuntu), then log out and back in: `sudo usermod -aG uucp $USER`.
- **Upload fails with `libusb_open() failed with LIBUSB_ERROR_ACCESS` / `esp_usb_jtag: could not find or open device`.** `upload_protocol = esp-builtin` flashes through the chip's USB-JTAG with OpenOCD, which needs raw USB access, not just the serial port. You can either:
  - install OpenOCD's udev rule for `303a:1001` (`60-openocd.rules`), or
  - flash over serial instead. Copy `platformio.ini`, set `upload_protocol = esptool` in the copy, and run `pio run -c <copy> -t upload`.
- **The board isn't detected at all (no `303a:1001` in `lsusb`).** Try a different USB-C cable, since charge-only cables are common. Or hold BOOT while plugging in.
- **`No module named pip` / `MissingPackageManifestError` when PlatformIO installs tools.** A distro-packaged PlatformIO (e.g. Arch's `platformio`) runs on system Python without pip. Use PlatformIO in its own venv instead: `python -m venv ~/.platformio/penv && ~/.platformio/penv/bin/pip install platformio`, then use `~/.platformio/penv/bin/pio`.
- **The web UI is slow, or the board sometimes doesn't join home WiFi at boot.** TX power is lowered to 8.5 dBm (`WIFI_POWER_8_5dBm`) to coexist with BLE, so the link is weak at range. The page is served gzipped (about 12 KB) to help. Moving the board closer to the router also helps. The `192.168.4.1` AP is always available as a fallback.
- **Testing `/cmd` or `/seq/save` with curl.** Send the body as `text/plain`, like the browser does: `curl -H 'Content-Type: text/plain' --data-binary 'STATUS' http://hid.local/cmd`. Plain `--data` sends form encoding, and the server answers `NO_BODY`.

## Command reference

Commands accepted over serial, `POST /cmd`, or the WebSocket:

| Command | Action |
|---|---|
| `TYPE <text>` | Type a string (`\n`, `\t` parsed) |
| `KEY <keycode>` | Press a raw HID keycode |
| `COMBO <mod> <key>` | Modifier bits + keycode (e.g. `COMBO 1 6` = Ctrl+C) |
| `MOVE <x> <y>` | Relative mouse move (-127..127) |
| `DRAG <x> <y>` | Move with left button held |
| `SCROLL <n>` | Scroll wheel |
| `CLICK <1\|2\|3>` | Click left/right/middle |
| `PRESS <1\|2\|3>` / `RELEASE` | Hold / release buttons |
| `STATUS` | Connection status |
| `LAYOUT US\|UK` | Set keyboard layout |
| `JIGGLE ON\|OFF` | Toggle jiggler |
| `JIGGLE MIN\|MAX <ms>` | Set jiggle interval bounds |
| `JIGGLE STEP <px>` | Set jiggle step size |
| `JIGGLE RAND <0\|1>` | Randomise jiggle direction |
| `SEQ RUN <1-5>` | Run a saved sequence |
| `SEQ STOP` | Stop the running sequence |

## Project layout

```
platformio.ini        board + library config
src/
├── main.cpp          app: WiFi/AP, web server, WebSocket, jiggler
├── BleHid.h          NimBLE keyboard+mouse HID (header-only)
├── web_ui.h          embedded browser UI (PAGE_HTML) — edit this
├── web_ui_gz.h       gzipped copy, generated at build by tools/gzip_ui.py
├── secrets.h         AP credentials — gitignored, create from example
└── secrets.h.example template for secrets.h
```
