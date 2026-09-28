#!/usr/bin/env bash
# Recopie les modifications locales de Reeltwo dans la bibliothèque téléchargée par PlatformIO.
set -euo pipefail

cd "$(dirname "$0")/../.."
DEST=.pio/libdeps/astropixelsplus/Reeltwo/src

if [ ! -d "$DEST" ]; then
    echo "Reeltwo introuvable dans $DEST : lancer d'abord 'pio pkg install'." >&2
    exit 1
fi

cp patches/reeltwo/src/ServoDispatchMaestro.h "$DEST/"
cp patches/reeltwo/src/ServoDispatch.h "$DEST/"
cp patches/reeltwo/src/dome/HoloLights.h "$DEST/dome/"
echo "Modifications Reeltwo restaurées dans $DEST"
