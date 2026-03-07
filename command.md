# Commandes AstroPixelsPlus

## Panneaux (Serial2 / USB, préfixe `:`)

### Commandes globales
-----------------------------------------------------------------------
| Commande | Description                                              |
|----------|----------------------------------------------------------|
| `:CL00`  | Fermer tous les panneaux risque brownout si alim commune |
| `:OP00`  | Ouvrir tous les panneaux                                 |
| `:OF00`  | Flutter (tremblement) tous les panneaux                  |
| `:ST00`  | Stop & désactiver TOUS les servos (mous, sans couple)    |
| `:SD<n>` | Désactiver UN servo (ex: `:SD0`, `:SD4`, `:SD12`)        |
-----------------------------------------------------------------------

### Mouvement direct
-------------------------------------------------------------------------------------------
| Commande              | Description                                                     |
|-----------------------|-----------------------------------------------------------------|
| `:SM<ch>,<dur>,<pos>` | Mouvement direct (ex: `:SM0,500,1500` = canal 0, 500ms, 1500µs) |
| `:SF<ch>$<easing>`    | Définir méthode easing d'un servo                               |
| `:SQ<ch>,<pos>`       | Position immédiate sans animation                               |
| `:SL<ch>,<min>,<max>` | Définir les limites min/max d'un servo                          |
-------------------------------------------------------------------------------------------

### Groupes individuels (01 à 18 + 19/20)
---------------------------------------------------------------------------------------
| Ouvrir          | Fermer          | Flutter         | Description                   |
|-----------------|-----------------|-----------------|-------------------------------|
| `:OP01`…`:OP18` | `:CL01`…`:CL18` | `:OF01`…`:OF10` | Groupes 1 à 18                |
| `:OP19`         | `:CL19`         | —               | Panneaux du haut (pie panels) |
| `:OP20`         | `:CL20`         | —               | Panneaux du bas (dome panels) |
---------------------------------------------------------------------------------------

### Animations dynamiques de groupe
> Remplacer `$` par le numéro de groupe en hexadécimal (ex: `1`, `2`, `FF`…)

--------------------------------------
| Commande | Description             |
|----------|-------------------------|
| `:OC$`   | Open/Close groupe       |
| `:OCL$`  | Open/Close long         |
| `:OCR$`  | Open/Close répété       |
| `:OF$`   | Flutter groupe          |
| `:OW$`   | Wave groupe             |
| `:OWF$`  | Fast wave groupe        |
| `:OWC$`  | Open/Close wave         |
| `:OMA$`  | Marching ants           |
| `:OAP$`  | Alternance              |
| `:OD$`   | Dance                   |
| `:OS$`   | Shake                   |
| `:OP$`   | Ouvrir groupe dynamique |
| `:CL$`   | Fermer groupe dynamique |
--------------------------------------

**Paramètres optionnels** (ajoutés après la virgule) : `<speedMin>,<speedMax>,<easingOn>,<easingOff>`
Exemple : `:OC1,10,50,1,2`

---

## Séquences (préfixe `:SE`)

### Contrôle
| Commande | Description            |
|----------|------------------------|
| `:SE00`  | Stop séquence en cours |

### Séquences interpolées — **recommandées** (speed=125ms, fiables)
> Ces séquences guident les servos étape par étape depuis l'ESP32 (interpolation 125ms/étape).
> C'est le minimum physique fiable pour les panneaux du dôme.

