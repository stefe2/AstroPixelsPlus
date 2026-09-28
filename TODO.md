# TODO — AstroPixelsPlus

Tâches ouvertes. Les numéros de problèmes renvoient à la [revue du firmware](docs/review/revue-firmware.md).
L'historique des étapes terminées est dans [docs/maestro-migration-history.md](docs/maestro-migration-history.md).

## 1. Tests matériels en attente

Ils confirment ou infirment les déductions de la revue. À faire avant de corriger le code.

- [ ] `:OP00` : les panneaux restent-ils ouverts, ou se referment-ils ~200 ms après ? (problème 4)
- [ ] `:SD5` : seul le servo 5 devient mou, ou tous les servos ? (problèmes 2 et 3)
- [ ] Contrôleur sur Serial2 (Kyber, Marcduino…) et fin de ligne utilisée dans le moniteur série : CR, LF ou CR+LF ? (problème 5)
- [ ] Maestro Control Center : enregistrer le fichier de réglages (*File → Save settings file*) et relever l'onglet *Errors* pendant que l'ESP32 tourne (problèmes 2 et 3)
- [ ] Holo arrière : vérifier si les axes H/V des canaux 17 et 18 sont inversés

Après avoir flashé les corrections déjà faites (section 2) :

- [ ] `~RTLE11050000` (effet 105) et `~RTLE1250000` (effet 25) : aucun redémarrage, les logics reviennent à l'effet normal (problème 6)
- [ ] `@1P61` puis `@1P60` : la police des logics avant change, le PSI ne passe pas en Leia (problème 14)
- [ ] `@1P6` et `@1P11` : toujours PSI Leia et PSI March (non-régression)

## 2. Corrections du firmware (plan d'action de la revue)

Dans l'ordre recommandé :

- [x] Intégrer les modifications de Reeltwo au build (`lib/Reeltwo/`) et épingler toutes les lib_deps (problème 1)
- [ ] Désactivation des servos Maestro avec `Set Target` = 0 au lieu de `0x60` ; arrêt des holos seulement sur la transition actif → inactif (problèmes 2 et 3)
- [ ] Fermeture automatique de fin de séquence seulement pour les séquences qui doivent finir fermées (problème 4)
- [ ] Parseur série : ignorer une ligne vide, lire tous les caractères disponibles (bornés) à chaque tour (problèmes 5 et 7)
- [x] `CustomLogicEffectSelector` : `<=` → `<` (problème 6, plantage sur l'effet 105) ; même erreur corrigée dans `LogicEffectDefaultSelector` (effet 25)
- [x] `@1P60`/`@1P61`/`@2P60`/`@2P61` : la correspondance la plus longue l'emporte désormais (problème 14)
- [ ] `enableLoopWDT()` et commande de diagnostic `#APSTAT` (problème 8)
- [ ] Bornage des impulsions `:SM`/`:SQ`/`:SL`, comparaisons de temps sûres au rollover de `millis()` (problèmes 10 et 11)
- [ ] Réduire le débit vers le Maestro, seulement si les mesures le justifient (problème 9)
- [ ] Nettoyage : `:SE36`/`:SE56` lancés deux fois, `@4S3` mort dans `:CL00`, SPIFFS inutile, flag PSRAM, handlers `:OX$` dupliqués (problème 13)

## 3. Holos (ancienne étape 9)

- [ ] Tester `*RD01`–`*RD03`, `*HA01`–`*HA03`, `*HV01`–`*HV03` sur le droïde
- [ ] Vérifier la fluidité des fondus LED
- [ ] Tester les commandes directes (`~RTHPA000|0`, autres modes)

## 4. Fonctionnalités à venir

- [ ] Pass-through Maestro externe sur GPIO 18 (MaestroCommandRouter). Conception détaillée dans l'annexe « Étape 11 » de [l'historique](docs/maestro-migration-history.md)
- [ ] Commenter `ServoDispatchMaestro.h` (Doxygen)
- [ ] Réparer la compilation avec Wi-Fi (`#define USE_WIFI`) : `src/WebPages.h` utilise encore `PREFERENCE_MARCSERIAL*` et `MARC_SERIAL_*`, supprimés au commit 41139a9
- [ ] Tag de version une fois les corrections validées (ex. `v1.0.0-maestro`)

## 5. Matériel — nouveau PCB (plus tard)

Remplacer la carte AstroPixels actuelle, très basique et sans protection, par un PCB dédié à la configuration Maestro.
Pistes issues de la revue (à préciser au moment de la conception) :

- [ ] Adaptateur de niveau 3,3 V → 5 V sur les sorties LED WS2812 (ex. 74AHCT125) — aucun scintillement aujourd'hui, mais aucune marge
- [ ] Résistance série (~330 Ω) et protection sur chaque ligne de données LED
- [ ] Connecteur dédié au Maestro : TX (GPIO 19), VIN et GND (aujourd'hui via AUX5)
- [ ] Alimenter la logique du Maestro au-dessus de 5 V (VIN accepte 5–16 V ; aujourd'hui au minimum)
- [ ] Connecteur Serial2 (contrôleur maître 3,3 V, signal + GND)
- [ ] Protection de l'entrée d'alimentation (inversion de polarité, fusible)
- [ ] Alimentation servos 6 V toujours séparée de la logique (configuration actuelle validée)
