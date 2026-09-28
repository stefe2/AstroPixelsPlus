#include <Arduino.h>
#include "driver/gpio.h"

/**
 *
 * AstroPixelsPlus : logics, PSI et holos du dôme ; panneaux et holos pilotés par un Pololu Maestro
 * (Serial1). Commandes Marcduino sur le port USB et sur Serial2.
 *
 */

// Choix du contrôleur servo (un seul actif à la fois)
#define USE_SERVO_MAESTRO     // Pololu Maestro sur Serial1
#define USE_DEBUG // Define to enable debug diagnostic

///////////////////////////////////

#if __has_include("build_version.h")
#include "build_version.h"
#endif

#if __has_include("reeltwo_build_version.h")
#include "reeltwo_build_version.h"
#endif

////////////////////////////////

#define CONSOLE_BUFFER_SIZE 300

////////////////////////////////

#include "ReelTwo.h"
#include "dome/Logics.h"
#include "dome/LogicEngineController.h"
#include "dome/HoloLights.h"

// Vrai quand l'échéance est atteinte. Reste correct quand millis() repasse à 0 (après 49,7 jours),
// contrairement à millis() >= deadline.
static inline bool timeReached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

// Les servos des holos sont relâchés par ServoDispatchMaestro 700 ms après chaque mouvement,
// comme les autres servos. L'ancienne classe HoloLightsWithAutoStop renvoyait disable() toutes les
// secondes quand les holos étaient au repos, ce qui mettait le Maestro en erreur (revue, problème 3).

#include "dome/NeoPSI.h"
#include "dome/FireStrip.h"
#include "dome/BadMotivator.h"
#include "dome/TeecesPSI.h"
#include "dome/TeecesLogics.h"
#include "body/DataPanel.h"
#include "body/ChargeBayIndicator.h"

#ifdef USE_MAESTRO_ADDRESS
#include "i2c/I2CReceiver.h"
#include "ServoDispatchDirect.h"
#elif defined(USE_SERVO_MAESTRO)
#include "ServoDispatchMaestro.h"
#elif defined(USE_SERVO_DIRECT)
#include "ServoDispatchDirect.h"
#else
#include "ServoDispatchPCA9685.h"
#endif
#include "ServoSequencer.h"
#include "servo-sequences-custom.h"
#include "core/Marcduino.h"

////////////////////////////////

#define SERIAL2_RX_PIN 16
#define SERIAL2_TX_PIN 17
#define COMMAND_SERIAL Serial2

////////////////////////////////
// Configuration Pololu Maestro (Serial1)
// Serial1 utilisé pour contrôle servos Maestro (anciennement son)
#define MAESTRO_SERIAL Serial1
#define MAESTRO_RX_PIN PIN_AUX4  // GPIO 18 (anciennement SOUND_RX_PIN)
#define MAESTRO_TX_PIN PIN_AUX5  // GPIO 19 (anciennement SOUND_TX_PIN)
#define MAESTRO_BAUD 115200      // 115200 ou 9600 selon config Maestro

////////////////////////////////

#define MARC_SERIAL2_BAUD_RATE 9600

////////////////////////////////

#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_FRONT_LOGIC 15
#define PIN_REAR_LOGIC 33
#define PIN_FRONT_PSI 32
#define PIN_REAR_PSI 23
#define PIN_FRONT_HOLO 25
#define PIN_REAR_HOLO 26
#define LED_BUILTIN 2  // LED onboard ESP32 pour heartbeat
#define PIN_TOP_HOLO 27
#define PIN_AUX1 2
#define PIN_AUX2 4
#define PIN_AUX3 5
#define PIN_AUX4 18
#define PIN_AUX5 19

#ifdef USE_RSERIES_RLD_CURVED
// Define RSeries RLD clock pin to be AUX5 (could just as well be AUX1, AUX2, AUX3, or AUX4)
#define PIN_REAR_LOGIC_CLOCK PIN_AUX5
#endif

