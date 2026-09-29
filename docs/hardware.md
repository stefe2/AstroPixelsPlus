# Hardware

The dome runs on an AstroPixels board (ESP32) that drives the LEDs directly and sends servo commands to a
Pololu Maestro 24 over serial. Servos have their own 6 V supply.

![Current wiring](wiring/wiring-maestro.svg)

Wiring photo with the Maestro pinout: [wiring-maestro.png](wiring/wiring-maestro.png).
The original PCA9685 wiring ([wiring-original-pca9685.png](wiring/wiring-original-pca9685.png)) no longer applies.

## Components

| Part | Details |
| --- | --- |
| Controller board | [AstroPixels by r2djp](https://r2djp.co.uk/category/electronics/astropixels/) ([documentation](https://r2djp.gitbook.io/astropixels)) |
| MCU | ESP32 DevKit, 30-pin, ESP-WROOM-32 (do not use a 38-pin board) |
| Servo controller | Pololu Mini Maestro 24, device number 1 |
| LEDs | 269 WS2812B: rear logic 108, front logic 90, PSI 2 × 25, holos 3 × 7 |
| Master controller | Kyber (or Marcduino), 3.3 V serial |

The AstroPixels board is basic: no voltage regulator, no level shifter, no protection.

## ESP32 pin assignment

| GPIO | Connector | Use |
| --- | --- | --- |
| 33 | RLD | Rear logic display data |
| 15 | FLD | Front logic display data (strapping pin) |
| 32 | FPSI | Front PSI data |
| 23 | RPSI | Rear PSI data |
| 25 | FHP | Front holo LEDs |
| 26 | RHP | Rear holo LEDs |
| 27 | THP | Top holo LEDs |
| 2 | AUX1 | Heartbeat LED (blinks at 1 Hz, also present on AUX1) |
| 4 | AUX2 | Free |
| 5 | AUX3 | Free (strapping pin) |
| 18 | AUX4 | Serial1 RX, not connected |
| 19 | AUX5 | Serial1 TX → Maestro RX |
| 16 / 17 | Serial2 | Master controller RX / TX, 9600 baud |
| 21 / 22 | I2C | Not used in Maestro mode |

LED data lines are 3.3 V, driven straight from the GPIOs to 5 V LEDs. It works without flicker on this
setup. Do not plug or unplug LED cables with power on: the GPIOs have no protection.

## Power

| Rail | Source | Feeds |
| --- | --- | --- |
| 24 V | Body harness (wires 3–4, ground on 5–6) | Buck converter in the dome |
| 5 V | Buck converter → AstroPixels screw terminal | ESP32, all LEDs, Maestro logic (via AUX5 V/GND) |
| 6 V | External supply → Maestro VSRV terminal | All servos |

- The AstroPixels board needs regulated 5 V (4.8–5.25 V, never above 5.5 V), 2 A minimum, 3 A recommended.
- The Maestro has **no VSRV=VIN jumper**: servo power stays separate from logic power.
- The Maestro VIN accepts 5–16 V, so the 5 V from AUX5 is at the bottom of its range.
- Measured on 2026-09-27: no voltage drop on the AUX5 5 V rail or the 6 V servo supply, even with all
  13 dome panels starting together.

## Dome harness

| Wire | Signal |
| --- | --- |
| 1 | Master controller TX → Serial2 RX (GPIO 16) |
| 2 | ESP32 TX (GPIO 19, AUX5) → Maestro RX |
| 3, 4 | 24 V |
| 5, 6 | GND |

## Pololu Maestro settings

Set in Maestro Control Center:

| Setting | Value |
| --- | --- |
| Serial mode | UART, fixed baud rate |
| Baud rate | 115200 |
| Device number | 1 |
| CRC | Disabled |

The firmware interpolates movements itself (125 ms steps in the `:SE22`–`:SE38` sequences), so Maestro speed
and acceleration can stay at 0. To let the Maestro smooth the original `speed=0` sequences instead, try on
channels 0–12 (panels): speed 80 (≈ 420 ms for an 850 µs travel) and acceleration 5. Leave holo channels 13–18
at speed 0.

Maestro units: speed in 0.25 µs per 10 ms, acceleration in 0.25 µs per 10 ms per 80 ms.

Exported settings: [maestro/maestro_settings.txt](maestro/maestro_settings.txt). "On startup or error" is
**Off** on all 24 channels, so any serial error stops every servo: the firmware must only send valid Pololu
commands. The min/max limits match the closed/open pulses below.

To check the Maestro while the ESP32 runs: close Maestro Control Center, then run
`UscCmd --status` (in `C:\Program Files (x86)\Pololu\Maestro\bin`). `errors: 0x0000` is expected.

## Maestro channel mapping

From `servoSettings[]` in `src/main.cpp`. "Closed" and "open" are the pulses sent for positions 0.0 and 1.0.
Panel numbers P1–P13 are those of the dome plan, checked one by one on the droid: channel N drives P(N+1).
P6 and P13 have no servo.

| Channel | Panel | Type | Group | Closed (µs) | Open (µs) |
| --- | --- | --- | --- | --- | --- |
| 0 | P1 | Small panel | 1 | 1840 | 992 |
| 1 | P2 | Small panel | 2 | 1888 | 992 |
| 2 | P3 | Small panel | 3 | 1872 | 992 |
| 3 | P4 | Medium panel | 4 | 1872 | 992 |
| 4 | P5 | Medium panel | 5 | 1872 | 992 |
| 5 | P6 (no servo) | Big panel | 6 | 2000 | 992 |
| 6 | P7 | Pie panel | 7 | 2000 | 992 |
| 7 | P8 | Pie panel | 8 | 2000 | 992 |
| 8 | P9 | Pie panel | 9 | 2000 | 992 |
| 9 | P10 | Pie panel | 10 | 1920 | 992 |
| 10 | P11 | Mini panel | 11 | 1872 | 992 |
| 11 | P12, mini front PSI door | Mini panel | 12 | 1552 | 992 |
| 12 | P13, dome top (no servo) | Top pie panel | 13 | 2000 | 992 |
| 13 | Front holo, horizontal | Holo H | — | 1248 | 1744 |
| 14 | Front holo, vertical | Holo V | — | 1248 | 1744 |
| 15 | Top holo, horizontal | Holo H | — | 1248 | 1744 |
| 16 | Top holo, vertical | Holo V | — | 1248 | 1744 |
| 17 | Rear holo, horizontal | Holo H | — | 1248 | 1744 |
| 18 | Rear holo, vertical | Holo V | — | 1248 | 1744 |
| 19–23 | Unused | — | 14–18 | 2000 | 992 |

Channel names in the Maestro (`RHP-H` = 17, `RHP-V` = 18) match `rearHolo.assignServos(&servoDispatch, 17, 18)`.
