# Modifications locales de Reeltwo 23.5.3

Le firmware dépend de trois fichiers de Reeltwo modifiés localement. PlatformIO les télécharge dans
`.pio/libdeps/astropixelsplus/Reeltwo/`, qui est ignoré par git : un nouveau clone, un nettoyage de `.pio`
ou une mise à jour des bibliothèques les efface, et le projet ne compile plus.

Ce dossier en garde une copie versionnée, basée sur Reeltwo `23.5.3` (commit `16cdce2`).

| Fichier | Changement |
| --- | --- |
| `src/ServoDispatchMaestro.h` | Nouveau : pilote du Pololu Maestro (protocole Pololu sur Serial1) |
| `src/ServoDispatch.h` | Ajout de `virtual void setSequenceActive(bool)` |
| `src/dome/HoloLights.h` | `fCounter = millis()` dans le reset des effets |

`reeltwo-23.5.3-local.patch` contient le diff des deux fichiers modifiés (sans le nouveau fichier).

## Restaurer après un nouveau clone ou un nettoyage

1. Laisser PlatformIO télécharger les bibliothèques : `pio pkg install` (ou un premier `pio run`, qui échouera).
2. Copier les fichiers par-dessus la bibliothèque :

   ```bash
   bash patches/reeltwo/restore.sh
   ```

3. Recompiler : `pio run`.

À terme, ces fichiers devraient vivre dans le projet (`lib/` ou un fork de Reeltwo référencé par tag) pour que le
build n'ait plus besoin de cette étape manuelle (voir `docs/revue-firmware.md`, problème 1).