#if defined(USE_RSERIES_RLD_CURVED)
LogicEngineCurvedRLD<PIN_REAR_LOGIC, PIN_REAR_LOGIC_CLOCK> RLD(LogicEngineRLDDefault, 3);
#elif defined(USE_RSERIES_RLD)
LogicEngineDeathStarRLD<PIN_REAR_LOGIC> RLD(LogicEngineRLDDefault, 3);
#else
AstroPixelRLD<PIN_REAR_LOGIC> RLD(LogicEngineRLDDefault, 3);
#endif

#ifdef USE_RSERIES_FLD
LogicEngineDeathStarFLD<PIN_FRONT_LOGIC> FLD(LogicEngineFLDDefault, 1);
#else
AstroPixelFLD<PIN_FRONT_LOGIC> FLD(LogicEngineFLDDefault, 1);
#endif

AstroPixelFrontPSI<PIN_FRONT_PSI> frontPSI(LogicEngineFrontPSIDefault, 4);
AstroPixelRearPSI<PIN_REAR_PSI> rearPSI(LogicEngineRearPSIDefault, 5);

#if USE_HOLO_TEMPLATE
HoloLights<PIN_FRONT_HOLO, NEO_GRB> frontHolo(1);
HoloLights<PIN_REAR_HOLO, NEO_GRB> rearHolo(2);
HoloLights<PIN_TOP_HOLO, NEO_GRB> topHolo(3);
#else
HoloLights frontHolo(PIN_FRONT_HOLO, HoloLights::kRGB, 1);
HoloLights rearHolo(PIN_REAR_HOLO, HoloLights::kRGB, 2);
HoloLights topHolo(PIN_TOP_HOLO, HoloLights::kRGB, 3);
#endif

// Animateur de mouvement aléatoire continu des 3 holos
class HoloAliveAnimator : public AnimatedEvent
{
public:
    void enable(uint32_t minDelayMs, uint32_t maxDelayMs)
    {
        fMinDelay = minDelayMs;
        fMaxDelay = maxDelayMs;
        fEnabled = true;
        uint32_t now = millis();
        // Stagger initial aléatoire pour chaque holo (organic, non synchronisé)
        fNextMoveTime[0] = now + random(500, fMinDelay);
        fNextMoveTime[1] = now + random(500, fMinDelay) + random(0, fMinDelay / 2);
        fNextMoveTime[2] = now + random(500, fMinDelay) + random(0, fMinDelay);
    }

    void disable()
    {
        fEnabled = false;
        // Retour au centre (position 1) avec mouvement doux
        frontHolo.moveHP(1, 500);
        rearHolo.moveHP(1, 500);
        topHolo.moveHP(1, 500);
    }

    // Pause sans recentrer les holos (chorégraphie musicale), puis reprise aux mêmes délais
    void suspend() { fEnabled = false; }
    void resume() { enable(fMinDelay, fMaxDelay); }
    bool isEnabled() const { return fEnabled; }

    virtual void animate() override
    {
        if (!fEnabled)
            return;
        uint32_t now = millis();
        HoloLights* holos[3] = { &frontHolo, &rearHolo, &topHolo };
        for (int i = 0; i < 3; i++)
        {
            if (timeReached(now, fNextMoveTime[i]))
            {
                holos[i]->moveHP(random(0, 9), random(300, 700));
                fNextMoveTime[i] = now + random(fMinDelay, fMaxDelay);
            }
        }
    }

private:
    bool     fEnabled       = false;
    uint32_t fNextMoveTime[3] = {0, 0, 0};
    uint32_t fMinDelay      = 3000;
    uint32_t fMaxDelay      = 8000;
};
HoloAliveAnimator holoAlive;

// Animateur LED indépendant par holo : fondu bleu↔blanc aléatoire avec machine à états
// Bypass complet de effectDimPulse (bugué : utilise 255/brightness au lieu de brightness)
class HoloLEDAnimator : public AnimatedEvent
{
    static const uint8_t  kMaxBri = 40;   // luminosité max ≈ 16% (40/255)
    static const uint32_t kFadeMs = 1500; // 1.5s fondu entrée/sortie

