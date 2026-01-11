# TODO - Migration vers Pololu Maestro 24 canaux

## 📋 Objectif
Remplacer les contrôleurs PCA9685 (I2C) par un Pololu Maestro 24 canaux (Serial) pour le contrôle des servomoteurs.

## 📐 Schéma de câblage
**Référence:** [Wiring-Diagram.png](Wiring-Diagram.png)

Le schéma de câblage contient:
- Connexions ESP32 ↔ Maestro (Serial1: GPIO 18/19)
- Connexions ESP32 ↔ Marcduino (Serial2: GPIO 16/17)
- Alimentation servos et logique
- Mapping physique des 19 servos sur le Maestro

⚠️ **Vérifier le schéma avant toute modification matérielle!**

---

## ✅ Étape 1 : Préparation (TERMINÉ)
- [x] Analyser l'architecture actuelle ServoDispatch
- [x] Vérifier les ports série disponibles
- [x] Identifier Serial1 comme disponible (son désactivé)
- [x] Fixer la compilation (FastLED 3.7.0)
- [x] Créer le dépôt GitHub (https://github.com/stefe2/AstroPixelsPlus)
- [x] Créer la branche feature/maestro-servo-controller
- [x] **Tester communication série Serial Monitor**
  - Module externe → Serial USB (115200 bauds) ✅
  - Commandes Marcduino fonctionnelles (`:OP00`, `:CL00`, etc.) ✅
  - Pass-through Serial → Serial2 actif ✅
  - Configuration PlatformIO monitor validée ✅

### 📝 Notes communication série
- **Serial Monitor PlatformIO**: Utiliser terminal `pio device monitor` pour saisie interactive
- **Baudrate module externe testé**: 115200 bauds (compatible avec futur Maestro)
- **Pass-through actif**: `MARC_SERIAL_PASS = true` → Commandes USB envoyées aussi à Serial2
- **Line ending**: Newline (LF) ou Both NL & CR pour terminer commandes
- **Commandes testées**: `:OP00` (open panels), `:CL00` (close panels) fonctionnelles

---

## ✅ Étape 2 : Ajouter LED heartbeat (TERMINÉ)
- [x] Ajouter `#define LED_BUILTIN 2` (ligne 190)
- [x] Initialiser LED dans setup() (ligne 617-619)
- [x] Faire clignoter LED dans loop() (ligne 1102-1109)
- [x] Compiler et tester
- [x] **Commit changements** ✅

**Implémentation:**
- LED clignote à 1Hz (toutes les 500ms)
- GPIO 2 = LED onboard ESP32
- Permet monitoring visuel du fonctionnement CPU
- Compilation réussie: RAM 19.2%, Flash 78.2%
- Upload et test sur ESP32 validé ✅

---

## ✅ Étape 3 : Configuration Maestro dans AstroPixelsPlus.ino (TERMINÉ)

**🎯 Objectif:** Ajouter la configuration sans casser le code actuel (garde PCA9685 actif pour l'instant)

### 3.1 Ajouter defines Maestro (ligne ~130-145)
```cpp
// Serial1 pour Maestro (anciennement son)
#define MAESTRO_SERIAL Serial1
#define MAESTRO_RX_PIN PIN_AUX4  // GPIO 18
#define MAESTRO_TX_PIN PIN_AUX5  // GPIO 19
#define MAESTRO_BAUD 115200      // 115200 ou 9600
```
- [x] Ajouter defines après COMMAND_SERIAL ✅
- [x] Commenter (ne pas supprimer) ancien code SOUND_SERIAL ✅

### 3.2 Ajouter directive de compilation (ligne ~22-28)
```cpp
// Choix du contrôleur servo (un seul actif)
// #define USE_SERVO_DIRECT
#define USE_SERVO_PCA9685  // Actuel - garde pour l'instant
// #define USE_SERVO_MAESTRO  // Nouveau - à activer plus tard
```
- [x] Ajouter bloc choix contrôleur ✅
- [x] Garder PCA9685 actif pour compilation ✅

### 3.3 Désactiver code son dans setup()
- [x] Commenter bloc init SOUND_SERIAL (ligne 662-669) ✅
- [x] Ajouter message debug "Serial1 reserved for Maestro" ✅

### 3.4 Test compilation
- [x] Compiler avec configuration actuelle (PCA9685) ✅
- [x] Vérifier aucune erreur ✅
- [x] Compilation réussie: RAM 19.2%, Flash 78.2% ✅

✅ **Point de test:** Code compile et fonctionne identique à avant (PCA9685 toujours actif)

---

## ✅ Étape 4 : Créer ServoDispatchMaestro.h minimal (TERMINÉ)

**🎯 Objectif:** Créer squelette de classe qui compile sans l'utiliser

### 4.1 Créer fichier et structure de base
**Fichier:** `.pio/libdeps/astropixelsplus/Reeltwo/src/ServoDispatchMaestro.h`

- [x] Créer fichier header guards ✅
- [x] Include nécessaires (ServoDispatch.h) ✅
- [x] Hériter de ServoDispatch, SetupEvent, AnimatedEvent ✅
- [x] Template `<uint16_t numServos>` ✅
- [x] Déclarer structure `ServoState` interne ✅

### 4.2 Constructor minimal
```cpp
ServoDispatchMaestro(HardwareSerial* serial, const ServoSettings* settings)
    : fSerial(serial)
```
- [x] Définir pointeur HardwareSerial ✅
- [x] Init basique sans logique complexe ✅
- [x] Charger settings depuis PROGMEM ✅

### 4.3 Implémenter méthodes virtuelles (stubs)
Toutes les méthodes pures retournent des valeurs par défaut:
- [x] `uint16_t getNumServos()` → return numServos ✅
- [x] `uint8_t getPin(uint16_t num)` → return settings pin ✅
- [x] `uint16_t getStart/End/Min/Max/Neutral()` → return settings values ✅
- [x] `uint32_t getGroup()` → return settings group ✅
- [x] `uint16_t currentPos()` → return stored pos ✅
- [x] `bool isActive()` → return false ✅
- [x] `void disable()` → update state ✅
- [x] `void setServo()` → update settings ✅
- [x] `void setPWM()` → stub (pas d'envoi encore) ✅
- [x] `void stop()` → update state ✅
- [x] Méthodes protected → stubs basiques ✅
- [x] `void setup()` → message debug ✅
- [x] `void animate()` → vide ✅

### 4.4 Test compilation avec Maestro
- [x] Modifier AstroPixelsPlus.ino ligne ~25: Activer `USE_SERVO_MAESTRO` ✅
- [x] Ajouter include conditionnel (ligne ~118-127) ✅
- [x] Ajouter instanciation conditionnelle (ligne ~333-340) ✅
- [x] Ajouter init Serial1 dans setup() (ligne ~648-654) ✅
- [x] **Compiler avec USE_SERVO_MAESTRO** activé ✅
- [x] Vérifier compilation réussie ✅
- [x] Compilation: RAM 19.0%, Flash 77.5% ✅

**Note:** ServoDispatchMaestro.h a toutes les méthodes en stubs. Les servos ne bougeront pas encore, mais le code compile.

✅ **Point de test:** Code compile avec Maestro (stubs), LED heartbeat OK, pas de régression

---

## 🔌 Étape 5 : Implémenter protocole Maestro Set Target (TESTABLE)

**🎯 Objectif:** Faire bouger UN servo avec commande simple

### 5.1 Implémenter setPWM() - Compact Protocol
```cpp
void setPWM(uint16_t num, uint16_t targetMicros) override
{
    if (num >= numServos || !fSerial) return;
    
    // Conversion µs → quart-µs (Maestro)
    uint16_t target = targetMicros * 4;
    
    // Compact Protocol: 4 bytes
    fSerial->write(0x84);                  // Set Target
    fSerial->write(num & 0x7F);            // Canal (0-23)
    fSerial->write(target & 0x7F);         // 7 bits bas
    fSerial->write((target >> 7) & 0x7F);  // 7 bits hauts
}
```
- [ ] Implémenter conversion µs → quart-µs
- [ ] Envoyer 4 bytes selon protocole Compact
- [ ] Ajouter validation (num < numServos)

### 5.2 Implémenter setServo() basique
```cpp
void setServo(uint16_t num, uint32_t pulseWidth, 
              uint16_t moveTimeMS, EasingMethod easing) override
{
    if (num >= numServos) return;
    
    // Pour test simple: ignorer easing/timing
    setPWM(num, pulseWidth);
    
    // Marquer servo actif
    fServos[num].fActive = true;
    fServos[num].fCurrentPos = pulseWidth;
}
```
- [ ] Appeler setPWM directement (pas d'easing pour test)
- [ ] Mettre à jour état interne

### 5.3 Test avec 1 servo physique
- [ ] Connecter Maestro TX Maestro → RX ESP32 (pas besoin de RX Maestro pour test)
  - ESP32 GPIO 19 (TX) → Maestro RX
  - GND commun
  - Alimenter Maestro séparément (5-6V)
- [ ] Connecter 1 servo au **canal 0** du Maestro
- [ ] Compiler et uploader firmware
- [ ] Via Serial Monitor, envoyer: `:SM0,1500` (position neutre)
- [ ] Vérifier servo bouge
- [ ] Envoyer: `:SM0,1000` (position min)
- [ ] Envoyer: `:SM0,2000` (position max)

✅ **Point de test:** 1 servo bouge avec commandes manuelles

---

## 🎨 Étape 6 : Implémenter easing et animate() (TESTABLE)

**🎯 Objectif:** Mouvements fluides avec interpolation

### 6.1 Structure ServoState complète
```cpp
struct ServoState {
    uint16_t fCurrentPos;     // Position actuelle µs
    uint16_t fTargetPos;      // Position cible µs
    uint32_t fMoveStartTime;  // millis() début mouvement
    uint16_t fMoveDuration;   // Durée mouvement ms
    EasingMethod fEasing;     // Méthode interpolation
    bool fActive;             // Servo activé?
    bool fMoving;             // En mouvement?
};
```
- [ ] Déclarer array `ServoState fServos[numServos]`
- [ ] Initialiser dans constructor

### 6.2 Implémenter setServo() avec easing
- [ ] Stocker target, startTime, duration, easing
- [ ] Marquer fMoving = true
- [ ] NE PAS appeler setPWM tout de suite

### 6.3 Implémenter animate()
```cpp
void animate() override
{
    uint32_t now = millis();
    
    for (uint16_t i = 0; i < numServos; i++)
    {
        if (!fServos[i].fMoving) continue;
        
        uint32_t elapsed = now - fServos[i].fMoveStartTime;
        
        if (elapsed >= fServos[i].fMoveDuration)
        {
            // Mouvement terminé
            fServos[i].fCurrentPos = fServos[i].fTargetPos;
            fServos[i].fMoving = false;
            setPWM(i, fServos[i].fCurrentPos);
        }
        else
        {
            // Interpoler position
            float progress = (float)elapsed / fServos[i].fMoveDuration;
            float eased = applyEasing(progress, fServos[i].fEasing);
            
            uint16_t newPos = fServos[i].fCurrentPos + 
                (fServos[i].fTargetPos - fServos[i].fCurrentPos) * eased;
            
            setPWM(i, newPos);
        }
    }
}
```
- [ ] Boucle sur tous servos actifs
- [ ] Calculer progression 0.0-1.0
- [ ] Appliquer easing
- [ ] Interpoler position
- [ ] Envoyer setPWM() à chaque frame

### 6.4 Test mouvements fluides
- [ ] Compiler et uploader
- [ ] Envoyer `:OP00` (ouvrir tous panneaux)
- [ ] Vérifier mouvements fluides (pas saccadés)
- [ ] Envoyer `:CL00` (fermer)
- [ ] Tester `:OF00` (flutter)

✅ **Point de test:** Mouvements fluides avec easing fonctionnel

---

## 🎭 Étape 7 : Implémenter méthodes protected pour groupes (TESTABLE)

**🎯 Objectif:** Commandes de groupe `:OP00`, `:CL00`, etc. fonctionnelles

### 7.1 Implémenter _moveServoToPulse()
- [ ] Appeler setServo() avec pulse length
- [ ] Gérer easing/timing

### 7.2 Implémenter _moveServoSetToPulse()
- [ ] Boucler sur tous servos du groupe (mask)
- [ ] Appeler _moveServoToPulse() pour chacun

### 7.3 Test commandes de groupe
- [ ] Connecter plusieurs servos (3-5 minimum)
- [ ] Tester `:OP00` (ouvrir tous panneaux)
- [ ] Tester `:CL00` (fermer tous)
- [ ] Tester `:OP01` (ouvrir groupe 1)
- [ ] Vérifier synchronisation

✅ **Point de test:** Commandes de groupe fonctionnelles

---

## 🌊 Étape 8 : Tests séquences animées (TESTABLE)

**🎯 Objectif:** Séquences complexes (Scream, Wave, etc.)

### 8.1 Connecter tous les servos panneaux
- [ ] 13 servos panneaux sur canaux 0-12
- [ ] Vérifier mapping avec servoSettings[]
- [ ] Alimenter correctement (5-6V, ampérage suffisant)

### 8.2 Test séquences
- [ ] `:SE01` (Scream) - open/close rapide
- [ ] `:SE02` (Wave) - séquence vague
- [ ] `:SE03` (Smirk Wave)
- [ ] `:SE04` (Short Circuit)
- [ ] `:SE05` (Cantina Wave)
- [ ] `:SE06` (Leia)

### 8.3 Valider timing
- [ ] Vérifier fluidité
- [ ] Pas de saccades
- [ ] Pas de timeouts série

✅ **Point de test:** Toutes séquences fonctionnelles

---

## 🎯 Étape 9 : Tests servos Holos (TESTABLE)

**🎯 Objectif:** Valider holos H/V sur canaux 13-18

### 9.1 Connecter servos holos
- [ ] Canaux 13-14: Front holo (H/V)
- [ ] Canaux 15-16: Top holo (H/V)
- [ ] Canaux 17-18: Rear holo (H/V)

### 9.2 Test mouvements holos
- [ ] `*RD00` (random movement)
- [ ] `$+` (speed up)
- [ ] `$-` (slow down)
- [ ] Vérifier `assignServos()` dans ServoEasing

### 9.3 Test commandes directes
- [ ] `~RTHPA000|0` (Holo to normal)
- [ ] Tester différents modes holos

✅ **Point de test:** Holos fonctionnels

---

## 🔍 Étape 10 : Debug & Optimisation (OPTIONNEL)

### Issues possibles
- [ ] Baudrate incorrect → Tester 115200 vs 9600
- [ ] Servos tremblent → Vérifier alimentation
- [ ] Positions inversées → Swap start/end dans servoSettings
- [ ] Timeout série → Mesurer latence
- [ ] Mémoire insuffisante → Optimiser ServoState

### Optimisations
- [ ] Mesurer latence setPWM()
- [ ] Optimiser animate() si nécessaire
- [ ] Réduire taille ServoState si RAM limitée

---

## 🔀 Étape 11 : Pass-through série externe - MaestroCommandRouter (TESTABLE)

**🎯 Objectif:** Recevoir commandes Maestro externes via GPIO 18, router vers Maestro

### 11.1 Créer MaestroCommandRouter.h (voir section détaillée plus bas)
- [ ] Structure Command avec queue FIFO
- [ ] Méthode sendInternal() pour ESP32
- [ ] Lecture Serial1.available() dans animate()
- [ ] Anti-collision 10ms entre commandes
- [ ] Validation format commandes

### 11.2 Modifier ServoDispatchMaestro
- [ ] Passer par router au lieu d'écriture directe

### 11.3 Tests pass-through
- [ ] Test commandes internes seules
- [ ] Test commandes externes seules (GPIO 18)
- [ ] Test collision/arbitrage

✅ **Point de test:** Pass-through externe fonctionnel

---

## 📚 Étape 12 : Documentation

- [ ] Commenter ServoDispatchMaestro.h (Doxygen)
- [ ] Mettre à jour README.md avec config Maestro
- [ ] Documenter câblage dans [Wiring-Diagram.png](Wiring-Diagram.png)
- [ ] Ajouter schéma Serial1 partagé
- [ ] Documenter MaestroCommandRouter
- [ ] Exemples commandes

---

## 🚀 Étape 13 : Finalisation

- [ ] Commit tous les changements
- [ ] Push branche feature/maestro-servo-controller
- [ ] Créer Pull Request vers main
- [ ] Tests finaux complets
- [ ] Merger dans main
- [ ] Tag version (v1.0.0-maestro)
- [ ] Archiver branche PCA9685 (backup)

---

## 🔀 ANNEXE : Détails Étape 11 - MaestroCommandRouter

**💡 Architecture:** Serial1 partagé (économise GPIO 4/5)
### 3.1 Structure de base
- [ ] Créer fichier `.pio/libdeps/astropixelsplus/Reeltwo/src/ServoDispatchMaestro.h`
- [ ] Hériter de ServoDispatch, SetupEvent, AnimatedEvent
- [ ] Template `<uint16_t numServos>`
- [ ] Déclarer structure `ServoState` interne

### 3.2 Constructor
```cpp
ServoDispatchMaestro(HardwareSerial* serial, const ServoSettings* settings)
```
- [ ] Initialiser pointeur serial
- [ ] Charger settings depuis PROGMEM
- [ ] Initialiser tableau de ServoState

### 3.3 Implémenter méthodes virtuelles pures
- [ ] `uint16_t getNumServos()`
- [ ] `uint8_t getPin(uint16_t num)`
- [ ] `uint16_t getStart(uint16_t num)`
- [ ] `uint16_t getEnd(uint16_t num)`
- [ ] `uint16_t getMinimum(uint16_t num)`
- [ ] `uint16_t getMaximum(uint16_t num)`
- [ ] `uint16_t getNeutral(uint16_t num)`
- [ ] `uint32_t getGroup(uint16_t num)`
- [ ] `uint16_t currentPos(uint16_t num)`
- [ ] `uint16_t scaleToPos(uint16_t num, float scale)`
- [ ] `bool isActive(uint16_t num)`
- [ ] `void disable(uint16_t num)`
- [ ] `void setServo(...)`
- [ ] `void setPWM(uint16_t num, uint16_t targetLength)` ⭐ CLEF
- [ ] `void stop()`

### 3.4 Implémenter méthodes protégées
- [ ] `void _moveServoToPulse(...)`
- [ ] `void _moveServosToPulse(...)`
- [ ] `void _moveServosByPulse(...)`
- [ ] `void _moveServoSetToPulse(...)`
- [ ] `void _moveServoSetByPulse(...)`
- [ ] `void _moveServosTo(...)`
- [ ] `void _moveServoSetTo(...)`
- [ ] `void _setServoEasingMethod(...)`
- [ ] `void _setServosEasingMethod(...)`
- [ ] `void _setEasingMethod(...)`

### 3.5 Implémenter méthodes événements
- [ ] `void setup()` - Initialiser Serial avec baudrate
- [ ] `void animate()` - Mettre à jour positions servos

---

## 🔌 Étape 4 : Protocole Série Maestro
### 4.1 Commande Set Target (Compact Protocol)
```cpp
void setPWM(uint16_t channel, uint16_t targetMicros) {
    uint16_t target = targetMicros * 4;  // µs → quart-µs
    fSerial->write(0x84);                 // Commande Set Target
    fSerial->write(channel);              // Canal (0-23)
    fSerial->write(target & 0x7F);        // 7 bits bas
    fSerial->write((target >> 7) & 0x7F); // 7 bits hauts
}
```
- [ ] Implémenter conversion µs → quart-µs
- [ ] Envoyer commande 4 octets
- [ ] Tester avec un servo

### 4.2 Autres commandes (optionnel)
- [ ] Get Position (0x90)
- [ ] Get Errors (0xA1)
- [ ] Go Home (0xA2)

---

## 🔧 Étape 5 : Modifier AstroPixelsPlus.ino
### 5.1 Configuration Serial1
- [ ] Commenter/désactiver code SOUND_SERIAL
- [ ] Définir `#define MAESTRO_SERIAL Serial1`
- [ ] Définir `#define MAESTRO_RX_PIN PIN_AUX4` (GPIO 18)
- [ ] Définir `#define MAESTRO_TX_PIN PIN_AUX5` (GPIO 19)
- [ ] Définir `#define MAESTRO_BAUD 115200` (ou 9600)

### 5.2 Directives de compilation
Ajouter dans les #define (ligne ~22):
```cpp
// #define USE_SERVO_DIRECT
// #define USE_SERVO_PCA9685
#define USE_SERVO_MAESTRO
```

### 5.3 Includes conditionnels
```cpp
#ifdef USE_SERVO_MAESTRO
    #include "ServoDispatchMaestro.h"
#elif defined(USE_SERVO_DIRECT)
    #include "ServoDispatchDirect.h"
#else
    #include "ServoDispatchPCA9685.h"
#endif
```

### 5.4 Instanciation ServoDispatch
Ligne ~314-318:
```cpp
#ifdef USE_SERVO_MAESTRO
    ServoDispatchMaestro<SizeOfArray(servoSettings)> servoDispatch(&MAESTRO_SERIAL, servoSettings);
#elif defined(USE_SERVO_DIRECT)
    ServoDispatchDirect<SizeOfArray(servoSettings)> servoDispatch(servoSettings);
#else
    ServoDispatchPCA9685<SizeOfArray(servoSettings)> servoDispatch(&Wire, servoSettings);
#endif
```

### 5.5 Setup
Ligne ~638-649, remplacer bloc son:
```cpp
#ifdef USE_SERVO_MAESTRO
    MAESTRO_SERIAL.begin(MAESTRO_BAUD, SERIAL_8N1, MAESTRO_RX_PIN, MAESTRO_TX_PIN);
    DEBUG_PRINTLN("Maestro servo controller initialized on Serial1");
#endif
```

---

## 🧪 Étape 6 : Tests unitaires
### 6.1 Test compilation
- [ ] Compiler avec `USE_SERVO_MAESTRO` défini
- [ ] Vérifier aucune erreur de compilation
- [ ] Vérifier warnings

### 6.2 Test LED heartbeat
- [ ] Uploader firmware
- [ ] Vérifier LED clignote à 1Hz
- [ ] Confirme que le code tourne

### 6.3 Test servo unique
- [ ] Connecter Maestro sur Serial1 (GPIO 18/19)
- [ ] Connecter 1 servo au canal 0
- [ ] Envoyer commande `:SM0,1500` (position neutre)
- [ ] Envoyer commande `:SM0,1000` (position min)
- [ ] Envoyer commande `:SM0,2000` (position max)

### 6.4 Test groupe de servos
- [ ] Connecter 3-5 servos
- [ ] Tester commande `:OP00` (ouvrir tous panneaux)
- [ ] Tester commande `:CL00` (fermer tous panneaux)
- [ ] Tester commande `:OF00` (flutter)

### 6.5 Test séquences
- [ ] Tester `:SE01` (Scream)
- [ ] Tester `:SE02` (Wave)
- [ ] Tester `:SE03` (Smirk Wave)

### 6.6 Test holos
- [ ] Connecter servos holos (canaux 13-18)
- [ ] Vérifier `assignServos()` fonctionne
- [ ] Tester mouvements holos

---

## � ANNEXE : Détails Étape 11 - MaestroCommandRouter

**💡 Architecture:** Serial1 partagé (économise GPIO 4/5)

### Câblage
```
Module externe TX → GPIO 18 (RX Serial1) → ESP32 → GPIO 19 (TX Serial1) → Maestro RX
                                                     (Maestro TX non connecté)
```
```
┌───────────────┐
│ Module externe│ Commandes Maestro
│               │ (0x84, 0x90, etc.)
└───────┬───────┘
        │ TX
        ▼ GPIO 18 (RX Serial1)
┌──────────────────────────────────┐
│ ESP32 - MaestroCommandRouter     │
│  ┌────────────────────────────┐  │
│  │  Serial1.available()       │  │
│  │    ↓                       │  │
│  │  Queue FIFO 32 cmd         │  │
│  │  Anti-collision            │  │
│  │  Validation format         │  │
│  │    ↓                       │  │
│  │  Serial1.write()           │  │
│  └────────────────────────────┘  │
│                                  │
│ Sources:                         │
│ • ServoDispatchMaestro (interne) │
│ • Module externe (GPIO 18)       │
└──────────┬───────────────────────┘
           │ GPIO 19 (TX Serial1)
           ▼
     ┌──────────┐
     │ Maestro  │ RX uniquement
     │ 24 chan  │ (TX non connecté)
     └──────────┘
```

### 11.7 Avantages de cette approche optimisée
- ✅ **Économie de 2 GPIO** (pas besoin de GPIO 4/5 pour Serial3)
- ✅ Console USB préservée pour debug
- ✅ Un seul port série à gérer (Serial1)
- ✅ Pas de collision hardware (RX/TX découplés physiquement)
- ✅ Maestro ne nécessite pas de retour pour Set Target
- ✅ Queue permet absorption pics commandes
- ✅ Validation évite commandes malformées
- ✅ Architecture simplifiée
- ✅ Si besoin futur de Get Position, possibilité de connecter Maestro TX → autre GPIO

### 11.8 Câblage physique simplifié
```
Module externe → ESP32 → Maestro
─────────────────────────────────
TX       →  GPIO 18 (RX)
GND      →  GND
            GPIO 19 (TX) → RX
            GND          → GND
            (TX Maestro non connecté)
```

---

## 📌 Notes importantes

### Câblage Maestro (Serial1 partagé)
```
Module externe → ESP32 → Maestro
TX           → GPIO 18 (RX Serial1)
GND          → GND
               GPIO 19 (TX Serial1) → RX
               GND                  → GND
(Alimentation servos séparée 5V/6V)
(TX Maestro non utilisé - laissé déconnecté)
```

### Baudrates supportés
- 9600 (fiable, longues distances)
- 115200 (rapide, recommandé)

### Configuration Maestro Control Center
- Mode: USB Dual Port
- Serial mode: UART, fixed baud rate
- CRC: Disabled (Compact Protocol)
- Servo ranges: Configurer min/max par canal

### Canaux Maestro (mapping servoSettings)

**⚠️ Valider les numéros de canaux avec [Wiring-Diagram.png](Wiring-Diagram.png)**

Le mapping ci-dessous correspond à `servoSettings[]` dans AstroPixelsPlus.ino (lignes 286-309).
Chaque canal Maestro doit être connecté au servo physique selon le schéma.

#### Panneaux du dôme (canaux 0-12)
| Canal | Pin PCA9685 | Type | Groupe | Nom/Position |
|-------|-------------|------|--------|--------------|
| 0 | 1 | SMALL_PANEL | PANEL_GROUP_4 | door 4 |
| 1 | 2 | SMALL_PANEL | PANEL_GROUP_3 | door 3 |
| 2 | 3 | SMALL_PANEL | PANEL_GROUP_2 | door 2 |
| 3 | 4 | SMALL_PANEL | PANEL_GROUP_1 | door 1 |
| 4 | 5 | MEDIUM_PANEL | PANEL_GROUP_5 | door 5 |
| 5 | 6 | BIG_PANEL | PANEL_GROUP_6 | door 9 |
| 6 | 7 | MINI_PANEL | - | mini door 2 |
| 7 | 8 | MINI_PANEL | - | mini front psi door |
| 8 | 9 | PIE_PANEL | PANEL_GROUP_10 | pie panel 1 |
| 9 | 10 | PIE_PANEL | PANEL_GROUP_9 | pie panel 2 |
| 10 | 11 | PIE_PANEL | PANEL_GROUP_8 | pie panel 3 |
| 11 | 12 | PIE_PANEL | PANEL_GROUP_7 | pie panel 4 |
| 12 | 13 | TOP_PIE_PANEL | - | dome top panel |

#### Servos Holos (canaux 13-18)
| Canal | Pin PCA9685 | Type | Nom/Position |
|-------|-------------|------|--------------|
| 13 | 16 | HOLO_HSERVO | horizontal front holo |
| 14 | 17 | HOLO_VSERVO | vertical front holo |
| 15 | 18 | HOLO_HSERVO | horizontal top holo |
| 16 | 19 | HOLO_VSERVO | vertical top holo |
| 17 | 20 | HOLO_VSERVO | vertical rear holo |
| 18 | 21 | HOLO_HSERVO | horizontal rear holo |

#### Canaux réservés (19-23)
Disponibles pour extension future (autres servos, HPL, etc.)

---

## 🎯 Checklist finale avant merge
- [ ] Code compile sans warnings
- [ ] Tous les tests passent
- [ ] Documentation à jour
- [ ] Pas de régression sur fonctionnalités existantes
- [ ] LED heartbeat fonctionne
- [ ] Toutes les commandes Marcduino testées
- [ ] Backup configuration fonctionnelle sur branche main

---

*Date création: 2026-01-09*
*Projet: AstroPixelsPlus - Migration Maestro*
*Développeur: stefe2*
