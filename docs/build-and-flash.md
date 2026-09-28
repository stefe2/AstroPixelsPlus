# Build and flash

The project builds with [PlatformIO](https://platformio.org/) (VS Code extension or `pio` command line).
Target: `esp32dev`, Arduino framework, platform espressif32 5.2.0 (arduino-esp32 2.0.5).

## First build after cloning

The firmware needs three modified Reeltwo files that PlatformIO does not download
(see [patches/reeltwo](../patches/reeltwo/README.md)). Restore them once after cloning, and again whenever
`.pio/` is deleted or the libraries are reinstalled.

```bash
pio pkg install                   # download the libraries into .pio/libdeps
bash patches/reeltwo/restore.sh   # copy the modified Reeltwo files over them
pio run                           # build
```

Without the restore step the build stops with `ServoDispatchMaestro.h: No such file or directory`.

Expected result: RAM about 25.8 KB (7.9 %), flash about 386 KB (29.4 %), no warnings.

## Flash over USB

```bash
pio run -t upload
pio device monitor                # 115200 baud, exception decoder enabled
```

## Flash the prebuilt binary

`firmware/Astropixels.bin` holds bootloader, partition table and firmware in one file. Flash it at offset
`0x0` with any ESP32 web flasher or with esptool:

```bash
esptool.py --chip esp32 write_flash 0x0 firmware/Astropixels.bin
```

## Update the binaries in `firmware/`

After a build:

```bash
cp .pio/build/astropixelsplus/bootloader.bin .pio/build/astropixelsplus/partitions.bin \
   .pio/build/astropixelsplus/firmware.bin firmware/
python tools/merge_firmware.py    # rebuilds firmware/Astropixels.bin
```

`firmware/boot_app0.bin` comes from `framework-arduinoespressif32/tools/partitions/` and does not change.

## Regenerate the HTML command reference

After editing [commands.md](commands.md):

```bash
pip install markdown              # once
python tools/build_commands_html.py
```