    enum State : byte { kIdle, kFadeIn, kHold, kFadeOut };

public:
    void enable()
    {
        fEnabled = true;
        uint32_t now = millis();
        static const char* initCmds[3] = { "HPF0000", "HPR0000", "HPT0000" };
        for (int i = 0; i < 3; i++)
        {
            CommandEvent::process(initCmds[i]); // reset fLEDFunction=0 sur chaque holo
            fState[i] = kIdle;
            fTimer[i] = now + random(500, 5000); // stagger aléatoire
        }
    }

    void disable()
    {
        fEnabled = false;
        HoloLights* holos[3] = { &frontHolo, &rearHolo, &topHolo };
        for (int i = 0; i < 3; i++)
        {
            fState[i] = kIdle;
            setColor(holos[i], 0, fWhite[i]);
        }
    }

    bool isEnabled() const { return fEnabled; }

    virtual void animate() override
    {
        if (!fEnabled)
            return;
        uint32_t now = millis();
        HoloLights* holos[3] = { &frontHolo, &rearHolo, &topHolo };
        for (int i = 0; i < 3; i++)
        {
            uint32_t elapsed = now - fTimer[i];
            switch (fState[i])
            {
                case kIdle:
                    if (timeReached(now, fTimer[i]))
                    {
                        fWhite[i] = (uint8_t)random(0, 101); // 0=bleu pur, 100=blanc pur
                        fState[i] = kFadeIn;
                        fTimer[i] = now;
                    }
                    break;

                case kFadeIn:
                {
                    uint8_t bri = (elapsed >= kFadeMs) ?
                        kMaxBri :
                        (uint8_t)((uint32_t)kMaxBri * elapsed / kFadeMs);
                    setColor(holos[i], bri, fWhite[i]);
                    if (elapsed >= kFadeMs)
                    {
                        fHoldMs[i] = random(5000, 10000);
                        fState[i]  = kHold;
                        fTimer[i]  = now;
                    }
                    break;
                }

                case kHold:
                    setColor(holos[i], kMaxBri, fWhite[i]);
                    if (elapsed >= fHoldMs[i])
                    {
                        fState[i] = kFadeOut;
                        fTimer[i] = now;
                    }
                    break;

                case kFadeOut:
                {
                    uint8_t bri = (elapsed >= kFadeMs) ?
                        0 :
                        (uint8_t)((uint32_t)kMaxBri * (kFadeMs - elapsed) / kFadeMs);
                    setColor(holos[i], bri, fWhite[i]);
                    if (elapsed >= kFadeMs)
                    {
                        fState[i] = kIdle;
                        fTimer[i] = now + random(3000, 9000);
                    }
                    break;
                }
            }
        }
    }

private:
    bool     fEnabled     = false;
    State    fState[3]    = { kIdle, kIdle, kIdle };
    uint32_t fTimer[3]    = { 0, 0, 0 };
    uint32_t fHoldMs[3]   = { 0, 0, 0 };
    uint8_t  fWhite[3]    = { 0, 0, 0 };  // 0=bleu pur … 100=blanc pur

    // w=0 → (0,0,bri)  w=100 → (bri,bri,bri)
    void setColor(HoloLights* holo, uint8_t bri, uint8_t w)
    {
        uint8_t rg = (uint8_t)((uint32_t)bri * w / 100);
        uint16_t n = holo->numPixels();
        for (uint16_t j = 0; j < n; j++)
            holo->setPixelColor(j, rg, rg, bri);
        holo->dirty();
    }
};
HoloLEDAnimator holoLED;

////////////////////////////////

#define SMALL_PANEL 0x0001
#define MEDIUM_PANEL 0x0002
#define BIG_PANEL 0x0004
#define PIE_PANEL 0x0008
#define TOP_PIE_PANEL 0x0010
#define MINI_PANEL 0x0020

#define HOLO_HSERVO 0x1000
#define HOLO_VSERVO 0x2000

