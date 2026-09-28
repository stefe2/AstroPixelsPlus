# Reeltwo 23.5.3 — copie locale modifiée

Copie du dossier `src/` de [Reeltwo](https://github.com/reeltwo/Reeltwo), tag `23.5.3` (commit `16cdce2`),
intégrée au projet pour que le build ne dépende plus d'un téléchargement ni d'une étape manuelle.
PlatformIO compile automatiquement les bibliothèques du dossier `lib/`.

Seuls `src/`, `LICENSE`, `README.md` et `library.properties` sont repris ; la documentation et les exemples
de Reeltwo ne sont pas nécessaires au build.

## Différences avec Reeltwo 23.5.3

| Fichier | Changement |
| --- | --- |
| `src/ServoDispatchMaestro.h` | Nouveau : pilote du Pololu Maestro (protocole Pololu sur Serial1). Cibles bornées aux limites du canal, temps sûrs au rollover de `millis()` |
| `src/ServoDispatch.h` | Ajout de `virtual void setSequenceActive(bool)` |
| `src/dome/HoloLights.h` | `fCounter = millis()` dans le reset des effets |
| `src/core/Marcduino.h` | `processCommand` : la correspondance la plus longue l'emporte (avant : la dernière déclarée) |
| `src/dome/LogicEngine.h` | `LogicEffectDefaultSelector` : `>=` au lieu de `>`, l'effet 25 lisait hors du tableau |

Pour voir le détail des changements, comparer avec la version d'origine :

```bash
git clone --branch 23.5.3 https://github.com/reeltwo/Reeltwo /tmp/Reeltwo
diff -ru /tmp/Reeltwo/src lib/Reeltwo/src
```

Toute modification future de Reeltwo se fait directement dans ce dossier et s'ajoute au tableau ci-dessus.
