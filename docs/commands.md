# AstroPixelsPlus Command Reference

Every command listed here is declared in the firmware source (`src/main.cpp` and `src/Marcduino*.h`).
Wiring, power and Maestro settings are in [hardware.md](hardware.md).

## Sending commands

| Port | Pins | Speed | Notes |
| --- | --- | --- | --- |
| USB (Serial) | USB connector | 115200 baud | Serial monitor |
| Serial2 | GPIO 16 (RX), GPIO 17 (TX) | 9600 baud | Kyber / Marcduino controller, 3.3 V logic |

- End each command with CR (`\r`), LF (`\n`) or CR+LF. Empty lines are ignored.
- Commands are matched by prefix; the longest matching command wins. Anything after it is the command argument.
- Only one command/animation runs at a time: a new command stops the running one.

## Prefixes

| Prefix | Target |
| --- | --- |
| `:` | Panels, servos, sequences |
| `*` | Holo projectors |
| `@` | Logic displays, PSI, alternative holo commands |
| `$` | Music-synchronised sequences |
| `~RT`, `@AP` | Raw Reeltwo command (for example `LE…`, `HP…`) |
| `#AP` | System commands |

---

## Panels and servos (`:`)

### All panels

| Command | Description |
| --- | --- |
| `:OP00` | Open all dome panels |
| `:CL00` | Close all dome panels (125 ms interpolated move) |
| `:OF00` | Flutter all dome panels |
| `:ST00` | Stop and disable all servos |
| `:SD<n>` | Disable servo `<n>` (for example `:SD4`) |

Open commands (`:OP00`, `:OP01`–`:OP20`, `:OP$…`) leave the panels open until `:CL00` or another
sequence. Outside sequences, every servo is released (no pulses) 0.7 s after reaching its position, and
all servos 1.5 s after a sequence ends: open panels then hold by servo friction.
At power-up, the firmware closes all dome panels (like `:CL00`), then releases them 0.7 s later.

### Panel groups

| Open | Close | Flutter | Panels |
| --- | --- | --- | --- |
| `:OP01` … `:OP18` | `:CL01` … `:CL18` | `:OF01` … `:OF10` | Groups 1–13 = Maestro channels 0–12; groups 14–18 = unused channels 19–23 |
| `:OP19` | `:CL19` | — | The 4 pie panels |
| `:OP20` | `:CL20` | — | Small, medium and big lower dome panels |

### Direct servo control

`<ch>` is the Maestro channel (0–23). Pulses are in microseconds, times in milliseconds.

| Command | Description |
| --- | --- |
| `:SQ<ch>,<pulse>` | Move immediately to `<pulse>` |
| `:SM<ch>,<pulse>` | Same as `:SQ` |
| `:SM<ch>,<time>,<pulse>` | Move to `<pulse>` over `<time>` ms (for example `:SM0,500,1500`) |
| `:SM<ch>,<delay>,<time>,<pulse>` | Same, after `<delay>` ms |
| `:SM<ch>,<delay>,<time>,<start>,<pulse>` | Same, starting from `<start>` |
| `:SL<ch>,<start>,<end>[,<neutral>[,<group>]]` | Change the closed (`start`) and open (`end`) pulses of a channel until the next reboot (`<group>` in decimal) |
| `:SF<easing>$<mask>` | Set the easing method (number) for all servos in `<mask>` (hexadecimal) |