#define DOME_PANELS_MASK (SMALL_PANEL | MEDIUM_PANEL | BIG_PANEL)
#define PIE_PANELS_MASK (PIE_PANEL)
#define ALL_DOME_PANELS_MASK (MINI_PANEL | DOME_PANELS_MASK | PIE_PANELS_MASK | TOP_PIE_PANEL)
#define DOME_DANCE_PANELS_MASK (DOME_PANELS_MASK | PIE_PANELS_MASK)
#define HOLO_SERVOS_MASK (HOLO_HSERVO | HOLO_VSERVO)

#define PANEL_GROUP_1 (1L << 14)
#define PANEL_GROUP_2 (1L << 15)
#define PANEL_GROUP_3 (1L << 16)
#define PANEL_GROUP_4 (1L << 17)
#define PANEL_GROUP_5 (1L << 18)
#define PANEL_GROUP_6 (1L << 19)
#define PANEL_GROUP_7 (1L << 20)
#define PANEL_GROUP_8 (1L << 21)
#define PANEL_GROUP_9 (1L << 22)
#define PANEL_GROUP_10 (1L << 23)
#define PANEL_GROUP_11 (1L << 24)
#define PANEL_GROUP_12 (1L << 25)
#define PANEL_GROUP_13 (1L << 26)
#define PANEL_GROUP_14 (1UL << 27)
#define PANEL_GROUP_15 (1UL << 28)
#define PANEL_GROUP_16 (1UL << 29)
#define PANEL_GROUP_17 (1UL << 30)
#define PANEL_GROUP_18 (1UL << 31)

#define EMPTY_AUX 0x4000

////////////////////////////////
// Positions fermé/ouvert de chaque canal du Maestro (µs) et groupes de panneaux
const ServoSettings servoSettings[] PROGMEM = {
#ifndef USE_MAESTRO_ADDRESS
    // First PCA9685 controller
    {0, 1840, 992,   PANEL_GROUP_1  | SMALL_PANEL},     // Maestro PIN 0: door 4
    {1, 1888, 992,   PANEL_GROUP_2  | SMALL_PANEL},     // Maestro PIN 1: door 3
    {2, 1872, 992,   PANEL_GROUP_3  | SMALL_PANEL},     // Maestro PIN 2: door 2
    {3, 1872, 992,   PANEL_GROUP_4  | MEDIUM_PANEL},    // Maestro PIN 3: door 1
    {4, 1872, 992,   PANEL_GROUP_5  | MEDIUM_PANEL},    // Maestro PIN 4: door 5
    {5, 2000, 992,   PANEL_GROUP_6  | BIG_PANEL},       // Maestro PIN 5: door 9
    {6, 2000, 992,   PANEL_GROUP_7  | PIE_PANEL},       // Maestro PIN 6: pie panel 1
    {7, 2000, 992,   PANEL_GROUP_8  | PIE_PANEL},       // Maestro PIN 7: pie panel 2
    {8, 2000, 992,   PANEL_GROUP_9  | PIE_PANEL},       // Maestro PIN 8: pie panel 3
    {9, 1920, 992,   PANEL_GROUP_10 | PIE_PANEL},       // Maestro PIN 9: pie panel 4
    {10, 1872, 992,  PANEL_GROUP_11 | MINI_PANEL},      // Maestro PIN 10: mini door 2
    {11, 1552, 992,  PANEL_GROUP_12 | MINI_PANEL},      // Maestro PIN 11: mini front psi door (1552 = limite du Maestro, vérifié)
    {12, 2000, 992,  PANEL_GROUP_13 | TOP_PIE_PANEL},   // Maestro PIN 12: dome top panel
    {13, 1248, 1744, HOLO_HSERVO},                      // Maestro PIN 13: horizontal front holo
    {14, 1248, 1744, HOLO_VSERVO},                      // Maestro PIN 14: vertical front holo
    {15, 1248, 1744, HOLO_HSERVO},                      // Maestro PIN 15: horizontal top holo
    {16, 1248, 1744, HOLO_VSERVO},                      // Maestro PIN 16: vertical top holo
    {17, 1248, 1744, HOLO_HSERVO},                      // Maestro PIN 17: horizontal rear holo (RHP-H dans le Maestro)
    {18, 1248, 1744, HOLO_VSERVO},                      // Maestro PIN 18: vertical rear holo (RHP-V dans le Maestro)
    {19, 2000, 992,  PANEL_GROUP_14 | EMPTY_AUX},       // Maestro PIN 19: Empty
    {20, 2000, 992,  PANEL_GROUP_15 | EMPTY_AUX},       // Maestro PIN 20: Empty
    {21, 2000, 992,  PANEL_GROUP_16 | EMPTY_AUX},       // Maestro PIN 21: Empty
    {22, 2000, 992,  PANEL_GROUP_17 | EMPTY_AUX},       // Maestro PIN 22: Empty
    {23, 2000, 992,  PANEL_GROUP_18 | EMPTY_AUX},       // Maestro PIN 23: Empty
#endif
};

