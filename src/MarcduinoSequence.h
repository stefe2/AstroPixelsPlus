////////////////

#include "servo-sequences-custom.h"

MARCDUINO_ACTION(StopSequence, :SE00, ({
}))

////////////////

MARCDUINO_ACTION(ScreamSequence, :SE01, ({
    // Send command to rear logics
    CommandEvent::process("LE3010003");
    // Send command to front logics
    CommandEvent::process("LE1010003");
    // sMarcSound.handleCommand("$S"); // Son désactivé
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpenClose, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(WaveSequence, :SE02, ({
    // sMarcSound.handleCommand("$213"); // Son désactivé
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelWaveCustom, ALL_DOME_PANELS_MASK);
}))

////////////////

// :SE22 = :SE02 avec interpolation ESP32 (speed=125ms)
// Durée totale : 14×125ms + 290cs×10ms = 1750 + 2900 = 4650ms
MARCDUINO_ACTION(WaveSequenceTest, :SE22, ({
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelWaveCustom, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(SmirkWaveSequence, :SE03, ({
    // sMarcSound.handleCommand("$34"); // Son désactivé
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelWaveFastCustom, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(OpenCloseWaveSequence, :SE04, ({
    // sMarcSound.handleCommand("$36"); // Son désactivé
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelOpenCloseWaveCustom, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ANIMATION(BeepCantinaSequence, :SE05)
{
    DO_START()
    // DO_ONCE({ sMarcSound.handleCommand("$c"); }) // Son désactivé
    // Wait 1 second
    DO_COMMAND(F(
        // Fire logics
        "LE3084115\n"
        "LE1084115\n"
        // Holo Short Circuit
        "HPA002|15\n"))
    DO_SEQUENCE(SeqPanelMarchingAnts, ALL_DOME_PANELS_MASK)
    // Wait 15 seconds
    DO_WAIT_SEC(15)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(ShortSequence, :SE06)
{
    DO_START()
    // Logic engine alarm
    DO_COMMAND(F("LE105000"))
    // Play scream-3 and wait 500ms (son désactivé)
    // DO_ONCE_AND_WAIT({ sMarcSound.handleCommand("$623");}, 500);
    // Logic engine failure
    DO_COMMAND(F(
        // Logic engine failure (all)
        "LE20000\n"
        // Holo Short Circuit
        "HPA007|7\n"
        // Fire strip spark for 1000ms
        "FS11000\n"
        // Charge Bay Indicator flicker for 6s
        "CB20006\n"
        // Data Panel flicker for 6s
        "DP20006\n"
        // Smoke on
        "BMON\n"))
    // Wait 3 seconds
    DO_WAIT_SEC(3)
    DO_COMMAND(F(
        // Smoke off
        "BMOFF\n"
        // Fire strip off
        "FSOFF\n"))
    // Wait 3 seconds
    DO_WAIT_SEC(3)   
    DO_COMMAND(F(
        // Charge Bay Indicator disabled for 8s
        "CB10008\n"
        // Data Panel disabled for 8s
        "DP10008\n"
        // Holo off
        "HPA000|0\n"))
    DO_SEQUENCE_VARSPEED(SeqPanelAllOpenCloseLong, ALL_DOME_PANELS_MASK, 700, 900);
    // Fake being dead for 8 seconds
    DO_WAIT_SEC(8)
    // Ok We are back!
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(CantinaSequence, :SE07)
{
    DO_START()
    // Play Orchestral Cantina
    // DO_ONCE({ sMarcSound.handleCommand("$C"); }) // Son désactivé
    // Wait 1 second
    DO_WAIT_SEC(1)
    DO_COMMAND(F(
        // Disco Logics
        "LE3104146\n"
        "LE1104146\n"
        // Holo Short Circuit
        "HPA006|46\n"))
    DO_SEQUENCE(SeqPanelDanceCustom, ALL_DOME_PANELS_MASK)
    // Wait 46 seconds
    DO_WAIT_SEC(46)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(LeiaMessage, :SE08)
{
    DO_START()
    // DO_ONCE({ sMarcSound.handleCommand("$73"); }) // Son désactivé
    DO_COMMAND_AND_WAIT(F(
        "LE3030045\n"
        "LE1030045\n"
        "HPS1|45\n"
        "CB10000\n"
        "DP10000\n"), 45000)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(DiscoSequence, :SE09)
{
    DO_START()
    // DO_ONCE({ sMarcSound.handleCommand("$D"); }) // Son désactivé
    DO_SEQUENCE(SeqPanelLongDiscoCustom, ALL_DOME_PANELS_MASK)
    DO_ONCE({
        FLD.selectSequence(LogicEngineRenderer::RAINBOW);
        RLD.selectScrollTextLeft("STAR WARS R2-D2 ASTROMECH", LogicEngineRenderer::ColorVal(random(10)));
    })
    DO_COMMAND_AND_WAIT(F(
        "HPS1|45\n"
        "CB10000\n"
        "DP10000\n"), 45000)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////
// SE23-SE29 : versions interpolées (speed=125ms) des séquences SE03-SE09
// Les séquences originales SE03-SE09 sont conservées comme référence (speed=0, Maestro natif)
////////////////

MARCDUINO_ACTION(SmirkWaveSequenceInterp, :SE23, ({
    // :SE03 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelWaveFastCustom, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(OpenCloseWaveSequenceInterp, :SE24, ({
    // :SE04 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelOpenCloseWaveCustom, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ANIMATION(BeepCantinaSequenceInterp, :SE25)
{
    DO_START()
    // :SE05 avec interpolation ESP32 speed=125ms
    DO_COMMAND(F(
        "LE3084115\n"
        "LE1084115\n"
        "HPA002|15\n"))
    DO_SEQUENCE_SPEED(SeqPanelMarchingAnts, ALL_DOME_PANELS_MASK, 125)
    DO_WAIT_SEC(15)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(ShortSequenceInterp, :SE26)
{
    DO_START()
    // :SE06 avec interpolation ESP32 speed=125ms
    DO_COMMAND(F("LE105000"))
    DO_COMMAND(F(
        "LE20000\n"
        "HPA007|7\n"
        "FS11000\n"
        "CB20006\n"
        "DP20006\n"
        "BMON\n"))
    DO_WAIT_SEC(3)
    DO_COMMAND(F(
        "BMOFF\n"
        "FSOFF\n"))
    DO_WAIT_SEC(3)
    DO_COMMAND(F(
        "CB10008\n"
        "DP10008\n"
        "HPA000|0\n"))
    DO_SEQUENCE_VARSPEED(SeqPanelAllOpenCloseLong, ALL_DOME_PANELS_MASK, 125, 125);
    DO_WAIT_SEC(8)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(CantinaSequenceInterp, :SE27)
{
    DO_START()
    // :SE07 avec interpolation ESP32 speed=125ms
    DO_WAIT_SEC(1)
    DO_COMMAND(F(
        "LE3104146\n"
        "LE1104146\n"
        "HPA006|46\n"))
    DO_SEQUENCE_SPEED(SeqPanelDanceCustom, ALL_DOME_PANELS_MASK, 125)
    DO_WAIT_SEC(46)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(LeiaMessageInterp, :SE28)
{
    DO_START()
    // :SE08 — pas de servos panneaux, copie identique
    DO_COMMAND_AND_WAIT(F(
        "LE3030045\n"
        "LE1030045\n"
        "HPS1|45\n"
        "CB10000\n"
        "DP10000\n"), 45000)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(DiscoSequenceInterp, :SE29)
{
    DO_START()
    // :SE09 avec interpolation ESP32 speed=125ms
    DO_SEQUENCE_SPEED(SeqPanelLongDiscoCustom, ALL_DOME_PANELS_MASK, 125)
    DO_ONCE({
        FLD.selectSequence(LogicEngineRenderer::RAINBOW);
        RLD.selectScrollTextLeft("STAR WARS R2-D2 ASTROMECH", LogicEngineRenderer::ColorVal(random(10)));
    })
    DO_COMMAND_AND_WAIT(F(
        "HPS1|45\n"
        "CB10000\n"
        "DP10000\n"), 45000)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////
// SE30-SE38 : versions interpolées (speed=125ms) des séquences SE50-SE58
// Les séquences originales SE50-SE58 sont conservées comme référence (speed=0, Maestro natif)
////////////////

MARCDUINO_ACTION(ScreamNoPanelSequenceInterp, :SE30, ({
    // :SE50 — pas de servos panneaux, copie identique
    CommandEvent::process("LE3010003");
    CommandEvent::process("LE1010003");
}))

////////////////

MARCDUINO_ACTION(ScreamPanelSequenceInterp, :SE31, ({
    // :SE51 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelAllOpenClose, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(WavePanelSequenceInterp, :SE32, ({
    // :SE52 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelWaveCustomSlow, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(SmirkWavePanelSequenceInterp, :SE33, ({
    // :SE53 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelWaveFastCustom, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(OpenWaveSequenceInterp, :SE34, ({
    // :SE54 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelOpenCloseWaveCustom, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(MarchingAntsPanelSequenceInterp, :SE35, ({
    // :SE55 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelMarchingAnts, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(FaintPanelSequenceInterp, :SE36, ({
    // :SE56 avec interpolation ESP32 speed=125ms
    DO_SEQUENCE_VARSPEED(SeqPanelAllOpenCloseLong, ALL_DOME_PANELS_MASK, 125, 125);
}))

////////////////

MARCDUINO_ACTION(RythmicPanelSequenceInterp, :SE37, ({
    // :SE57 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelAllOpenCloseLong, ALL_DOME_PANELS_MASK, 125);
}))

////////////////

MARCDUINO_ACTION(OneByOnePanelSequenceInterp, :SE38, ({
    // :SE58 avec interpolation ESP32 speed=125ms
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelOneByOne, 0xFFFFFFFF, 125);
}))

////////////////

MARCDUINO_ACTION(ScreamNoPanelSequence, :SE50, ({
    CommandEvent::process("LE3010003");
    CommandEvent::process("LE1010003");
    // sMarcSound.handleCommand("$S"); // Son désactivé
}))

////////////////

MARCDUINO_ACTION(ScreamPanelSequence, :SE51, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpenClose, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(WavePanelSequence, :SE52, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelWaveCustomSlow, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(SmirkWavePanelSequence, :SE53, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelWaveFastCustom, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(OpenWaveSequence, :SE54, ({
    // sMarcSound.handleCommand("$36"); // Son désactivé
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelOpenCloseWaveCustom, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(MarchingAntsPanelSequence, :SE55, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelMarchingAnts, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(FaintPanelSequence, :SE56, ({
    DO_SEQUENCE_VARSPEED(SeqPanelAllOpenCloseLong, ALL_DOME_PANELS_MASK, 700, 900);
}))

////////////////

MARCDUINO_ACTION(RythmicPanelSequence, :SE57, ({
    SEQUENCE_PLAY_ONCE_SPEED(servoSequencer, SeqPanelAllOpenCloseLong, ALL_DOME_PANELS_MASK, 900);
}))

////////////////

MARCDUINO_ACTION(OneByOnePanelSequence, :SE58, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelOneByOne, 0xFFFFFFFF);
}))

////////////////

MARCDUINO_ANIMATION(HarlemShakeSequence, $815)
{
    DO_START()
    // Wait 2 seconds
    DO_WAIT_SEC(2)
    DO_COMMAND(F(
        // Fire logics
        "LE3070000\n"
        "LE1070000\n"
        // Holo Short Circuit
        "HPA006|11\n"))
    // Wait 0.5 second
    DO_WAIT_MILLIS(500)
    // Start panel sequence
    DO_SEQUENCE(SeqPanelLongHarlemShakeCustom, ALL_DOME_PANELS_MASK)
    // Wait 10 second
    DO_WAIT_SEC(11)
    DO_COMMAND(F(
        // Fire logics
        "LE100000\n"
        // Holo Short Circuit
        "HPA002|12\n"))

    // Beginning of Shake loop
    DO_LABEL(shake)
    // Play random
    DO_SEQUENCE_RANDOM_STEP(SeqPanelLongHarlemShakeCustom, ALL_DOME_PANELS_MASK)
    // Wait 50 seconds
    DO_WAIT_MILLIS(50)
    // Loop until total play time reaches 26.5 seconds
    DO_DURATION(26500, { animation.gotoStep(shake); })
    // Start panel sequence
    DO_SEQUENCE(SeqPanelAllOpenCloseLong, ALL_DOME_PANELS_MASK)
    // Wait 2 seconds
    DO_WAIT_SEC(2)

    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(GirlOnFireSequence, $821)
{
    DO_START()
    // Wait 3.5 seconds
    DO_WAIT_MILLIS(3500)
    DO_SEQUENCE(SeqPanelDanceCustom, ALL_DOME_PANELS_MASK)
    DO_COMMAND(F(
        // Fire logics
        "LE220055\n"
        // Holo Short Circuit
        "HPA00318|53\n"))
    // Wait 34 seconds
    DO_WAIT_SEC(34)
    // Smoke on
   DO_COMMAND(F("BMON"))
    DO_COMMAND(F(
        // Fire strip for 10000ms
        "FS210000\n"
        // Data panel flicker for 5 seconds
        "DP20010\n"
        // Charge Bay Indicator flicker for 5s
        "CB20010\n"
        // Data panel flicker for 5 seconds
        "DP20010\n"))
    // Wait 8 seconds
    DO_WAIT_SEC(8)
    // Smoke off
    DO_COMMAND(F("BMOFF"))
    // Wait 10 seconds
    DO_WAIT_SEC(10)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

MARCDUINO_ANIMATION(YodaClearMind, $720)
{
    DO_START()
    DO_SEQUENCE(SeqPanelAllOpen, PANEL_GROUP_6)
    DO_COMMAND(F(
        // Yoda LED sequence
        "HPO006|15\n"))
    // Wait 15 seconds
    DO_WAIT_SEC(15)
    DO_SEQUENCE(SeqPanelAllClose, PANEL_GROUP_6)
    DO_RESET({
        resetSequence();
    })
    DO_END()
}

////////////////

