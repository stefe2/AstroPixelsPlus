"""Génère les chorégraphies musicales :MUnn à partir des MP3 du Kyber.

Usage (depuis la racine du dépôt) :
    python tools/music/build.py            # analyse (avec cache) et génère tout
    python tools/music/build.py 5 12       # seulement :MU05 et :MU12

Entrées :
    mp3/                     MP3 exacts joués par le Kyber (jamais commités)
    tools/music/songs.json   table des chansons : numéro :MU, fichier, titre, décalage
Sorties :
    src/music/muNN.h         événements de chaque chanson (générés, ne pas modifier)
    src/music/songs.h        table des chansons pour le firmware
    docs/music.md            table de concordance (section générée)
    mp3/preview.html         prévisualisation dans le navigateur (locale, avec la musique)
    ../../R2_Bling/include/music/songs.h   pistes des pieds pour R2-Bling (si le dépôt est présent)
"""

import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)

import analysis            # noqa: E402
import choreography as ch  # noqa: E402
import feet                # noqa: E402
import validate            # noqa: E402

MP3_DIR = os.path.join(ROOT, 'mp3')
CACHE_DIR = os.path.join(MP3_DIR, '_cache')
SONGS_JSON = os.path.join(HERE, 'songs.json')
OUT_DIR = os.path.join(ROOT, 'src', 'music')
FONT_H = os.path.join(ROOT, 'lib', 'Reeltwo', 'src', 'core', 'Font.h')
DOC = os.path.join(ROOT, 'docs', 'music.md')
PREVIEW = os.path.join(MP3_DIR, 'preview.html')
PREVIEW_TEMPLATE = os.path.join(HERE, 'preview_template.html')
LAYOUT_JSON = os.path.join(HERE, 'layout.json')   # disposition du dessin, enregistrée depuis la page
# Dépôt R2-Bling (pieds), à côté de celui-ci : Arduino Projects/R2_Bling. R2_BLING_DIR le remplace.
R2_BLING_DIR = os.environ.get('R2_BLING_DIR', os.path.normpath(os.path.join(ROOT, '..', '..', 'R2_Bling')))

TITLE_CHARS = set(' !-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ')


def clean_title(title):
    """Titre affichable par la police de la logic arrière (majuscules sans accents)."""
    import unicodedata
    t = unicodedata.normalize('NFKD', title).encode('ascii', 'ignore').decode().upper()
    return ''.join(c if c in TITLE_CHARS else ' ' for c in t).strip()


