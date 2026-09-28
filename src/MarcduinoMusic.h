////////////////
// Chorégraphies musicales :MU01–:MU99
//
// Le Kyber lance la musique et envoie :MUnn au même moment. Une chanson est une liste
// d'événements horodatés (src/music/), déroulée par le lecteur d'animations comme les
// séquences $815 ou :SE07 : toute autre commande l'interrompt, et sa remise à zéro s'exécute.
// :MU00 (ou un numéro inconnu) arrête simplement la chanson en cours.
//
// Au départ, le titre de la chanson défile en blanc sur la logic arrière ; les chorégraphies
// générées n'y envoient rien avant la fin du défilement. Pendant la chanson, les mouvements
// aléatoires des holos (*HA, *HV) sont suspendus ; ils reprennent à la fin s'ils étaient actifs.
// Chorégraphies générées par tools/music/build.py (voir docs/music.md).

enum MusicEventKind : uint8_t
{
    kMusicPanels,   // panneaux (P1–P13) vers une position (0 = fermé, 100 = ouvert)
    kMusicHolo,     // un holo (ou les trois) vers une position de moveHP() (0–8)
    kMusicCommand   // commandes Reeltwo (LE…, HP…) séparées par \n
};

// 16 octets par événement : une chanson de 3 min en compte environ un millier
struct MusicEvent
{
    uint32_t timeMs;        // depuis le début de la chanson
    const char* cmd;        // commandes
    uint16_t mask;          // panneaux : un bit par panneau du plan, MP(1)…MP(13)
    uint16_t moveMs;        // durée du mouvement
    MusicEventKind kind;
    uint8_t a;              // panneaux : position en % ; holo : MUSIC_HOLO_*
    uint8_t b;              // holo : position (0 bas, 1 centre, 2 haut, 3 gauche, 6 droite…)
};

struct MusicSong
{
    uint8_t number;         // :MUnn
    const char* title;
    const MusicEvent* events;
    uint16_t count;
    uint32_t durationMs;    // fin de la chanson (remise à zéro)
    int16_t offsetMs;       // décalage : > 0 retarde la chorégraphie par rapport à la commande
};

#define MUSIC_HOLO_FRONT 0
#define MUSIC_HOLO_REAR  1
#define MUSIC_HOLO_TOP   2
#define MUSIC_HOLO_ALL   3

// Panneau n du plan du dôme (P1–P13) ; décalé vers PANEL_GROUP_n de servoSettings[] à l'exécution.
// P6 et P13 n'ont pas de servo : les chorégraphies ne les commandent pas.
#define MP(n) (1u << ((n) - 1))
#define MUSIC_ALL_PANELS (MP(1) | MP(2) | MP(3) | MP(4) | MP(5) | MP(7) | MP(8) | MP(9) | MP(10) | MP(11) | MP(12))
static_assert(PANEL_GROUP_1 == (1L << 14), "MP(n) << 14 doit donner PANEL_GROUP_n");

#define MUSIC_PANELS(t, mask, pct, ms) { (t), nullptr, (mask), (ms), kMusicPanels, (pct), 0 }
#define MUSIC_HOLO(t, holo, pos, ms)   { (t), nullptr, 0, (ms), kMusicHolo, (holo), (pos) }
#define MUSIC_CMD(t, cmd)              { (t), (cmd), 0, 0, kMusicCommand, 0, 0 }
#define MUSIC_SONG(num, title, events, durationMs, offsetMs) \
    { (num), (title), (events), SizeOfArray(events), (durationMs), (offsetMs) }

#include "music/songs.h"

static const MusicSong* sMusicSong = nullptr;
static uint16_t sMusicIndex;
static bool sMusicResumeHoloAlive;
static bool sMusicResumeHoloLED;

static const MusicSong* findMusicSong(unsigned number)
{
    for (unsigned i = 0; i < SizeOfArray(sMusicSongs); i++)
    {
        if (sMusicSongs[i].number == number)
            return &sMusicSongs[i];
    }
    return nullptr;
}

static void musicBegin(const MusicSong* song)
{
    sMusicSong = song;
    sMusicIndex = 0;
    servoSequencer.abortQuietly();
    sMusicResumeHoloAlive = holoAlive.isEnabled();
    sMusicResumeHoloLED = holoLED.isEnabled();
    holoAlive.suspend();
    if (sMusicResumeHoloLED)
        holoLED.disable();
    RLD.selectScrollTextLeft(song->title, LogicEngineRenderer::kWhite, 0, 0);
    printf("Music :MU%02u %s\n", song->number, song->title);
}

static void musicEnd()
{
    if (sMusicSong == nullptr)
        return;
    sMusicSong = nullptr;
    resetSequence();
    servoDispatch.moveServosTo(ALL_DOME_PANELS_MASK, 250, 0.0);
    if (sMusicResumeHoloAlive)
        holoAlive.resume();
    else
        holoAlive.disable();   // holos au centre
    if (sMusicResumeHoloLED)
        holoLED.enable();
}

static void musicRunEvent(const MusicEvent& ev)
{
    switch (ev.kind)
    {
    case kMusicPanels:
        servoDispatch.moveServosTo((uint32_t)ev.mask << 14, ev.moveMs, ev.a / 100.0f);
        break;
    case kMusicHolo:
    {
        HoloLights* holos[3] = { &frontHolo, &rearHolo, &topHolo };
        for (unsigned i = 0; i < 3; i++)
        {
            if (ev.a == MUSIC_HOLO_ALL || ev.a == i)
                holos[i]->moveHP(ev.b, ev.moveMs);
        }
        break;
    }
    case kMusicCommand:
        CommandEvent::process(reinterpret_cast<PROGMEMString>(ev.cmd));
        break;
    }
}

// Exécute les événements échus ; faux quand la chanson est terminée
static bool musicTick(unsigned long elapsedMs)
{
    int32_t t = (int32_t)elapsedMs - sMusicSong->offsetMs;
    while (sMusicIndex < sMusicSong->count && t >= (int32_t)sMusicSong->events[sMusicIndex].timeMs)
    {
        musicRunEvent(sMusicSong->events[sMusicIndex]);
        sMusicIndex++;
    }
    return t < (int32_t)sMusicSong->durationMs;
}

// Étape de l'animation :MU (même contrat que les MARCDUINO_ANIMATION) :
// ~0 = remise à zéro, 0 = démarrage, 1 = lecture jusqu'à la fin
ANIMATION_FUNC_DECL(MusicPlay)
{
    UNUSED_ARG(num)
    if (step == (unsigned)~0u)
    {
        musicEnd();
        return true;
    }
    if (step == 0)
    {
        const MusicSong* song = findMusicSong(atoi(Marcduino::getCommand()));
        if (song == nullptr)
        {
            animation.end();
            return false;
        }
        musicBegin(song);
        return true;
    }
    if (musicTick(elapsedMillis))
        return false;   // rester sur cette étape
    animation.end();
    return false;
}
const char _marc_msg_MusicPlay[] PROGMEM = ":MU";
Marcduino Marc_MusicPlay(Animation_MusicPlay, _marc_msg_MusicPlay);
