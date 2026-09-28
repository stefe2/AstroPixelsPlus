////////////////

MARCDUINO_ACTION(CloseAllPanels, :CL00, ({
    // Ferme tous les panneaux avec interpolation 125ms (filet de sécurité)
    servoDispatch.moveServosTo(ALL_DOME_PANELS_MASK, 125, 0.0);
}))

////////////////

MARCDUINO_ACTION(StopAllServos, :ST00, ({
    servoDispatch.stop();
}))

////////////////

MARCDUINO_ACTION(DisableServo, :SD, ({
    int32_t args[1] = { 0 };
    char* cmd = (char*)Marcduino::getCommand();
    uint8_t argcount = 0;
    numberparams(cmd, argcount, args, SizeOfArray(args));
    if (argcount >= 1)
    {
        servoDispatch.disable(args[0]);
    }
}))

////////////////

MARCDUINO_ACTION(OpenAllPanels, :OP00, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, ALL_DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(FlutterAllPanels, :OF00, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, ALL_DOME_PANELS_MASK, 10, 50);
}))

////////////////

MARCDUINO_ACTION(SetServoEasing, :SF, ({
    uint32_t group = 0;
    char* cmd = (char*)Marcduino::getCommand();
    char* gstr = strchr(cmd, '$');
    if (gstr != nullptr)
    {
        *gstr++ = '\0';
        group = strtol(gstr, 0, 16);
    }
    Easing::Method method = Easing::getEasingMethod(strtol(cmd, 0, 10));
    if (method == nullptr)
        method = Easing::LinearInterpolation;
    if (group != 0)
        servoDispatch.setServosEasingMethod(group, method);
}))

MARCDUINO_ACTION(SetServoPosition, :SQ, ({
    int32_t args[4] = { 0, 0, 0, 0 };
    char* cmd = (char*)Marcduino::getCommand();
    uint8_t argcount = 0;
    numberparams(cmd, argcount, args, SizeOfArray(args));
    if (argcount >= 2)
    {
        servoDispatch.moveToPulse(args[0], args[1]);
    }
}))

MARCDUINO_ACTION(SetServoLimits, :SL, ({
    int32_t args[5] = { 0, 0, 0, 0, 0 };
    char* cmd = (char*)Marcduino::getCommand();
    uint8_t argcount = 0;
    numberparams(cmd, argcount, args, SizeOfArray(args));
    if (argcount >= 3)
    {
        servoDispatch.setServo(args[0],
            servoDispatch.getPin(args[0]),
            args[1], /* start pulse */
            args[2], /* end pulse */
            (argcount >= 4) ? args[3] : args[1], /* neutral pulse */
            (argcount >= 5) ? args[4] : servoDispatch.getGroup(args[0])); /* group */
    }
}))

MARCDUINO_ACTION(MoveServos, :SM, ({
    int32_t args[5] = { 0, 0, 0, 0, 0 };
    char* cmd = (char*)Marcduino::getCommand();
    uint8_t argcount = 0;
    numberparams(cmd, argcount, args, SizeOfArray(args));
    if (argcount == 2)
    {
        servoDispatch.moveToPulse(args[0], args[1]);
    }
    else if (argcount == 3)
    {
        servoDispatch.moveToPulse(args[0], args[1], args[2]);
    }
    else if (argcount == 4)
    {
        servoDispatch.moveToPulse(args[0], args[1], args[2], args[3]);
    }
    else if (argcount >= 5)
    {
        servoDispatch.moveToPulse(args[0], args[1], args[2], args[3], args[4]);
    }
}))

// Commandes dynamiques :XX$<masque hex>[,<vitesse min>,<vitesse max>,<easing ouverture>,<easing fermeture>]
// Vitesses en ms (défaut 10 et 50), easing 0 = aucun. Rien n'est fait si le masque vaut 0.
static void playPanelGroupDynamic(const ServoStep* sequence, uint16_t length)
{
    int32_t args[4] = { 10, 50, 0, 0 };
    char* cmd = (char*)Marcduino::getCommand();
    char* pstr = strchr(cmd, ',');
    if (pstr != nullptr)
    {
        *pstr++ = '\0';
        uint8_t argcount = 0;
        numberparams(pstr, argcount, args, SizeOfArray(args));
    }
    uint32_t group = strtol(cmd, 0, 16);
    if (group != 0)
    {
        Easing::Method onEasing = Easing::getEasingMethod(args[2]);
        Easing::Method offEasing = Easing::getEasingMethod(args[3]);
        servoSequencer.playVariableSpeed(sequence, length, group, args[0], args[1], 0.0, 1.0, onEasing, offEasing);
    }
}
#define PLAY_PANEL_GROUP_DYNAMIC(sequence) playPanelGroupDynamic(sequence, SizeOfArray(sequence))

