// :MU99 — chorégraphie de test écrite à la main (20 s, sans musique) pour vérifier le lecteur

#define MU99_PIES   (MP(7) | MP(8) | MP(9) | MP(10))
#define MU99_SMALLS (MP(1) | MP(2) | MP(3))

static const MusicEvent sMusicMU99[] = {
    // Logics bleues qui flashent, holos bleus
    // (la logic arrière affiche le titre « TEST » jusqu'à ~4,2 s)
    MUSIC_CMD(0,     "LE1066000\nHPA0055"),
    MUSIC_PANELS(1000,  MU99_PIES, 100, 300),
    MUSIC_PANELS(2000,  MU99_PIES, 0, 300),
    MUSIC_HOLO(2500,    MUSIC_HOLO_FRONT, 3, 400),
    MUSIC_HOLO(2500,    MUSIC_HOLO_REAR, 6, 400),
    MUSIC_PANELS(3000,  MU99_SMALLS | MP(4) | MP(5), 100, 300),
    MUSIC_PANELS(4000,  MU99_SMALLS | MP(4) | MP(5), 0, 300),
    MUSIC_CMD(4500,  "LE3066000"),
    // Logics en feu, PSI en arc-en-ciel
    MUSIC_CMD(5000,  "LE1220000\nLE3220000\nLE4100000\nLE5100000"),
    MUSIC_PANELS(6000,  MUSIC_ALL_PANELS, 100, 400),
    MUSIC_HOLO(6000,    MUSIC_HOLO_ALL, 2, 400),
    MUSIC_PANELS(8000,  MUSIC_ALL_PANELS, 0, 400),
    MUSIC_HOLO(8000,    MUSIC_HOLO_ALL, 0, 400),
    // Holos rouges, mini-panneaux
    MUSIC_CMD(9000,  "HPA0051"),
    MUSIC_PANELS(10000, MP(11) | MP(12), 100, 250),
    MUSIC_PANELS(10500, MP(11) | MP(12), 0, 250),
    // Alternance pie / petits panneaux
    MUSIC_PANELS(11000, MU99_PIES, 100, 250),
    MUSIC_PANELS(11500, MU99_PIES, 0, 250),
    MUSIC_PANELS(11500, MU99_SMALLS, 100, 250),
    MUSIC_PANELS(12000, MU99_SMALLS, 0, 250),
    MUSIC_PANELS(12000, MU99_PIES, 100, 250),
    MUSIC_PANELS(12500, MU99_PIES, 0, 250),
    MUSIC_PANELS(12500, MU99_SMALLS, 100, 250),
    MUSIC_PANELS(13000, MU99_SMALLS, 0, 250),
    // Un panneau à la fois : P1, P2, P3, P4
    MUSIC_PANELS(13250, MP(1), 100, 200),
    MUSIC_PANELS(13500, MP(1), 0, 200),
    MUSIC_PANELS(13500, MP(2), 100, 200),
    MUSIC_PANELS(13750, MP(2), 0, 200),
    MUSIC_PANELS(13750, MP(3), 100, 200),
    MUSIC_PANELS(14000, MP(3), 0, 200),
    MUSIC_PANELS(14000, MP(4), 100, 200),
    MUSIC_PANELS(14500, MP(4), 0, 200),
    // Arc-en-ciel partout, panneaux à mi-course
    MUSIC_CMD(15000, "LE1100000\nLE3100000\nHPA006"),
    MUSIC_PANELS(16000, MUSIC_ALL_PANELS, 50, 300),
    MUSIC_PANELS(17000, MUSIC_ALL_PANELS, 0, 300),
    MUSIC_HOLO(18000,   MUSIC_HOLO_ALL, 1, 500),
};