def c_string(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'


def c_mask(mask):
    if mask == ch.ALL:
        return 'MUSIC_ALL_PANELS'
    return ' | '.join('MP(%d)' % p for p in ch.MOVABLE if mask & ch.bit(p))


def write_song_header(song, events, duration_ms):
    num = song['mu']
    lines = [
        '// :MU%02d — %s (%s)' % (num, song['title'], song['file']),
        '// Généré par tools/music/build.py : ne pas modifier à la main.',
        '',
        'static const MusicEvent sMusicMU%02d[] = {' % num,
    ]
    for e in events:
        if e['kind'] == 'cmd':
            lines.append('    MUSIC_CMD(%d, %s),' % (e['t'], c_string(e['cmd'])))
        elif e['kind'] == 'panels':
            lines.append('    MUSIC_PANELS(%d, %s, %d, %d),' % (e['t'], c_mask(e['mask']), e['pct'], e['ms']))
        else:
            lines.append('    MUSIC_HOLO(%d, %d, %d, %d),' % (e['t'], e['holo'], e['pos'], e['ms']))
    lines.append('};')
    with open(os.path.join(OUT_DIR, 'mu%02d.h' % num), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


def write_songs_table(songs, results):
    lines = [
        '// Table des chorégraphies musicales (:MUnn).',
        '// Générée par tools/music/build.py à partir de tools/music/songs.json : ne pas modifier à la main.',
        '',
        '#include "mu99-test.h"',
    ]
    for s in songs:
        if s['mu'] in results:
            lines.append('#include "mu%02d.h"' % s['mu'])
    lines += ['', 'static const MusicSong sMusicSongs[] = {']
    for s in songs:
        if s['mu'] in results:
            r = results[s['mu']]
            lines.append('    MUSIC_SONG(%d, %s, sMusicMU%02d, %d, %d),'
                         % (s['mu'], c_string(s['title']), s['mu'], r['duration_ms'], s.get('offset_ms', 0)))
    lines.append('    MUSIC_SONG(99, "TEST", sMusicMU99, 20000, 0),')
    lines.append('};')
    with open(os.path.join(OUT_DIR, 'songs.h'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


def feet_line(e):
    if e['kind'] == 'beat':
        return '    FEET_BEAT(%d, 0x%02X, %d, %d, %d),' % (e['t'], e['flags'], e['bass'], e['high'], e['bal'])
    if e['kind'] == 'hit':
        return '    FEET_HIT(%d, %d),' % (e['t'], e['strength'])
    return '    FEET_%s(%d, %s, %d, %d, %d),' % (e['kind'].upper(), e['t'], feet.CPP_FX[e['fx']],
                                              e['col'], e['col2'], e['p'])


def write_feet_songs(songs, results):
    """Pistes des pieds dans le dépôt R2-Bling (include/music/songs.h)."""
    out_dir = os.path.join(R2_BLING_DIR, 'include', 'music')
    if not os.path.isdir(os.path.join(R2_BLING_DIR, 'include')):
        print('Dépôt R2-Bling absent (%s) : pistes des pieds non écrites' % R2_BLING_DIR)
        return
    os.makedirs(out_dir, exist_ok=True)
    lines = [
        '// Pistes des pieds (:MUnn) pour R2-Bling.',
        '// Générées par AstroPixelsPlus/tools/music/build.py : ne pas modifier à la main.',
        '#pragma once',
        '#include "MusicTrack.h"',
        '',
    ]
    table = []
    for s in songs:
        r = results.get(s['mu'])
        if not r:
            continue
        lines.append('// :MU%02d — %s' % (s['mu'], s['title']))
        lines.append('static const MusicFeetEvent kFeetMU%02d[] = {' % s['mu'])
        lines += [feet_line(e) for e in r['feet']]
        lines.append('};')
        table.append('    FEET_SONG(%d, kFeetMU%02d, %d, %d),' % (s['mu'], s['mu'], r['duration_ms'], s.get('offset_ms', 0)))
    test, test_ms = feet.test_track()
    lines.append('// :MU99 — piste de test : chaque effet tour à tour')
    lines.append('static const MusicFeetEvent kFeetMU99[] = {')
    lines += [feet_line(e) for e in test]
    lines.append('};')
    table.append('    FEET_SONG(99, kFeetMU99, %d, 0),' % test_ms)
    lines += ['', 'static const MusicFeetSong kFeetSongs[] = {'] + table + ['};']
    with open(os.path.join(out_dir, 'songs.h'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')
    n = sum(len(r['feet']) for r in results.values()) + len(test)
    print('R2-Bling : %d pistes, %d événements (~%d Ko de flash)' % (len(table), n, n * 12 // 1024))


def write_doc_table(songs, results):
    rows = ['| Command | Kyber file | Title shown | Length | Tempo | Sections (intensity 0–3) | Events |',
            '| --- | --- | --- | --- | --- | --- | --- |']
    for s in songs:
        r = results.get(s['mu'])
        if not r:
            continue
        d = r['duration_ms'] / 1000
        rows.append('| `:MU%02d` | `%s` | %s | %d:%02d | %d BPM | %s | %d |' % (
            s['mu'], s['file'], s['title'], d // 60, d % 60, round(r['tempo']),
            ' '.join(str(x) for x in r['tiers']), len(r['events'])))
    table = '\n'.join(rows)
    begin, end = '<!-- table:debut -->', '<!-- table:fin -->'
    with open(DOC, encoding='utf-8') as f:
        doc = f.read()
    doc = re.sub(re.escape(begin) + '.*?' + re.escape(end), begin + '\n' + table + '\n' + end, doc, flags=re.S)
    with open(DOC, 'w', encoding='utf-8', newline='\n') as f:
        f.write(doc)


def write_preview(songs, results):
    data = []
    for s in songs:
        r = results.get(s['mu'])
        if not r:
            continue
        data.append({'mu': s['mu'], 'title': s['title'], 'file': s['file'], 'duration': r['duration_ms'],
                     'titleMs': r['title_ms'], 'tempo': r['tempo'], 'events': r['events'], 'motifs': r['motifs'], 'feet': r['feet'],
                     'beats': [round(b * 1000) for b in r['analysis']['beats']]})
    with open(PREVIEW_TEMPLATE, encoding='utf-8') as f:
        html = f.read()
    html = html.replace('/*SONGS*/[]', json.dumps(data, separators=(',', ':')))
    if os.path.exists(LAYOUT_JSON):
        with open(LAYOUT_JSON, encoding='utf-8') as f:
            html = html.replace('/*LAYOUT*/null', json.dumps(json.load(f), separators=(',', ':')))
    with open(PREVIEW, 'w', encoding='utf-8', newline='\n') as f:
        f.write(html)


def main():
    with open(SONGS_JSON, encoding='utf-8') as f:
        songs = json.load(f)['songs']
    only = {int(x) for x in sys.argv[1:]}
    os.makedirs(OUT_DIR, exist_ok=True)
    results = {}
    failed = False
    for s in songs:
        s['title'] = clean_title(s['title'])
        path = os.path.join(MP3_DIR, s['file'])
        if not s.get('enabled', True) or not os.path.exists(path):
            print(':MU%02d  %-34s absent ou désactivé' % (s['mu'], s['file']))
            continue
        a = analysis.analyze_cached(path, CACHE_DIR)
        events, duration_ms, title_ms, tiers, motifs, plan = ch.choreograph(a, s['title'], FONT_H, LAYOUT_JSON)
        feet_events = feet.feet_track(a, plan)
        errors, stats = validate.validate(events, duration_ms, title_ms)
        errors += validate.validate_feet(feet_events, duration_ms)
        if errors:
            print(':MU%02d  %s : %d erreur(s)' % (s['mu'], s['file'], len(errors)))
            for e in errors[:10]:
                print('    ' + e)
            failed = True
            continue
        results[s['mu']] = {'events': events, 'duration_ms': duration_ms, 'title_ms': title_ms,
                            'tempo': a['tempo'], 'tiers': tiers, 'motifs': motifs, 'feet': feet_events, 'analysis': a}
        if not only or s['mu'] in only:
            write_song_header(s, events, duration_ms)
        n_feet = sum(1 for e in feet_events if e['kind'] != 'beat')
        print(':MU%02d  %-34s %5.1f s  %3d BPM  %4d événements (max %2d/s)  pieds %3d  sections %s'
              % (s['mu'], s['file'], duration_ms / 1000, round(a['tempo']), len(events),
                 stats['max_per_second'], n_feet, ''.join(str(x) for x in tiers)))
    if failed:
        sys.exit('Génération interrompue : corriger les erreurs ci-dessus.')
    write_songs_table(songs, results)
    write_doc_table(songs, results)
    write_feet_songs(songs, results)
    if os.path.exists(PREVIEW_TEMPLATE):
        write_preview(songs, results)
    total = sum(len(r['events']) for r in results.values())
    print('Total : %d chansons, %d événements (~%d Ko de flash)' % (len(results), total, total * 16 // 1024))


if __name__ == '__main__':
    main()
