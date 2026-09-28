# AstroPixelsPlus — Kyber Edition

> **Personal fork of [AstroPixelsPlus](https://github.com/reeltwo/AstroPixelsPlus) by [reeltwo](https://github.com/reeltwo).**
> All credit for the original work goes to the original author. This fork drives the dome servos with a
> Pololu Maestro 24 and adds reliability fixes and new sequences.
> Designed to work with the [Kyber Controller](https://www.facebook.com/groups/1341505756182087) — join the
> Kyber community on Facebook for support.

> ⚠️ **Work in progress.** Features may change, break or be incomplete. Use at your own risk.
> Wi-Fi and the web interface are compiled out: send commands over USB or Serial2.

## What this fork adds

- **Pololu Maestro 24 servo controller** on Serial1 (GPIO 19, 115200 baud) instead of the PCA9685 I²C boards.
  `ServoDispatchMaestro` implements the Reeltwo `ServoDispatch` interface and interpolates movements on the ESP32.
- **Interpolated sequences `:SE22`–`:SE38`**: copies of `:SE02`–`:SE09` and `:SE50`–`:SE58` driven at
  125 ms per step, the reliable minimum measured for the dome panels.
- **Safer panel handling**: all dome panels close at the end of every sequence, then servos are released;
  `:CL00` closes the panels at any time.
- **Holo animations**: continuous random moves (`*HA01`–`*HA03`) and "R2 alive" moves with blue↔white
  LED fades (`*HV01`–`*HV03`).
- **Heartbeat LED** on GPIO 2 (1 Hz) to show the main loop is running.

## Documentation

| Document | Content |
| --- | --- |
| [docs/commands.md](docs/commands.md) ([HTML](docs/commands.html)) | Every serial command, checked against the source |
| [docs/hardware.md](docs/hardware.md) | Board, wiring, power, Maestro settings and channel mapping |
| [docs/build-and-flash.md](docs/build-and-flash.md) | Build, flash and update the prebuilt binaries |
| [docs/review/revue-firmware.md](docs/review/revue-firmware.md) | Firmware review (French): known issues and action plan |
| [docs/maestro-migration-history.md](docs/maestro-migration-history.md) | Development log of the Maestro migration (French) |
| [TODO.md](TODO.md) | Open tasks (French) |

## Quick start

```bash
pio run -t upload
```

Or flash the prebuilt `firmware/Astropixels.bin` at offset `0x0`. Details in
[docs/build-and-flash.md](docs/build-and-flash.md).

## Hardware at a glance

| Part | Details |
| --- | --- |
| Board | [AstroPixels](https://r2djp.co.uk/category/electronics/astropixels/) with a 30-pin ESP32 DevKit |
| Servos | Pololu Maestro 24: panels on channels 0–12, holos on 13–18 |
| Commands | Serial2, GPIO 16/17, 9600 baud (Kyber / Marcduino) and USB, 115200 baud |
| Power | 5 V regulated for the board and LEDs, separate 6 V for the servos |

Wiring diagram and full pinout in [docs/hardware.md](docs/hardware.md).

## Known issues

The [firmware review](docs/review/revue-firmware.md) and [TODO.md](TODO.md) track the remaining issues and the
hardware tests still to run.

## Project layout

| Path | Content |
| --- | --- |
| `src/main.cpp` | Firmware entry point: device setup, servo table, main loop |
| `src/` | Also the command handlers (`Marcduino*.h`), custom sequences, logic effects, web pages |
| `lib/Reeltwo/` | Reeltwo 23.5.3 with the local changes (Maestro driver) |
| `firmware/` | Prebuilt binaries |
| `tools/` | Binary merge script and HTML generator |
| `docs/` | Documentation and diagrams |

## Original project

- Repository: <https://github.com/reeltwo/AstroPixelsPlus>
- Wiki: <https://github.com/reeltwo/AstroPixelsPlus/wiki>
- Web installer (original firmware): <https://reeltwo.github.io/AstroPixels-Installer/>

Original Wi-Fi defaults, for reference if Wi-Fi is compiled back in (`#define USE_WIFI`): SSID `AstroPixels`,
password `Astromech`, web interface at <http://192.168.4.1>.

## Libraries

- [Reeltwo](https://github.com/reeltwo/Reeltwo) 23.5.3, included in `lib/Reeltwo/` with local changes
  ([LOCAL-CHANGES.md](lib/Reeltwo/LOCAL-CHANGES.md))
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) 1.15.2
- [FastLED](https://github.com/FastLED/FastLED) 3.7.0
- [DFRobotDFPlayerMini](https://github.com/DFRobot/DFRobotDFPlayerMini) 1.0.6

## License

See [LICENSE](LICENSE).
