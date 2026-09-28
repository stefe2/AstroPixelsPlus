# TODO — AstroPixelsPlus

Tâches ouvertes. Les numéros de problèmes renvoient à la [revue du firmware](docs/review/revue-firmware.md).
L'historique des étapes terminées est dans [docs/maestro-migration-history.md](docs/maestro-migration-history.md).

## 1. Tests matériels

### Session du 2026-09-27 (firmware de la branche `nettoyage/revue-firmware`, commandes par USB)

Outils : commandes envoyées sur le port USB de l'ESP32 ; état du Maestro lu avec `UscCmd --status`
(USB natif) ; trames ESP32 → Maestro capturées sur le port Command du Maestro, qui les recopie en mode UART.

Déductions de la revue, maintenant mesurées :

- [x] Au repos, le Maestro est en erreur `0x0010 ERROR_SERIAL_PROTOCOL` en permanence (voyant rouge allumé) :
  l'ESP32 envoie ~7 trames `AA 01 60 <canal> 00 00` par seconde, surtout pour les holos 13–18 (problèmes 2 et 3 confirmés)
- [x] Chaque trame `0x60` met le Maestro en erreur et, avec « On startup or error = Off » sur les 24 canaux,
  coupe **tous** les canaux : un panneau est coupé 0,2 à 0,3 s après son arrivée, et `:CL00` est coupé 70 ms après l'envoi
  (problème 2 confirmé ; `:SD5` n'est plus nécessaire)
- [x] `:OP00` : ouverture, puis fermeture commandée ~250 ms après ; les panneaux s'ouvrent à peine (problème 4 confirmé, observé)
- [x] Réglages du Maestro relevés : [docs/maestro/maestro_settings.txt](docs/maestro/maestro_settings.txt)
- [x] Holo arrière : dans le Maestro, 17 = `RHP-H` et 18 = `RHP-V`, comme `assignServos(17, 18)` ; c'est le commentaire
  de `servoSettings[]` qui était inversé ; confirmé à l'œil : le canal 17 bouge à l'horizontale
- [x] Fin de ligne : le parseur accepte maintenant CR, LF et CR+LF, la question n'est plus bloquante

Corrections déjà faites, validées :

- [x] `~RTLE11050000` (effet 105) et `~RTLE1250000` (effet 25) : aucun redémarrage (problème 6)
- [x] `@1P61` : le PSI avant ne passe plus en Leia ; police Aurabesh visible sur les logics avant (problème 14)
- [x] `@1P6` : PSI avant en Leia (non-régression)
- [x] `#APSTAT` en CR+LF : une seule exécution ; rafale de 3 commandes : les 3 exécutées (problèmes 5 et 7)
- [x] `:SM0,500,2400` : l'ESP32 n'envoie jamais plus de 1840 µs (problème 11)
- [x] `:SE56` : un seul cycle ouverture-fermeture (problème 13)
- [x] `#APRESTART` : redémarrage propre, `Reset reason: SOFTWARE`
- [x] Durée de boucle : au repos moyenne 297–320 µs, max 8–10 ms ; pendant `:SE22` moyenne 363 µs, max 10,3 ms.
  Le maximum vient du rafraîchissement des LED, pas du Maestro : problème 9 sans objet. Heap libre 328 Ko (min 322 Ko),
  pile loopTask 6,5 Ko libres sur 8 Ko

Après les corrections 2, 3 et 4 (commit 94cbf35, flashé le même jour) :

- [x] Au repos : `errors: 0x0000`, voyant rouge éteint, aucune trame envoyée au Maestro (avant : ~7 trames `0x60`/s)
- [x] `:SM0,500,1500` : panneau tenu 0,7 s à l'arrivée puis relâché par Set Target 0 sur ce seul canal, aucune erreur
- [x] `:OP00` : les panneaux restent ouverts, relâchés après 1,5 s, tiennent par friction (observé)
- [x] `:CL00` : fermeture complète de tous les panneaux, porte 11 comprise, tenus 0,7 s puis relâchés, aucune erreur
- [x] `*HA01` : les holos bougent, 431 lectures `UscCmd` sans erreur ; `*HZ00` les recentre et les arrête (canaux 13–18 à 0)
- [x] `@1P6` puis `@1P1` : le PSI avant revient à son effet de démarrage (color wipe bleu/rouge), sans mélange avec Leia
- [x] `:SE02` : séquence complète, tous les panneaux refermés à la fin, `errors: 0x0000`, 24 canaux relâchés
- [x] `@1P11` : PSI avant en March (non-régression)
- [x] `:OW$3F` (vague) et `:OP$3F,300,300` (ouverture, panneaux restés ouverts) : tous les panneaux du dôme, car
  `$3F` est un masque de types de panneaux, pas de numéros ; `:OC$8` n'actionne que les 4 pie panels ; aucune erreur
- [x] `:SE07` (Cantina, 46 s) : aucun redémarrage (uptime continu, `Reset reason: POWERON` inchangé), boucle moy. 1,4 ms,
  max 17 ms, pile loopTask 6,1 Ko libres, `errors: 0x0000` (problème 8)
- [x] `$815` (Harlem Shake) : aucun redémarrage ; mais les logics restaient en arc-en-ciel à la fin (`LE000000|0`
  de `resetSequence()` ignorée, voir section 2). Après correction : retour seul au scintillement, boucle moy. 448 µs,
  max 10,9 ms (17,1 ms avec le `printf` des holos), `errors: 0x0000`
- [x] `:SE06` : alarme 2 s puis panne, retour seul à la normale. `$815` : clignotement rouge (effet 7, d'origine,
  conservé) puis feu avec l'agitation des panneaux, retour seul à la normale

Encore à faire :

- [ ] Contrôleur Serial2 (Kyber) : les commandes habituelles fonctionnent, une rafale n'en perd aucune (problème 7)
- [x] Porte 11 (canal 11) : se ferme complètement à 1552 µs, sans forcer
- [x] `@1M` seul : le texte défile une fois après ~2 s d'écran noir (pas un défaut, il avait été manqué)
- [x] Holo arrière : le canal 17 est l'axe horizontal

## 2. Corrections du firmware (plan d'action de la revue)

Dans l'ordre recommandé :

- [x] Intégrer les modifications de Reeltwo au build (`lib/Reeltwo/`) et épingler toutes les lib_deps (problème 1)
- [x] Désactivation des servos Maestro avec `Set Target` = 0 au lieu de `0x60` ; holos relâchés par le pilote après chaque mouvement, `HoloLightsWithAutoStop` supprimée (problèmes 2 et 3)
- [x] Fermeture automatique de fin de séquence seulement si la séquence finit fermée ou est interrompue ; panneaux ouverts relâchés, tenus par friction (problème 4)
- [x] Parseur série : ignorer une ligne vide, lire tous les caractères disponibles (bornés) à chaque tour (problèmes 5 et 7)
- [x] `CustomLogicEffectSelector` : `<=` → `<` (problème 6, plantage sur l'effet 105) ; même erreur corrigée dans `LogicEffectDefaultSelector` (effet 25)
- [x] `@1P60`/`@1P61`/`@2P60`/`@2P61` : la correspondance la plus longue l'emporte désormais (problème 14)
- [x] `enableLoopWDT()` et commande de diagnostic `#APSTAT` (problème 8)
- [x] Bornage des impulsions `:SM`/`:SQ` aux limites du canal, comparaisons de temps sûres au rollover de `millis()` (problèmes 10 et 11)
- [x] ~~Réduire le débit vers le Maestro~~ : sans objet, la boucle reste à 363 µs en moyenne pendant un mouvement (problème 9)
- [x] Nettoyage : `:SE36`/`:SE56` lancés deux fois, `@4S3` mort dans `:CL00`, SPIFFS inutile, flag PSRAM, handlers `:OX$` dupliqués (problème 13)
- [x] Canal 11 (porte mini du PSI avant) : position fermée 2552 → 1552 µs (limite du Maestro, fermeture vérifiée sur le droïde)
- [x] `@1P1`, `@2P1` et `resetSequence()` (`LE000000`) remettent les PSI en effet « Normal » (scintillement) au lieu de leur effet de démarrage (color wipe 23)
- [x] `resetSequence()` : `LE000000|0` → `LE000000`. Une commande LE de 9 caractères ou plus vise l'appareil dont
  l'ID est le premier chiffre (0 : aucun), les logics n'étaient jamais remises à zéro (code d'origine)
- [x] `printf("COMMAND: …")` de débogage retiré de `HoloLights::handleCommand` (3 lignes par commande)
- [x] RX de Serial2 (GPIO 16) : rappel au niveau haut, l'entrée ne flotte plus sans contrôleur branché
- [x] `:SE06`/`:SE26` : vraie alarme 2 s (`LE010003`) avant la panne ; `$815` : feu (`LE220000`) au lieu de l'arc-en-ciel
- [x] Commentaire de `servoSettings[]` : canaux 17/18 du holo arrière inversés (17 = horizontal)

## 3. Holos (ancienne étape 9)

- [ ] Tester `*RD01`–`*RD03`, `*HA01`–`*HA03`, `*HV01`–`*HV03` sur le droïde
- [ ] Vérifier la fluidité des fondus LED
- [ ] Tester les commandes directes (`~RTHPA000|0`, autres modes)

## 4. Fonctionnalités à venir

- [ ] Pass-through Maestro externe sur GPIO 18 (MaestroCommandRouter). Conception détaillée dans l'annexe « Étape 11 » de [l'historique](docs/maestro-migration-history.md)
- [ ] Commenter `ServoDispatchMaestro.h` (Doxygen)
- [x] Compilation avec Wi-Fi (`-DUSE_WIFI`) réparée : réglages Serial2 obsolètes retirés de la page web `/serial`
  (compilation seulement, Wi-Fi non testé sur le droïde)
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