---------------------------------------------------------------------------------
| Commande | Équivalent | Description                                           |
|----------|------------|-------------------------------------------------------|
| `:SE22`  | SE02       | Wave                                                  |
| `:SE23`  | SE03       | Smirk Wave (rapide)                                   |
| `:SE24`  | SE04       | Open/Close Wave                                       |
| `:SE25`  | SE05       | Cantina bip (marching ants 15s)                       |
| `:SE26`  | SE06       | Short Circuit (spark + failure 8s)                    |
| `:SE27`  | SE07       | Cantina complète (dance 46s)                          |
| `:SE28`  | SE08       | Leia Message (45s) — identique, pas de servos panneau |
| `:SE29`  | SE09       | Disco                                                 |
| `:SE30`  | SE50       | Scream logics uniquement — identique, pas de panneaux |
| `:SE31`  | SE51       | Scream panneaux uniquement                            |
| `:SE32`  | SE52       | Wave lent                                             |
| `:SE33`  | SE53       | Smirk Wave                                            |
| `:SE34`  | SE54       | Open Wave                                             |
| `:SE35`  | SE55       | Marching Ants                                         |
| `:SE36`  | SE56       | Faint (open/close long)                               |
| `:SE37`  | SE57       | Rythmique                                             |
| `:SE38`  | SE58       | One By One (panneau par panneau)                      |
---------------------------------------------------------------------------------

### Séquences originales — référence (speed=0, Maestro gère le mouvement)
> Conservées comme référence. Le Maestro contrôle la vitesse de ses propres servos.
> Moins fiables avec la configuration actuelle (speed et accélération Maestro = 0).

-------------------------------------------------------
| Commande | Description                              |
|----------|------------------------------------------|
| `:SE01`  | Scream (panneaux + logics)               |
| `:SE02`  | Wave                                     |
| `:SE03`  | Smirk Wave (rapide)                      |
| `:SE04`  | Open/Close Wave                          |
| `:SE05`  | Cantina bip (marching ants 15s)          |
| `:SE06`  | Short Circuit (spark + failure 8s)       |
| `:SE07`  | Cantina complète (dance 46s)             |
| `:SE08`  | Leia Message (45s)                       |
| `:SE09`  | Disco                                    |
| `:SE50`  | Scream logics uniquement (sans panneaux) |
| `:SE51`  | Scream panneaux uniquement               |
| `:SE52`  | Wave lent                                |
| `:SE53`  | Smirk Wave                               |
| `:SE54`  | Open Wave                                |
| `:SE55`  | Marching Ants                            |
| `:SE56`  | Faint (open/close long)                  |
| `:SE57`  | Rythmique                                |
| `:SE58`  | One By One (panneau par panneau)         |
-------------------------------------------------------

> **Note :** À la fin de chaque séquence, tous les panneaux sont automatiquement fermés via `moveServosTo(ALL_DOME_PANELS_MASK, 125, 0.0)`, puis les servos sont désactivés après 1,5 s (sans risque de brownout).

---

## Holos (préfixe `*`)

### ON / OFF
----------------------------------
| Commande | Description         |
|----------|---------------------|
| `*ON01` | Front Holo ON        |
| `*OF01` | Front Holo OFF       |
| `*ON02` | Rear Holo ON         |
| `*OF02` | Rear Holo OFF        |
| `*ON03` | Top Holo ON          |
| `*OF03` | Top Holo OFF         |
| `*ST00` | Reset tous les holos |
----------------------------------

### Mouvements
--------------------------------------------------
| Commande | Description                         |
|----------|-------------------------------------|
| `*RD01` | Mouvement aléatoire Front (one-shot) |
| `*RD02` | Mouvement aléatoire Rear (one-shot)  |
| `*RD03` | Mouvement aléatoire Top (one-shot)   |
| `*HW01` | Wag (gauche/droite) Front            |
| `*HW02` | Wag Rear                             |
| `*HW03` | Wag Top                              |
| `*HN01` | Nod (haut/bas) Front                 |
| `*HN02` | Nod Rear                             |
| `*HN03` | Nod Top                              |
--------------------------------------------------

### R2 Vivant — Tout-en-un
> Active simultanément les mouvements aléatoires continus ET les LEDs bleu↔blanc avec fondu doux (1.5s fade in/out, 5–10s allumé, 3–9s éteint). Chaque holo est indépendant et décalé.