The firmware clamps every target pulse between the channel's closed and open pulses (see
[hardware.md](hardware.md#maestro-channel-mapping), or the values set with `:SL`). To go further, widen
the range with `:SL` first. Per-channel limits in the Maestro remain a useful second safety net.

### Dynamic group animations

The `$` is part of the command. It is followed by a **servo mask in hexadecimal**, then optional
parameters: `<speedMin>,<speedMax>,<easingOn>,<easingOff>` (defaults 10, 50, none, none).

| Command | Animation |
| --- | --- |
| `:OP$<mask>` | Open |
| `:CL$<mask>` | Close |
| `:OC$<mask>` | Open then close |
| `:OCL$<mask>` | Open then close, long |
| `:OCR$<mask>` | Open/close repeated |
| `:OF$<mask>` | Flutter |
| `:OW$<mask>` | Wave |
| `:OWF$<mask>` | Fast wave |
| `:OWC$<mask>` | Open/close wave |
| `:OMA$<mask>` | Marching ants |
| `:OAP$<mask>` | Alternate |
| `:OD$<mask>` | Dance |
| `:OS$<mask>` | Shake |

Masks (combine by adding):

| Mask | Servos |
| --- | --- |
| `1` | Small panels (channels 0–2) |
| `2` | Medium panels (3–4) |
| `4` | Big panel (5) |
| `8` | Pie panels (6–9) |
| `10` | Top pie panel (12) |
| `20` | Mini panels (10–11) |
| `1000` | Holo horizontal servos |
| `2000` | Holo vertical servos |
| `4000` × 2ⁿ⁻¹ | Panel group n (group 1 = `4000`, group 2 = `8000`, group 3 = `10000`, …) |

Examples: `:OC$8` opens and closes the pie panels; `:OW$3F,20,80` waves all dome panels, slower than the default.

---

## Sequences (`:SE`, `$`)

At the end of every servo sequence the firmware moves all dome panels to closed, then disables all
servos 1.5 s later.

### Interpolated sequences (recommended)

The ESP32 drives each step at 125 ms, the reliable minimum measured for the dome panels.

| Command | Same as | Description |
| --- | --- | --- |
| `:SE22` | `:SE02` | Wave |
| `:SE23` | `:SE03` | Smirk wave (fast) |
| `:SE24` | `:SE04` | Open/close wave |
| `:SE25` | `:SE05` | Beep cantina: marching ants, logics, holo short circuit (15 s) |
| `:SE26` | `:SE06` | Short circuit: logics alarm (2 s), then failure, then panels (≈ 16 s) |
| `:SE27` | `:SE07` | Cantina: dance, disco logics (46 s) |
| `:SE28` | `:SE08` | Leia message (45 s), no panels |
| `:SE29` | `:SE09` | Disco: long disco panels, rainbow logics (45 s) |
| `:SE30` | `:SE50` | Scream, logics only |
| `:SE31` | `:SE51` | Scream, panels only |
| `:SE32` | `:SE52` | Slow wave |
| `:SE33` | `:SE53` | Smirk wave |
| `:SE34` | `:SE54` | Open wave |
| `:SE35` | `:SE55` | Marching ants |
| `:SE36` | `:SE56` | Faint (long open/close) |
| `:SE37` | `:SE57` | Rhythmic (long open/close) |
| `:SE38` | `:SE58` | One by one |

### Original sequences (reference)

Same animations with `speed=0`: the Maestro moves the servos at its own speed. Less reliable with the
current configuration.

| Command | Description |
| --- | --- |
| `:SE00` | Stop the current sequence |
| `:SE01` | Scream: panels and logics |
| `:SE02` … `:SE09` | See the interpolated table above |
| `:SE50` … `:SE58` | See the interpolated table above |

### Music sequences

| Command | Description |
| --- | --- |
| `$720` | Yoda "clear your mind": opens panel group 6, holo effect (15 s) |
| `$815` | Harlem Shake: fire on all logics, panels shake (≈ 30 s) |
| `$821` | Girl on Fire (≈ 55 s) |

### Music choreographies (`:MU`)

The Kyber starts the song and sends `:MUnn` at the same time. Each song is a timed list of panel, logic,
PSI and holo events (`src/music/`). Random holo moves (`*HA`, `*HV`) pause during the song and resume
after it. Any other sequence command stops the song; the droid is then reset (panels closed, logics normal).

| Command | Description |
| --- | --- |
| `:MU01`–`:MU98` | Play song `nn` (unknown numbers are ignored) |
| `:MU99` | Test choreography, no music (20 s) |
| `:MU00` | Stop the current song |

---

## Holo projectors (`*`)

Holo numbers: `01` front, `02` rear, `03` top.

| Command | Description |
| --- | --- |
| `*ON01` / `*ON02` / `*ON03` | Holo on (dim cycle, random color) |
| `*OF01` / `*OF02` / `*OF03` | Holo off |
| `*ST00` | Reset all holos |
| `*RD01` / `*RD02` / `*RD03` | One random move |
| `*HW01` / `*HW02` / `*HW03` | Wag (left/right) |
| `*HN01` / `*HN02` / `*HN03` | Nod (up/down) |
| `*HPS301` / `*HPS302` / `*HPS303` | Pulse |
| `*HPS601` / `*HPS602` / `*HPS603` | Rainbow |
| `*HP<p><nn>` | Fixed position `<p>` for holo `<nn>` (table below) |

| `<p>` | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Position | Down | Center | Up | Left | Up-left | Down-left | Right | Up-right | Down-right |

Example: `*HP102` = rear holo to center.

### Continuous animation

| Command | Description |
| --- | --- |
| `*HV01` / `*HV02` / `*HV03` | "R2 alive": random moves (slow 8–15 s, medium 3–8 s, fast 1–4 s) **and** blue↔white LED fades |
| `*HV00` | Stop: holos to center, LEDs off |
| `*HA01` / `*HA02` / `*HA03` | Random moves only (slow, medium, fast) |
| `*HZ00` | Stop moves, holos to center |

### Radar eye

| Command | Description |
| --- | --- |
| `*HRS3` | Pulse, random color |
| `*HRSR` | Pulse, red |
| `*HRS4` | Color cycle |
| `*HRS6` | Rainbow |
| `*OF04` | Off |

---

## Logic displays and PSI (`@`)

### Sequences

`T` = logic displays, `P` = PSI. Target: `0` all, `1` front, `2` rear.

| `<n>` | 1 | 2 | 3 | 4 | 5 | 6 | 11 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Sequence | Normal | Flash | Alarm | Failure | Scream (red alert) | Leia | March |

| Command | Description |
| --- | --- |
| `@0T<n>` / `@1T<n>` / `@2T<n>` | Logic displays: all / front / rear |
| `@0P<n>` / `@1P<n>` / `@2P<n>` | PSI: all / front / rear |

Example: `@0T5` = all logics in scream.

### Text and font

| Command | Description |
| --- | --- |
| `@1M<text>` | Front logic, top line: scroll `<text>` left |
| `@2M<text>` | Front logic, bottom line: scroll `<text>` left |
| `@3M<text>` | Rear logic: scroll `<text>` left |
| `@3P60` / `@3P61` | Rear logic font: Latin / Aurabesh |
| `@1P60` / `@1P61`, `@2P60` / `@2P61` | Front logic font: Latin / Aurabesh |

### Alternative holo commands

| Command | Description |
| --- | --- |
| `@6T1` / `@6D` | Front holo on / off |
| `@7T1` / `@7D` | Top holo on / off |
| `@8T1` / `@8D` | Rear holo on / off |
| `@HP<cmd>` | Raw holo command (Reeltwo `HP…` syntax) |

### Logic engine effects (`LE`)

Send with `@APLE…` or `~RTLE…`. Format: `LE[L]EECSNN`.

- **L — target** (optional). Without it the effect goes to every logic and PSI. With it: `1` front logic,
  `3` rear logic, `4` front PSI, `5` rear PSI. The target is only read when the full command has 9 characters or more.
- **EE — effect**

  | EE | Effect | EE | Effect |
  | --- | --- | --- | --- |
  | 00 | Normal | 12 | Mic bright |
  | 01 | Alarm | 13 | Mic rainbow |
  | 02 | Failure | 14 | Lights out |
  | 03 | Leia | 15 | Display text |
  | 04 | March | 16 | Text scroll left |
  | 05 | Single color | 17 | Text scroll right |
  | 06 | Flashing color | 18 | Text scroll up |
  | 07 | Flip flop | 19 | Roaming pixel |
  | 08 | Flip flop alt | 20 | Horizontal scan line |
  | 09 | Color swap | 21 | Vertical scan line |
  | 10 | Rainbow | 22 | Fire |
  | 11 | Red alert | 23 | PSI color wipe |
  |  |  | 24 | Pulse |
  |  |  | 99 | Random |

  Custom effects of this firmware (three-digit effect, target required): 101 Plasma, 102 Metaballs,
  103 Fractal, 104 Fade and scroll. An unknown effect number falls back to Normal.
- **C — color**: 1 red, 2 orange, 3 yellow, 4 green, 5 cyan, 6 blue, 7 purple, 8 magenta, 9 pink,
  0 effect default.
- **S — speed** (1–9, 5 = default) or microphone sensitivity for red alert and mic effects.
- **NN — duration** in seconds, `00` = continuous or effect default.

Leading zeros disappear because the value is read as a number.

| Example | Result |
| --- | --- |
| `@APLE51000` | Solid red |
| `@APLE54008` | Solid green for 8 s |
| `@APLE10500` | Alarm |
| `@APLE20000` | Failure |
| `@APLE30008` | Leia for 8 s |
| `@APLE40500` | March |
| `@APLE63315` | Flashing yellow, faster, 15 s |
| `@APLE100500` | Rainbow |
| `@APLE111300` | Red alert |
| `@APLE225000` | Fire |
| `@APLE3010003` | Rear logic: alarm, 3 s |
| `@APLE11015000` | Front logic: plasma |

---

## System commands (`#AP`)

| Command | Description |
| --- | --- |
| `#APRESTART` | Restart the ESP32 |
| `#APSTAT` | Print diagnostics on USB: uptime, last reset reason, heap, loop stack, average and maximum loop time since the previous `#APSTAT` |
