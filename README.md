# AstroPixelsPlus — Custom Fork

> **This is a personal fork of the original [AstroPixelsPlus](https://github.com/reeltwo/AstroPixelsPlus)
> firmware by [reeltwo](https://github.com/reeltwo), generously made available to the community.
> All credit for the original work goes to the original author.
> This fork adds support for a Pololu Maestro 24-channel servo controller and several
> reliability improvements on top of the original codebase.**

> ⚠️ **Work in progress — this fork is actively evolving and is far from a final release.
> Features may change, break, or be incomplete. Use at your own risk.**
>
> 📵 **WiFi and the web interface are currently disabled** in this build to free up resources
> during development. Commands must be sent via serial (USB or Serial2/Marcduino).

---

## What's Different in This Fork

### Pololu Maestro 24-Channel Servo Controller

The original firmware uses a PCA9685 I²C servo controller. This fork replaces it with a
**Pololu Maestro 24-channel** controller connected via Serial1 (GPIO 18/19, 115200 baud).

A new class `ServoDispatchMaestro` was written to implement the full `ServoDispatch` interface
using the Pololu Compact Serial Protocol. It handles:

- ESP32-side temporal interpolation of servo positions (used instead of relying on the
  Maestro's internal speed/acceleration settings, which proved unreliable at speed=0)
- Per-servo enable/disable with PWM-off (command `0x60`) for true torque-free state
- Automatic per-servo stop after 700 ms of inactivity (unless a sequence is active)

**Wiring:**

| ESP32 | Maestro |
|-------|---------|
| GPIO 19 (TX — Serial1) | RX |
| GPIO 18 (RX — Serial1) | TX (optional) |
| GND | GND |

Maestro configuration: Serial mode `UART, fixed baud rate`, 115200 baud, Device ID 1, CRC disabled.

### Servo Sequencer Reliability Fixes

Two bugs in the servo sequencer were identified and fixed:

1. **`fLastIsFinished` bug** — `setSequenceActive(true)` was never called because
   `play()` sets the sequence pointer synchronously before `animate()` runs.
   On the first frame `isFinished()` already returns `false`, so the transition
   was never detected. Per-servo 700 ms auto-stop timers would fire mid-sequence
   and disable servos while they were still moving.
   **Fix:** `fLastIsFinished = true` tracks the previous frame's finished state.

2. **Ghost position bug in `_moveServoToPulse`** — After `stop()`, `fActive = false`
   but `fCurrentPos` retained a stale value from the previous `speed=0` sequence
   (which sets `fCurrentPos` instantly without physical movement). Subsequent calls
   to `:CL00` were silently skipped because `fCurrentPos == targetPos` appeared true.
   **Fix:** Early-exit condition changed to `fActive && fCurrentPos == pos`.

### Automatic Close-All at Sequence End

When a sequence finishes, all dome panel servos are automatically moved to their
closed position (`moveServosTo(ALL_DOME_PANELS_MASK, 125, 0.0)`) and then disabled
after 1500 ms. This prevents panels from being left open accidentally.

### `:CL00` — Reliable Close All Panels

`:CL00` now calls `servoDispatch.moveServosTo()` directly (instead of playing a
sequence), making it work reliably at any time — even immediately after `stop()` or
after a `speed=0` sequence.

### New Interpolated Sequences: SE22–SE38

The original sequences SE02–SE09 and SE50–SE58 use `speed=0` (Maestro manages
movement internally), which is unreliable with the current Maestro configuration.

New sequences SE22–SE38 are interpolated equivalents where the ESP32 guides the
servos step-by-step at **speed=125 ms/step** — the identified physical minimum for
reliable dome panel travel.

| Command | Equivalent | Description |
|---------|------------|-------------|
| `:SE22` | SE02 | Wave |
| `:SE23` | SE03 | Fast Wave |
| `:SE24` | SE04 | Open/Close Wave |
| `:SE25` | SE05 | Marching Ants (15 s) |
| `:SE26` | SE06 | Short Circuit (8 s) |
| `:SE27` | SE07 | Cantina Dance (46 s) |
| `:SE28` | SE08 | Leia Message (45 s) |
| `:SE29` | SE09 | Disco |
| `:SE30` | SE50 | Scream — logics only |
| `:SE31` | SE51 | Scream — panels only |
| `:SE32` | SE52 | Slow Wave |
| `:SE33` | SE53 | Fast Wave |
| `:SE34` | SE54 | Open Wave |
| `:SE35` | SE55 | Marching Ants |
| `:SE36` | SE56 | Faint (open/close long) |
| `:SE37` | SE57 | Rhythmic |
| `:SE38` | SE58 | One by One |

The original SE02–SE09 and SE50–SE58 are preserved unchanged as reference.

> **Note:** Sequences `:SE00` (stop) and `:SE01` (Scream with logics) are unchanged.

### Holo Projector Enhancements

Two new animated subsystems have been added for the holo projectors:

**HoloAliveAnimator** — Continuous random movement for all 3 holos, independently
timed with staggered offsets for an organic feel. LEDs are not affected.

| Command | Description |
|---------|-------------|
| `*HA01` | Start — Slow (8–15 s between moves) |
| `*HA02` | Start — Medium (3–8 s between moves) |
| `*HA03` | Start — Fast (1–4 s between moves) |
| `*HZ00` | Stop — return to center |

**HoloVivantAnimator ("R2 Alive")** — Combines continuous random movement with
soft LED fades (blue↔white, 1.5 s fade in/out, 5–10 s on, 3–9 s off).
Each holo is independent and offset in time.

| Command | Description |
|---------|-------------|
| `*HV01` | Start — Slow (8–15 s) + LEDs |
| `*HV02` | Start — Medium (3–8 s) + LEDs |
| `*HV03` | Start — Fast (1–4 s) + LEDs |
| `*HV00` | Stop — return to center, LEDs off |

---

## Hardware Configuration

| Component | Details |
|-----------|---------|
| Controller | ESP32 |
| Servo controller | Pololu Maestro 24-channel |
| Servo serial | Serial1 — GPIO 18 (RX), GPIO 19 (TX), 115200 baud |
| Marcduino serial | Serial2 — GPIO 16 (RX), GPIO 17 (TX), 9600 baud |
| Panel servos | Maestro channels 0–12 |
| Holo servos | Maestro channels 13–18 |

### Maestro Channel Mapping

| Channel | Servo | Type |
|---------|-------|------|
| 0 | Door 4 | SMALL_PANEL |
| 1 | Door 3 | SMALL_PANEL |
| 2 | Door 2 | SMALL_PANEL |
| 3 | Door 1 | MEDIUM_PANEL |
| 4 | Door 5 | MEDIUM_PANEL |
| 5 | Door 9 | BIG_PANEL |
| 6 | Pie 1 | PIE_PANEL |
| 7 | Pie 2 | PIE_PANEL |
| 8 | Pie 3 | PIE_PANEL |
| 9 | Pie 4 | PIE_PANEL |
| 10 | Mini 2 | MINI_PANEL |
| 11 | Mini PSI | MINI_PANEL |
| 12 | Top Center | TOP_PIE_PANEL |
| 13 | Front Holo H | HOLO_HSERVO |
| 14 | Front Holo V | HOLO_VSERVO |
| 15 | Top Holo H | HOLO_HSERVO |
| 16 | Top Holo V | HOLO_VSERVO |
| 17 | Rear Holo H | HOLO_HSERVO |
| 18 | Rear Holo V | HOLO_VSERVO |

---

## Original Firmware

All original AstroPixelsPlus functionality is preserved.
Please refer to the original project for documentation on logic displays, PSI,
NeoPixel effects, WiFi configuration, and the web interface.

- **Original repository:** https://github.com/reeltwo/AstroPixelsPlus
- **Original wiki:** https://github.com/reeltwo/AstroPixelsPlus/wiki
- **Web installer (original):** https://reeltwo.github.io/AstroPixels-Installer/
- **AstroPixels boards:** https://we-make-things.co.uk/product/astropixels/

### Default WiFi Credentials (original — currently disabled)

> 📵 WiFi and the web interface (`http://192.168.4.1`) are **disabled** in this fork for now.
> Use `#APWIFI1` to re-enable WiFi if needed.

| Setting | Value |
|---------|-------|
| SSID | AstroPixels |
| Password | Astromech |
| Web interface | http://192.168.4.1 (disabled) |

---

## Original Documentation

The sections below are preserved from the original README for reference.

---

# Configuration Commands Supported

## #APWIFI[0|1] - Change WiFi Setting
  #APWIFI - Toggle WiFi
  #APWIFI0 - WiFi Off
  #APWIFI1 - WiFi On

## #APREMOTE[0|1] - Change Droid Remote Setting
  #APREMOTE - Toggle Droid Remote Support
  #APREMOTE0 - Droid Remote Support Off
  #APREMOTE1 - Droid Remote Support On

## #APZERO - Clear All Preference including WiFi Settings

## #APRESTART - Restart AstroPixels

# Marcduino Commands Supported

The prefix @ is optional and is ignored. All Marcduino commands are terminated by \r (carriage return).

## @1T1 - Front Logics set to Normal

## @1T2 - Front Logics set to Flashing Color (06)

## @1T3 - Front Logics set to Alarm (01)

## @1T4 - Front Logics set to Failure (02)

## @1T5 - Front Logics set to Red Alert (11)

## @1T6 - Front Logics set to Leia (03)

## @1T11 - Front Logics set to March (04)

## @2T1 - Rear Logics set to Normal

## @2T2 - Rear Logics set to Flashing Color (06)

## @2T3 - Rear Logics set to Alarm (01)

## @2T4 - Rear Logics set to Failure (02)

## @2T5 - Rear Logics set to Red Alert (11)

## @2T6 - Rear Logics set to Leia (03)

## @2T11 - Rear Logics set to March (04)

## @0T1 - All Logics set to Normal

## @0T2 - All Logics set to Flashing Color (06)

## @0T3 - All Logics set to Alarm (01)

## @0T4 - All Logics set to Failure (02)

## @0T5 - All Logics set to Red Alert (11)

## @0T6 - All Logics set to Leia (03)

## @0T11 - All Logics set to March (04)

## @1MHello - Set top front logics text to "Hello" and scroll left

## @2MWorld - Set bottom front logics text to "World" and scroll left

## @3MAstromech - Set rear logics text to "Astromech" and scroll left

## @1P60 - Set front logics font to Latin

## @2P60 - Set front logics font to Latin

## @3P60 - Set rear logics font to Latin

## @1P61 - Set front logics font to Aurabesh

## @2P61 - Set front logics font to Aurabesh

## @3P61 - Set rear logics font to Aurabesh

# Panel commands

## :CL00 - Close all panels

## :OP00 - Open all panels

## :OF00 - Flutter all panels

## :SF<servo number>$<easing number> - Set servo easing for <servo number> to <easing number>

## :OP<number> - Open panel <number>. For example :OP01

## :CL<number> - Open panel <number> For example: CL02

## :OF<number> - Flutter panel <number> For example: OF02

# Holo projector commands

## *ON01 - Front holo on

## *OF01 - Front holo off

## *ON02 - Rear holo on

## *OF02 - Rear holo off

## *ON03 - Top holo on

## *OF03 - Top holo off

## *ST00 - Reset all holos

## *RD01 - Front holo random move

## *RD02 - Rear holo random move

## *RD03 - Top holo random move

## *HPS301 - Front holo pulse

## *HPS302 - Rear holo pulse

## *HPS303 - Top holo pulse

## *HPS601 - Front holo rainbow

## *HPS602 - Rear holo rainbow

## *HPS603 - Top holo rainbow

## *HP001 - Front holo position down

## *HP101 - Front holo position center

## *HP201 - Front holo position up

## *HP301 - Front holo position left

## *HP401 - Front holo position upper left

## *HP501 - Front holo position lower left

## *HP601 - Front holo position right

## *HP701 - Front holo position upper right

## *HP801 - Front holo position upper right

## *HP002 - Rear holo position down

## *HP102 - Rear holo position center

## *HP202 - Rear holo position up

## *HP302 - Rear holo position left

## *HP402 - Rear holo position upper left

## *HP502 - Rear holo position lower left

## *HP602 - Rear holo position right

## *HP702 - Rear holo position upper right

## *HP802 - Rear holo position upper right

## *HP003 - Top holo position down

## *HP103 - Top holo position center

## *HP203 - Top holo position up

## *HP303 - Top holo position left

## *HP403 - Top holo position upper left

## *HP503 - Top holo position lower left

## *HP603 - Top holo position right

## *HP703 - Top holo position upper right

## *HP803 - Top holo position upper right

## *HN01 - Front holo nod

## *HN02 - Rear holo nod

## *HN03 - Top holo nod

# AstroPixels specific Marcduino commands

Additional you can select a sequence to run by sending:

@APLE followed by a integer in this format LEECSNN

## L - the logic designator - if not provided, defaults to 0 (all)
   0 - All  
   1 - Front  
   2 - Rear  

## EE - the effect - use two digits if logic designator provided
   00 - Reset to Normal  
   01 - Alarm - flips between color and red  
   02 - Failure - cycles colors and brightness fading  
   03 - Leia - pale green  
   04 - March - sequence timed to Imperial March  
   05 - Single Color - single hue shown  
   06 - Flashing Color - single hue on and off  
   07 - Flip Flop Color - boards flip back and forth - similar to march  
   08 - Flip Flop Alt - other direction of flips on back board, front is same to flip flop  
   09 - Color Swap - switches between color specified and inverse compliment color  
   10 - Rainbow - rotates through colors over time  
   11 - Red Alert - shows color specified  
   12 - Mic Bright - brightness of color specified back on mic input  
   13 - Mic Rainbow - color goes from default specified through color range  
   14 - Ligts out - slowly turn off all LEDs  
   15 - Display Text  
   16 - Text Scrolling Left  
   17 - Text Scrolling Right  
   18 - Text Scrolling Up  
   19 - Roaming pixel  
   21 - Vertial scan line  
   22 - Fire  
   23 - PSI style color wipe between two colors  
   99 - Random  
## C - color designator
   1 - Red  
   2 - Orange  
   3 - Yellow  
   4 - Green  
   5 - Cyan (Aqua)  
   6 - Blue  
   7 - Purple  
   8 - Magenta  
   9 - Pink  
   0 - Default color on alarm / default to red on many effects / color cycle on march / ignored on failure and rainbow  
## S - speed or sensitivity (1-9 scale) with 5 generally considered default for speed
   Flip Flop and Rainbow - 200ms x speed  
   Flash - 250ms x speed  
   March - 150ms x speed  
   Color Swap - 350ms x speed  
   Red Alert - sets mic sensitivity - as a fraction of speed / 10 - we recommend 3  
   Mic Bright - sets minimum brightness - fraction of speed / 10  
## NN - 2 digit time length in seconds
   00 for continuous use on most  
   00 for default length on Leia  
   Not used on March or Failure  

 ## Some sequence examples:
 Note: Leading 0s drop off as these are long ints  
 Solid Red:  @APLE51000  
 Solid Orange: @APLE52000  
 Solid Yellow:  @APLE53000  
 Solid Green:  @APLE54000  
 Solid Cyan:  @APLE55000  
 Solid Blue:  @APLE56000  
 Solid Purple:  @APLE57000  
 Solid Magenta:  @APLE58000  
 Solid Pink: @APLE59000  
 Alarm (default):  @APLE10500  
 Failure: @APLE20000  
 Leia: @APLE30000  
 March:  @APLE40500  
 March (Red Only):  @APLE41500  
 Flash (Yellow): @APLE63500  
 Color Swap (pink): @APLE99500  
 Rainbow: @APLE100500  
 Red Alert: @APLE111300  
 Mic Bright (Green): @APLE124200  
 Mic Rainbow (Cyan): @APLE135000  
 Fire: @APLE225000

 @APLE54008 - solid green for 8 seconds  
 @APLE63315 - flashing yellow at slightly higher speed for 15 seconds  
 @APLE30008 - leia effect for only 8 seconds  

**********

## Libraries Used

<ul>
<li>https://github.com/reeltwo/Reeltwo</li>
<li>https://github.com/adafruit/Adafruit_NeoPixel</li>
<li>https://github.com/FastLED/FastLED</li>
<li>https://github.com/DFRobot/DFRobotDFPlayerMini</li>
</ul>