------------------------------------------------------------
| Commande | Description                                   |
|----------|-----------------------------------------------|
| `*HV01` | **Démarrer Lent** — mouvements 8–15s + LEDs    |
| `*HV02` | **Démarrer Moyen** — mouvements 3–8s + LEDs    |
| `*HV03` | **Démarrer Rapide** — mouvements 1–4s + LEDs   |
| `*HV00` | **Stopper** — retour au centre + LEDs éteintes |
------------------------------------------------------------

### HoloAlive — Mouvement continu (contrôle fin)
> Anime les 3 holos indépendamment vers des positions aléatoires en boucle. Chaque holo a son propre timer décalé (effet organique). LEDs non affectées.

----------------------------------------------------------------
| Commande | Description                                       |
|----------|---------------------------------------------------|
| `*HA01`  | Démarrer mode **Lent** (8–15s entre mouvements)   |
| `*HA02`  | Démarrer mode **Moyen** (3–8s entre mouvements)   |
| `*HA03`  | Démarrer mode **Rapide** (1–4s entre mouvements)  |
| `*HZ00`  | **Stopper** — tous les holos reviennent au centre |
----------------------------------------------------------------

### Effets lumineux
-----------------------------
| Commande  | Description   |
|-----------|---------------|
| `*HPS301` | Pulse Front   |
| `*HPS302` | Pulse Rear    |
| `*HPS303` | Pulse Top     |
| `*HPS601` | Rainbow Front |
| `*HPS602` | Rainbow Rear  |
| `*HPS603` | Rainbow Top   |
-----------------------------

### Positions fixes (xx = 01 Front / 02 Rear / 03 Top)
--------------------------
| Commande | Description |
|----------|-------------|
| `*HP0xx` | Bas         |
| `*HP1xx` | Centre      |
| `*HP2xx` | Haut        |
| `*HP3xx` | Gauche      |
| `*HP4xx` | Haut-gauche |
| `*HP5xx` | Bas-gauche  |
| `*HP6xx` | Droite      |
| `*HP7xx` | Haut-droite |
| `*HP8xx` | Bas-droite  |
--------------------------

Exemples : `*HP001` = Front bas, `*HP102` = Rear centre, `*HP203` = Top haut

### Radar Eye
-----------------------------
| Commande | Description    |
|----------|----------------|
| `*HRS3` | Pulse (couleur) |
| `*HRSR` | Pulse rouge     |
| `*HRS6` | Rainbow         |
| `*HRS4` | Cycle couleurs  |
| `*OF04` | Radar Eye OFF   |
-----------------------------

---

## Logics & PSI (préfixe `@`)

### Séquences (T = Logic Display, P = PSI)
> `0` = tous | `1` = Front | `2` = Rear

-----------------
| N° | Séquence |
|----|----------|
| 1  | Normal   |
| 2  | Flash    |
| 3  | Alarm    |
| 4  | Failure  |
| 5  | Scream   |
| 6  | Leia     |
| 11 | March    |
-----------------

-----------------------------------------------
| Commande | Description                      |
|----------|----------------------------------|
| `@0T<n>` | Tous les logics : séquence n     |
| `@1T<n>` | Front Logic Display : séquence n |
| `@2T<n>` | Rear Logic Display : séquence n  |
| `@0P<n>` | Tous les PSI : séquence n        |
| `@1P<n>` | Front PSI : séquence n           |
| `@2P<n>` | Rear PSI : séquence n            |
-----------------------------------------------

Exemples : `@0T1` = tous logics normal, `@1T5` = Front scream, `@2P6` = Rear PSI Leia

### Holos via Marcduino
--------------------------------------
| Commande   | Description           |
|------------|-----------------------|
| `@6T1`     | Front Holo ON         |
| `@6D`      | Front Holo OFF        |
| `@7T1`     | Top Holo ON           |
| `@7D`      | Top Holo OFF          |
| `@8T1`     | Rear Holo ON          |
| `@8D`      | Rear Holo OFF         |
| `@HP<cmd>` | Commande holo directe |
--------------------------------------

---

## Commandes système

