#!/usr/bin/env python3
"""
Fusionne les fichiers .bin ESP32 en un seul binaire pour le web flasher.
Usage : python merge_firmware.py
Output: Firmware/merged_firmware.bin (à flasher à l'offset 0x0000)
"""

import os
import sys

FILES = [
    ("Firmware/bootloader.bin",  0x1000),   #  4096
    ("Firmware/partitions.bin",  0x8000),   # 32768
    ("Firmware/boot_app0.bin",   0xE000),   # 57344
    ("Firmware/firmware.bin",    0x10000),  # 65536
]

OUTPUT = "Firmware/Astropixels.bin"

def merge():
    # Vérifier que tous les fichiers existent
    missing = [f for f, _ in FILES if not os.path.isfile(f)]
    if missing:
        print("Fichiers manquants :")
        for f in missing:
            print(f"  {f}")
        sys.exit(1)

    # Calculer la taille totale nécessaire
    total_size = max(offset + os.path.getsize(path) for path, offset in FILES)

    # Buffer rempli de 0xFF (état d'un flash effacé)
    merged = bytearray(b'\xFF' * total_size)

    for path, offset in FILES:
        with open(path, 'rb') as f:
            data = f.read()
        merged[offset:offset + len(data)] = data
        print(f"  {path:35s} @ 0x{offset:05X}  ({len(data):>8} octets)")

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, 'wb') as f:
        f.write(merged)

    print(f"\nFusionné : {OUTPUT}  ({total_size} octets)")
    print("Flasher ce fichier à l'offset 0x0000 avec le web flasher.")

if __name__ == "__main__":
    merge()