MARCDUINO_ACTION(OpenCloseRepeatPanelGroupDynamic, :OCR$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelAllFOpenCloseRepeat);
}))

MARCDUINO_ACTION(FlutterPanelGroupDynamic, :OF$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelAllFlutter);
}))

MARCDUINO_ACTION(OpenClosePanelGroupDynamic, :OC$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelAllOpenClose);
}))

MARCDUINO_ACTION(OpenClosePanelLongGroupDynamic, :OCL$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelAllOpenCloseLong);
}))

MARCDUINO_ACTION(WavePanelGroupDynamic, :OW$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelWave);
}))

MARCDUINO_ACTION(FastWavePanelGroupDynamic, :OWF$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelWaveFast);
}))

MARCDUINO_ACTION(OpenCloseWavePanelGroupDynamic, :OWC$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelOpenCloseWave);
}))

MARCDUINO_ACTION(MarchingAntPanelGroupDynamic, :OMA$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelMarchingAnts);
}))

MARCDUINO_ACTION(AlternatePanelGroupDynamic, :OAP$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelAlternate);
}))

MARCDUINO_ACTION(DancePanelGroupDynamic, :OD$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelDance);
}))

MARCDUINO_ACTION(ShakePanelGroupDynamic, :OS$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelLongHarlemShake);
}))

MARCDUINO_ACTION(OpenPanelGroupDynamic, :OP$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelAllOpen);
}))

MARCDUINO_ACTION(ClosePanelGroupDynamic, :CL$, ({
    PLAY_PANEL_GROUP_DYNAMIC(SeqPanelAllClose);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup1, :OP01, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_1);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup2, :OP02, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_2);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup3, :OP03, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_3);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup4, :OP04, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_4);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup5, :OP05, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_5);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup6, :OP06, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_6);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup7, :OP07, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_7);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup8, :OP08, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_8);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup9, :OP09, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_9);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup10, :OP10, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_10);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup11, :OP11, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_11);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup12, :OP12, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_12);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup13, :OP13, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_13);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup14, :OP14, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_14);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup15, :OP15, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_15);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup16, :OP16, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_16);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup17, :OP17, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_17);
}))

////////////////

MARCDUINO_ACTION(OpenPanelGroup18, :OP18, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PANEL_GROUP_18);
}))

////////////////

MARCDUINO_ACTION(OpenTopPanels, :OP19, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, PIE_PANEL);
}))

////////////////

MARCDUINO_ACTION(OpenBottomPanels, :OP20, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllOpen, DOME_PANELS_MASK);
}))


MARCDUINO_ACTION(ClosePanelGroup1, :CL01, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_1);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup2, :CL02, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_2);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup3, :CL03, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_3);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup4, :CL04, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_4);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup5, :CL05, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_5);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup6, :CL06, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_6);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup7, :CL07, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_7);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup8, :CL08, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_8);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup9, :CL09, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_9);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup10, :CL10, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_10);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup11, :CL11, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_11);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup12, :CL12, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_12);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup13, :CL13, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_13);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup14, :CL14, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_14);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup15, :CL15, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_15);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup16, :CL16, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_16);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup17, :CL17, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_17);
}))

////////////////

MARCDUINO_ACTION(ClosePanelGroup18, :CL18, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PANEL_GROUP_18);
}))

////////////////

MARCDUINO_ACTION(CloseTopPanels, :CL19, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, PIE_PANEL);
}))

////////////////

MARCDUINO_ACTION(CloseBottomPanels, :CL20, ({
    SEQUENCE_PLAY_ONCE(servoSequencer, SeqPanelAllClose, DOME_PANELS_MASK);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup1, :OF01, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_1, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup2, :OF02, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_2, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup3, :OF03, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_3, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup4, :OF04, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_4, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup5, :OF05, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_5, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup6, :OF06, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_6, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup7, :OF07, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_7, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup8, :OF08, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_8, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup9, :OF09, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_9, 10, 50);
}))

////////////////

MARCDUINO_ACTION(FlutterPanelGroup10, :OF10, ({
    SEQUENCE_PLAY_ONCE_VARSPEED(servoSequencer, SeqPanelAllFlutter, PANEL_GROUP_10, 10, 50);
}))