---------------------------------------------------------------------
| Commande     | Description                                        |
|--------------|----------------------------------------------------|
| `~RT<cmd>`   | Commande directe Reeltwo                           |
| `@AP<cmd>`   | Commande directe MD vers AstroPixels               |
| `#APWIFI`    | Toggle WiFi ON/OFF *(WiFi actuellement désactivé)* |
| `#APZERO`    | Effacer toutes les préférences NVS (reset usine)   |
| `#APRESTART` | Redémarrer l'ESP32                                 |
---------------------------------------------------------------------

---

## Récapitulatif des préfixes

-----------------------------------------------------------
| Préfixe | Destination                                   |
|---------|-----------------------------------------------|
| `:`     | Panneaux / Séquences / Servo                  |
| `*`     | Holos (Marcduino standard)                    |
| `@`     | Logics / PSI / Holos alternatif               |
| `~RT`   | Reeltwo direct                                |
| `@AP`   | AstroPixels direct                            |
| `#AP`   | Commandes système AstroPixelsPlus             |
| `$`     | Séquences musicales (ex: `$815` Harlem Shake) |
-----------------------------------------------------------

---

*Projet: AstroPixelsPlus - ESP32 + Pololu Maestro 24 canaux*
*Serial2: GPIO 16/17, 9600 bauds (Marcduino)*
*Serial1: GPIO 18/19, 115200 bauds (Maestro servos)*

---

## Configuration Pololu Maestro 24 canaux

### Paramètres globaux (Serial Settings)
----------------------------------------
| Paramètre    | Valeur                |
|--------------|-----------------------|
| Serial mode  | UART, fixed baud rate |
| Baud rate    | 115200                |
| Device ID    | 1                     |
| CRC disabled | X                     |
----------------------------------------

### Vitesse et accélération des servos

Les unités Maestro sont : **Speed** en 0.25 µs / 10 ms, **Acceleration** en 0.25 µs / 10 ms².

Le déplacement typique d'un panneau est ~850 µs (ex : 1350 → 2200 µs).
 ---------------------------------------------------
| Speed | Vitesse réelle     | Temps pour 850 µs    |
|-------|--------------------|----------------------|
| 0     | illimitée (actuel) | ~100–200 ms physique |
| 40    | 1 µs/ms            | ~850 ms              |
| 80    | 2.5 µs/ms          | ~420 ms              |
| 120   | 3 µs/ms            | ~280 ms              |
 ---------------------------------------------------
**Valeurs recommandées à tester — canaux 0–12 (panneaux) :**
- **Speed : 80** → ~420 ms, naturel et fiable
- **Acceleration : 5** → rampe douce, évite chocs mécaniques et pics de courant

**Canaux 13–18 (servos holos) :** laisser Speed=0 (illimité) pour mouvement vif.

### Mapping des canaux
----------------------------------------
| Canal | Servo        | Type          |
|-------|--------------|---------------|
| 0     | Door 4       | SMALL_PANEL   |
| 1     | Door 3       | SMALL_PANEL   |
| 2     | Door 2       | SMALL_PANEL   |
| 3     | Door 1       | MEDIUM_PANEL  |
| 4     | Door 5       | MEDIUM_PANEL  |
| 5     | Door 9       | BIG_PANEL     |
| 6     | Pie 1        | PIE_PANEL     |
| 7     | Pie 2        | PIE_PANEL     |
| 8     | Pie 3        | PIE_PANEL     |
| 9     | Pie 4        | PIE_PANEL     |
| 10    | Mini 2       | MINI_PANEL    |
| 11    | Mini PSI     | MINI_PANEL    |
| 12    | Top Center   | TOP_PIE_PANEL |
| 13    | Front Holo H | HOLO_HSERVO   |
| 14    | Front Holo V | HOLO_VSERVO   |
| 15    | Top Holo H   | HOLO_HSERVO   |
| 16    | Top Holo V   | HOLO_VSERVO   |
| 17    | Rear Holo H  | HOLO_HSERVO   |
| 18    | Rear Holo V  | HOLO_VSERVO   |
----------------------------------------
