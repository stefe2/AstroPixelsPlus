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

## 🔨 Étape 2 : Ajouter LED heartbeat
- [ ] Ajouter `#define LED_BUILTIN 2` (ligne ~181)
- [ ] Initialiser LED dans setup() (ligne ~605)
- [ ] Faire clignoter LED dans loop() (ligne ~1152)
- [ ] Compiler et tester

---

## 📝 Étape 3 : Créer ServoDispatchMaestro.h
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

## 📊 Étape 7 : Validation & Optimisation
- [ ] Mesurer latence communication série
- [ ] Vérifier fluidité des mouvements
- [ ] Tester avec 19 servos simultanés
- [ ] Valider easing/interpolation
- [ ] Tester toutes les commandes Marcduino

---

## 🔍 Étape 8 : Debug & Troubleshooting
### Issues possibles
- [ ] Baudrate incorrect → Ajuster 115200 vs 9600
- [ ] Servos tremblent → Vérifier alimentation
- [ ] Positions inversées → Swap start/end pulse
- [ ] Timeout série → Ajouter délais
- [ ] Conflits GPIO → Vérifier pinout

---

## 📚 Étape 9 : Documentation
- [ ] Documenter ServoDispatchMaestro.h (Doxygen)
- [ ] Mettre à jour README.md
- [ ] Ajouter schéma de câblage Maestro
- [ ] Documenter configuration Serial1
- [ ] Ajouter exemples commandes

---

## 🚀 Étape 10 : Finalisation
- [ ] Commit tous les changements
- [ ] Push branche feature/maestro-servo-controller
- [ ] Créer Pull Request
- [ ] Merger dans main après validation
- [ ] Tag version (v1.0.0-maestro)

---

## 🔀 Étape 11 : Pass-through série externe (Serial1 partagé)

**💡 Optimisation:** Communication unidirectionnelle ESP32→Maestro, pas besoin de RX depuis Maestro.  
**Solution:** Utiliser GPIO 18 pour recevoir du module externe, GPIO 19 pour envoyer au Maestro.

### 11.1 Configuration GPIO (Serial1 partagé)
- [ ] ~~Pas besoin de GPIO 4/5~~ → **Économie de 2 GPIO!**
- [ ] Câblage:
  - Module externe TX → **GPIO 18 (RX Serial1)**
  - **GPIO 19 (TX Serial1)** → Maestro RX
  - Maestro TX → **non connecté** (pas de retour nécessaire)
- [ ] Mettre à jour [Wiring-Diagram.png](Wiring-Diagram.png) avec ce câblage simplifié

### 11.2 Créer MaestroCommandRouter.h
**Fichier:** `.pio/libdeps/astropixelsplus/Reeltwo/src/MaestroCommandRouter.h`

#### Structure de base
- [ ] Créer classe `MaestroCommandRouter`
- [ ] Ajouter pointeur `HardwareSerial* fSerial` (Serial1 partagé, plus besoin de 2 pointeurs)
- [ ] Définir structure `Command` (data[16], length, timestamp, source)
- [ ] Créer queue FIFO circulaire (32 commandes)
- [ ] Variables état: `fBusy`, `fLastCommandTime`
- [ ] Buffer réception externe: `uint8_t fRxBuffer[16]`, `fRxIndex`

#### Méthodes publiques
- [ ] `void setup(HardwareSerial* serial)` - Un seul port partagé
- [ ] `void animate()` - Traitement queue + lecture Serial1.available()
- [ ] `bool sendInternal(const uint8_t* data, uint8_t length)` - Depuis ESP32
- [ ] ~~processExternalSerial()~~ → Intégré dans animate()

#### Méthodes privées
- [ ] `bool enqueueCommand(const uint8_t* data, uint8_t length, Source source)`
- [ ] `void sendNextCommand()` - Dépile et envoie via Serial1.write()
- [ ] `void readExternalCommands()` - Lecture Serial1.available() dans animate()
- [ ] `bool validateCommand(const uint8_t* data, uint8_t length)` - Format Maestro
- [ ] `bool isQueueFull()`

#### Gestion anti-collision
- [ ] Délai minimum 10ms entre commandes (`MIN_COMMAND_INTERVAL`)
- [ ] Timeout 100ms pour commandes incomplètes depuis module externe
- [ ] Validation format avant envoi:
  - `0x84` Set Target → 4 bytes
  - `0x90` Get Position → 2 bytes (si nécessaire pour debug)
  - `0xA2` Go Home → 2 bytes
  - Autres commandes selon protocole Maestro

### 11.3 Modifier ServoDispatchMaestro.h
- [ ] Ajouter pointeur `MaestroCommandRouter* fRouter`
- [ ] Modifier constructeur pour accepter router
- [ ] Remplacer tous `fSerial->write()` par `fRouter->sendInternal()`
- [ ] Dans `setPWM()`: router au lieu d'écriture directe

### 11.4 Modifier AstroPixelsPlus.ino

#### Déclarations globales (ligne ~320)
```cpp
#ifdef USE_SERVO_MAESTRO
    MaestroCommandRouter maestroRouter;
#endif
```

#### Dans setup() (ligne ~650)
```cpp
#ifdef USE_SERVO_MAESTRO
    // Serial1 déjà initialisé pour Maestro
    maestroRouter.setup(&MAESTRO_SERIAL);  // Un seul port partagé
    DEBUG_PRINTLN("Maestro command router initialized (shared Serial1)");
    DEBUG_PRINTLN("Module externe → GPIO 18 (RX), GPIO 19 (TX) → Maestro");
#endif
```

#### Dans loop() (ligne ~1155)
```cpp
#ifdef USE_SERVO_MAESTRO
    maestroRouter.animate();  // Gère réception externe + envoi Maestro
#endif
```

### 11.5 Tests & Validation
- [ ] Compiler et vérifier taille mémoire
- [ ] Test 1: Commandes internes seules (servo moves)
- [ ] Test 2: Commandes externes seules (via GPIO 18)
- [ ] Test 3: Commandes simultanées (détection collision)
- [ ] Test 4: Surcharge queue (32+ commandes)
- [ ] Test 5: Commandes invalides (validation)
- [ ] Test 6: Mesurer latence end-to-end
- [ ] Test 7: Vérifier timing 10ms entre commandes
- [ ] Test 8: Vérifier pas de corruption avec RX/TX partagé

### 11.6 Architecture (Serial1 partagé)
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