#ifdef USE_MAESTRO_ADDRESS
ServoDispatchDirect<SizeOfArray(servoSettings)> servoDispatch(servoSettings);
#elif defined(USE_SERVO_MAESTRO)
ServoDispatchMaestro<SizeOfArray(servoSettings)> servoDispatch(&MAESTRO_SERIAL, servoSettings);
#elif defined(USE_SERVO_DIRECT)
ServoDispatchDirect<SizeOfArray(servoSettings)> servoDispatch(servoSettings);
#else
ServoDispatchPCA9685<SizeOfArray(servoSettings)> servoDispatch(&Wire, servoSettings);
#endif

// Custom ServoSequencer that automatically disables servos when sequence finishes
class ServoSequencerWithAutoStop : public ServoSequencer
{
private:
    unsigned long fStopDelayMS = 0;
    // BUG FIX: isFinished() is already false when animate() first runs after play(),
    // because play() sets fSequence != null before animate() is called.
    // Solution: track the previous frame's finished state so wasFinished=true
    // on the first animate() call after play(), triggering setSequenceActive(true).
    bool fLastIsFinished = true;
    
public:
    ServoSequencerWithAutoStop(ServoDispatch& dispatch) : ServoSequencer(dispatch) {}

    // Arrête la séquence en cours sans fermeture automatique ni relâchement global différé :
    // une chorégraphie musicale prend la main sur les panneaux.
    void abortQuietly()
    {
        stop();
        fLastIsFinished = true;
        fStopDelayMS = 0;
        dispatch().setSequenceActive(false);
    }

    virtual void animate() override
    {
        bool wasFinished = fLastIsFinished;   // state from previous frame
        ServoSequencer::animate();
        bool nowFinished = isFinished();
        fLastIsFinished = nowFinished;        // update for next frame
        
        // Notify dispatch when sequence starts/stops
        if (wasFinished && !nowFinished)
        {
            // Sequence just started — prevent per-servo auto-stop during sequence
            dispatch().setSequenceActive(true);
        }
        else if (!wasFinished && nowFinished)
        {
            // Sequence just finished — close-all safety net, then schedule global stop.
            // Pas de fermeture si la séquence est allée au bout en laissant des panneaux ouverts
            // (:OP00, :OP01–:OP20, :OP$…, $720) : ils restent ouverts jusqu'à :CL00. Avant, ils se
            // refermaient ~250 ms après l'ouverture (revue, problème 4). Une séquence interrompue
            // par une autre commande est toujours refermée.
            bool leftPanelsOpen = completed() && (lastServoSetMask() & ALL_DOME_PANELS_MASK) != 0;
            if (!leftPanelsOpen)
                dispatch().moveServosTo(ALL_DOME_PANELS_MASK, 125, 0.0);
            dispatch().setSequenceActive(false);
            // 1500ms: 125ms movement + 1375ms margin before stop()
            fStopDelayMS = millis() + 1500;
        }
        
        // Check if it's time to stop all servos after sequence end
        if (fStopDelayMS != 0 && timeReached(millis(), fStopDelayMS))
        {
            dispatch().stop();
            fStopDelayMS = 0;
        }
    }
};

ServoSequencerWithAutoStop servoSequencer(servoDispatch);
AnimationPlayer player(servoSequencer);

/////////////////////////////////////////////////////////////////////////

// Variables pour effets personnalisés (utilisées dans effects/)
#define NUM_LEDS 28 * 4
CRGB leds[NUM_LEDS];

enum
{
    SDBITMAP = 100,
    PLASMA,
    METABALLS,
    FRACTAL,
    FADEANDSCROLL
};

#include "effects/BitmapEffect.h"
#include "effects/FadeAndScrollEffect.h"
#include "effects/FractalEffect.h"
#include "effects/MeatBallsEffect.h"
#include "effects/PlasmaEffect.h"

////////////////////////////////
// Standard LogicEngine sequences are in the range 0-99. Custom sequences start at 100
LogicEffect CustomLogicEffectSelector(unsigned selectSequence)
{
    static const LogicEffect sCustomLogicEffects[] = {
        LogicEffectBitmap,
        LogicEffectPlasma,
        LogicEffectMetaBalls,
        LogicEffectFractal,
        LogicEffectFadeAndScroll};
    if (selectSequence >= 100 && selectSequence - 100 < SizeOfArray(sCustomLogicEffects))
    {
        return LogicEffect(sCustomLogicEffects[selectSequence - 100]);
    }
    return LogicEffectDefaultSelector(selectSequence);
}

////////////////////////////////

// Redémarrage de l'ESP32 (#APRESTART)
void reboot()
{
    DEBUG_PRINTLN("Restarting...");
    delay(1000);
    ESP.restart();
}

////////////////////////////////
// This function is called when aborting or ending Marcduino sequences. It should reset all droid devices to Normal
void resetSequence()
{
    Marcduino::send(F("$s"));
    CommandEvent::process(F(
        // Sans "|0" : une commande LE de 9 caractères ou plus vise l'appareil dont l'ID est le
        // premier chiffre (ici 0, qui n'existe pas) et aucune logic n'était remise à zéro
        "LE000000\n"   // LogicEngine devices to normal
        "FSOFF\n"      // Fire Stripe Off
        "BMOFF\n"      // Bad Motiviator Off
        "HPA000|0\n"   // Holo Projectors to Normal
        "CB00000\n"    // Charge Bay to Normal
        "DP00000\n")); // Data Panel to Normal
    // LE000000 met aussi les PSI sur l'effet NORMAL (scintillement) : les remettre sur leur
    // effet de démarrage (color wipe)
    frontPSI.selectEffect(LogicEngineFrontPSIDefault.fDefaultEffect);
    rearPSI.selectEffect(LogicEngineRearPSIDefault.fDefaultEffect);
}

////////////////////////////////

int32_t strtol(const char *cmd, const char **endptr)
{
    bool sign = false;
    int32_t result = 0;
    if (*cmd == '-')
    {
        cmd++;
        sign = true;
    }
    while (isdigit(*cmd))
    {
        result = result * 10L + (*cmd - '0');
        cmd++;
    }
    *endptr = cmd;
    return (sign) ? -result : result;
}

////////////////////////////////

bool numberparams(const char *cmd, uint8_t &argcount, int32_t *args, uint8_t maxcount)
{
    for (argcount = 0; argcount < maxcount; argcount++)
    {
        args[argcount] = strtol(cmd, &cmd);
        if (*cmd == '\0')
        {
            argcount++;
            return true;
        }
        else if (*cmd != ',')
        {
            return false;
        }
        cmd++;
    }
    return true;
}

////////////////////////////////

#include "MarcduinoHolo.h"
#include "MarcduinoLogics.h"
#include "MarcduinoSequence.h"
#include "MarcduinoPanel.h"
#include "MarcduinoPSI.h"
#include "MarcduinoMusic.h"

////////////////////////////////
// Diagnostic (#APSTAT)

// Durée des tours de loop(), remise à zéro à chaque #APSTAT
static uint32_t sLoopMaxUs = 0;
static uint64_t sLoopTotalUs = 0;
static uint32_t sLoopCount = 0;
static bool sSkipLoopSample = false;  // le tour qui affiche #APSTAT n'est pas mesuré

static const char *resetReasonName(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXTERNAL";
    case ESP_RST_SW:        return "SOFTWARE";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INTERRUPT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
    }
}

static void printStatus()
{
    Serial.println("---- APSTAT ----");
    Serial.printf("Uptime:         %lu s\n", (unsigned long)(millis() / 1000));
    Serial.printf("Reset reason:   %s\n", resetReasonName(esp_reset_reason()));
    Serial.printf("Heap free:      %u bytes\n", (unsigned)ESP.getFreeHeap());
    Serial.printf("Heap min free:  %u bytes\n", (unsigned)ESP.getMinFreeHeap());
    Serial.printf("Heap max block: %u bytes\n", (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Serial.printf("Loop stack min: %u bytes free\n", (unsigned)uxTaskGetStackHighWaterMark(NULL));
    if (sLoopCount > 0)
    {
        Serial.printf("Loop time:      avg %lu us, max %lu us (%lu loops)\n",
                      (unsigned long)(sLoopTotalUs / sLoopCount), (unsigned long)sLoopMaxUs,
                      (unsigned long)sLoopCount);
    }
    sLoopMaxUs = 0;
    sLoopTotalUs = 0;
    sLoopCount = 0;
    sSkipLoopSample = true;
}

////////////////////////////////

void setup()
{
    REELTWO_READY();

    PrintReelTwoInfo(Serial, "AstroPixelsPlus");

    COMMAND_SERIAL.begin(MARC_SERIAL2_BAUD_RATE, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
    // Rappel au niveau haut (repos UART) : sans contrôleur branché, l'entrée RX flotterait et
    // des parasites seraient lus comme des caractères. gpio_pullup_en ne touche pas au routage UART.
    gpio_pullup_en((gpio_num_t)SERIAL2_RX_PIN);

    // LED heartbeat setup
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    DEBUG_PRINTLN("LED heartbeat initialized on GPIO 2");

    // Initialize Maestro Serial1 (if using Maestro)
#ifdef USE_SERVO_MAESTRO
    MAESTRO_SERIAL.begin(MAESTRO_BAUD, SERIAL_8N1, MAESTRO_RX_PIN, MAESTRO_TX_PIN);
    DEBUG_PRINT("Maestro servo controller initialized on Serial1 (");
    DEBUG_PRINT(MAESTRO_BAUD);
    DEBUG_PRINTLN(" baud)");
#endif

#if !defined(USE_MAESTRO_ADDRESS) && !defined(USE_SERVO_MAESTRO) && !defined(USE_SERVO_DIRECT)
    Wire.begin();  // Only needed for PCA9685 servo mode
#endif
    SetupEvent::ready();

    RLD.selectScrollTextLeft("... R2-BLING ...", LogicEngineRenderer::kWhite, 0, 15);
    FLD.selectScrollTextLeft("... R2-BLING ...", LogicEngineRenderer::kWhite, 0, 15);

    // Servos des holos (canal horizontal, canal vertical), voir servoSettings[]
    frontHolo.assignServos(&servoDispatch, 13, 14);
    topHolo.assignServos(&servoDispatch, 15, 16);
    rearHolo.assignServos(&servoDispatch, 17, 18);

    // Fermer les panneaux du dôme au démarrage (le Maestro reste sur « Off » au démarrage) ;
    // le pilote les relâche 0,7 s après. Mouvement instantané : le pilote ne connaît pas encore
    // la position réelle (il suppose la mi-course), une interpolation partirait de là.
    servoDispatch.moveServosTo(ALL_DOME_PANELS_MASK, 0.0);

    RLD.setLogicEffectSelector(CustomLogicEffectSelector);
    FLD.setLogicEffectSelector(CustomLogicEffectSelector);
    frontPSI.setLogicEffectSelector(CustomLogicEffectSelector);
    rearPSI.setLogicEffectSelector(CustomLogicEffectSelector);

    // Watchdog de la boucle principale : redémarre l'ESP32 si loop() bloque plus de 5 s
    // (CONFIG_ESP_TASK_WDT_TIMEOUT_S). Activé après setup() pour ne pas surveiller le démarrage.
    enableLoopWDT();

    Serial.print("Reset reason: ");
    Serial.println(resetReasonName(esp_reset_reason()));
    DEBUG_PRINTLN("Ready");
}

////////////////

MARCDUINO_ACTION(DirectCommand, ~RT, ({
                     // Direct ReelTwo command
                     CommandEvent::process(Marcduino::getCommand());
                 }))

////////////////

MARCDUINO_ACTION(MDDirectCommand, @AP, ({
                     // Direct ReelTwo command
                     CommandEvent::process(Marcduino::getCommand());
                 }))

////////////////

MARCDUINO_ACTION(Restart, #APRESTART, ({
                     reboot();
                 }))

////////////////
// Diagnostic : état du système et durée des tours de boucle depuis le dernier #APSTAT

MARCDUINO_ACTION(Status, #APSTAT, ({
                     printStatus();
                 }))

////////////////

static unsigned sPos;
static char sBuffer[CONSOLE_BUFFER_SIZE];
static unsigned sPos2;
static char sBuffer2[CONSOLE_BUFFER_SIZE];

// Nombre maximum de caractères lus par port et par tour de boucle
#define MAX_SERIAL_CHARS_PER_LOOP 64

// Lit les caractères disponibles et exécute la commande à la fin de ligne (CR ou LF).
// Une ligne vide est ignorée : avec CR+LF, le LF ne relance pas la commande précédente.
// La lecture s'arrête après une commande : processCommand() programme l'animation pour le
// tour suivant, une deuxième commande lue dans le même tour l'écraserait.
static void readCommandSerial(Stream &port, char *buffer, unsigned &pos, const char *logPrefix)
{
    for (int i = 0; i < MAX_SERIAL_CHARS_PER_LOOP && port.available(); i++)
    {
        int ch = port.read();
        if (ch == 0x0A || ch == 0x0D)
        {
            if (pos == 0)
                continue;
            buffer[pos] = '\0';
            pos = 0;
            if (logPrefix != nullptr)
            {
                Serial.print(logPrefix);
                Serial.println(buffer);
            }
            Marcduino::processCommand(player, buffer);
            return;
        }
        else if (pos < CONSOLE_BUFFER_SIZE - 1)
        {
            buffer[pos++] = ch;
            buffer[pos] = '\0';
        }
    }
}

////////////////
// LED heartbeat - clignotement 1Hz
static uint32_t sLastHeartbeat = 0;
static bool sHeartbeatState = false;

////////////////

#ifdef USE_MAESTRO_ADDRESS
I2CReceiverBase<CONSOLE_BUFFER_SIZE> i2cReceiver(USE_MAESTRO_ADDRESS, [](char *cmd)
                                                 {
    DEBUG_PRINT("[I2C] RECEIVED=\"");
    DEBUG_PRINT(cmd);
    DEBUG_PRINTLN("\"");
    Marcduino::processCommand(player, cmd); });
#endif

////////////////

void mainLoop()
{
    // LED heartbeat - clignoter toutes les 500ms (1Hz)
    uint32_t now = millis();
    if (now - sLastHeartbeat >= 500)
    {
        sHeartbeatState = !sHeartbeatState;
        digitalWrite(LED_BUILTIN, sHeartbeatState ? HIGH : LOW);
        sLastHeartbeat = now;
    }

    AnimatedEvent::process();

    readCommandSerial(Serial, sBuffer, sPos, nullptr);
    readCommandSerial(COMMAND_SERIAL, sBuffer2, sPos2, "[Serial2] ");
}

////////////////

void loop()
{
    uint32_t start = micros();
    mainLoop();
    uint32_t elapsed = micros() - start;
    if (sSkipLoopSample)
    {
        sSkipLoopSample = false;
        return;
    }
    if (elapsed > sLoopMaxUs)
        sLoopMaxUs = elapsed;
    sLoopTotalUs += elapsed;
    sLoopCount++;
}
