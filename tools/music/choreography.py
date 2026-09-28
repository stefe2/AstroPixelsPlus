"""Règles de chorégraphie : transforme l'analyse d'une chanson en événements pour le droïde.

Chaque section de la chanson reçoit un niveau d'intensité (0 calme, 1 groove, 2 énergie,
3 apogée) et un motif de panneaux choisi dans le répertoire de ce niveau. Les sections qui se
ressemblent (refrains) reprennent le même motif. Les règles sont décrites dans docs/music.md.
Les temps des événements sont en millisecondes depuis le début du fichier audio.
"""

import hashlib
import json
import math
import os
import random
import re

# Panneaux du plan du dôme (P1–P13) : un bit par panneau, bit n-1 pour Pn. Le firmware
# décale ce masque vers PANEL_GROUP_n (voir MarcduinoMusic.h).
# P6 et P13 n'ont pas de servo sur le droïde : jamais commandés.
MOVABLE = [1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12]
SMALLS, MEDIUMS, PIES, MINIS = [1, 2, 3], [4, 5], [7, 8, 9, 10], [11, 12]


def bit(p):
    return 1 << (p - 1)


ALL = sum(bit(p) for p in MOVABLE)

# Ordre de l'« égaliseur » : du plus discret au plus spectaculaire
EQ_ORDER = [MINIS, SMALLS, MEDIUMS, PIES]

# Appareils LogicEngine (LE<id>…)
FLD, RLD, PSIF, PSIR = 1, 3, 4, 5

# Effets LogicEngine
NORMAL, SOLID, FLASH, FLIPFLOP, RAINBOW, LIGHTSOUT, FIRE = 0, 5, 6, 7, 10, 14, 22

# Couleurs LogicEngine 1–9 dans l'ordre du cercle chromatique, et code couleur holo équivalent
LOGIC_COLORS = ['rouge', 'orange', 'jaune', 'vert', 'cyan', 'bleu', 'violet', 'magenta', 'rose']
HOLO_COLOR = {1: 1, 2: 7, 3: 2, 4: 3, 5: 4, 6: 5, 7: 8, 8: 6, 9: 6}
HOLO_WHITE = 9

# Positions de moveHP()
DOWN, CENTER, UP, LEFT, UP_LEFT, DOWN_LEFT, RIGHT, UP_RIGHT, DOWN_RIGHT = range(9)
HOLOS = [0, 1, 2]          # avant, arrière, dessus (MUSIC_HOLO_*)
HOLO_ALL = 3
HOLO_LETTER = {0: 'F', 1: 'R', 2: 'T', 3: 'A'}

# Logic arrière : 27 pixels de large, défilement d'un pixel toutes les 50 ms, 2 s d'écran noir avant
RLD_WIDTH = 27
SCROLL_MS_PER_PX = 50
TEXT_BLANK_MS = 2000

# Limites mécaniques
PANEL_MIN_GAP_MS = 250     # entre deux commandes d'un même panneau
PANEL_MIN_MOVE_MS = 150
HOLO_MIN_GAP_MS = 300
LOGIC_MIN_GAP_MS = 110

# Répertoire des motifs de panneaux par niveau d'intensité
MOTIF_NAMES = {
    'breathe': 'respiration', 'sparkle': 'scintillement', 'morse': 'morse', 'solo': 'solo',
    'knight': 'aller-retour', 'wave': 'vague', 'stereo': 'gauche / droite', 'heartbeat': 'battement de cœur',
    'callresp': 'question / réponse', 'wheel': 'roue des pie', 'mirror': 'miroir', 'dominos': 'dominos',
    'accordion': 'accordéon', 'groups': 'groupes', 'equalizer': 'égaliseur',
}
MOTIFS = {
    0: ['breathe', 'sparkle', 'morse', 'solo'],
    1: ['solo', 'knight', 'wave', 'stereo', 'heartbeat', 'callresp'],
    2: ['wheel', 'mirror', 'dominos', 'accordion', 'stereo', 'groups'],
    3: ['equalizer', 'wheel', 'dominos', 'mirror'],
}

# Disposition du plan (tools/music/layout.json) si le fichier est absent
CENTER_XY = (230, 350)
DEFAULT_LAYOUT = {
    'P1': {'a': 48.8, 'r0': 160.7, 'r1': 176.7}, 'P2': {'a': 36.3, 'r0': 161.3, 'r1': 177.3},
    'P3': {'a': 23.9, 'r0': 160.7, 'r1': 176.7}, 'P4': {'a': 4.1, 'r0': 160.2, 'r1': 176.2},
    'P5': {'a': -45.7, 'r0': 160.2, 'r1': 176.2}, 'P11': {'a': 125.6, 'r0': 160.3, 'r1': 176.3},
    'P12': {'a': 103.7, 'r0': 160.2, 'r1': 176.2}, 'P7': {'a': 180, 'r0': 42, 'r1': 112},
    'P8': {'a': 120, 'r0': 42, 'r1': 112}, 'P9': {'a': 60, 'r0': 42, 'r1': 112},
    'P10': {'a': 0, 'r0': 42, 'r1': 112},
    'FHP': {'x': 296.2, 'y': 493.7}, 'RHP': {'x': 198.8, 'y': 194.5}, 'THP': {'x': 267, 'y': 284},
    'FPSI': {'x': 234.2, 'y': 507.7}, 'RPSI': {'x': 270.2, 'y': 196.9},
    'FLD': {'x': 99.4, 'y': 527.1}, 'RLD': {'x': 10, 'y': 143.1},
}


def font_advances(font_h):
    """Largeur (en pixels) de chaque caractère de la police 4 pt de la logic arrière."""
    with open(font_h, encoding='utf-8') as f:
        s = f.read()
    i = s.index('class LatinFontVar4pt')
    seg = s[i:]
    seg = seg[:seg.index('};')]
    return {ch: int(a) for ch, a in re.findall(r"'(.)', (\d+),", seg)}


def title_scroll_ms(title, advances):
    """Durée d'affichage du titre sur la logic arrière (écran noir + défilement complet)."""
    width = sum(advances.get(c, 3) + 1 for c in title)
    return TEXT_BLANK_MS + (RLD_WIDTH + width) * SCROLL_MS_PER_PX


class Dome:
    """Géométrie du dôme vue de dessus (avant en bas), d'après la disposition du plan."""

    def __init__(self, layout_path=None):
        lay = dict(DEFAULT_LAYOUT)
        if layout_path and os.path.exists(layout_path):
            with open(layout_path, encoding='utf-8') as f:
                lay.update(json.load(f))
        cx, cy = CENTER_XY
        self.pos = {}      # panneau -> (x, y) absolus
        self.angle = {}
        for p in MOVABLE:
            L = lay['P%d' % p]
            a = math.radians(L['a'])
            r = (L['r0'] + L['r1']) / 2
            self.pos[p] = (cx + r * math.cos(a), cy + r * math.sin(a))
            self.angle[p] = L['a'] % 360
        self.dev = {k: (lay[k]['x'], lay[k]['y']) for k in ('FHP', 'RHP', 'THP', 'FPSI', 'RPSI', 'FLD', 'RLD')}
        outer = [p for p in MOVABLE if p not in PIES]
        # Tour du dôme (tous les panneaux par angle), roue des pie, ligne de l'anneau extérieur
        self.ring = sorted(MOVABLE, key=lambda p: self.angle[p])
        self.pie_ring = sorted(PIES, key=lambda p: self.angle[p])
        self.line = sorted(outer, key=lambda p: (self.angle[p] + 90) % 360)
        # Côtés vus du public (devant le droïde) : droite = x plus grand que le centre
        self.right = [p for p in MOVABLE if self.pos[p][0] > cx + 5]
        self.left = [p for p in MOVABLE if self.pos[p][0] < cx - 5]
        self.outer = outer
        # Rangées de l'avant vers l'arrière (accordéon)
        by_y = sorted(MOVABLE, key=lambda p: -self.pos[p][1])
        self.rows = [by_y[i:i + 3] for i in range(0, len(by_y), 3)]
        # Paires en miroir gauche/droite (la plus proche du symétrique), restes appariés entre voisins
        left = list(MOVABLE)
        self.pairs = []
        while left:
            p = left.pop(0)
            mx = 2 * cx - self.pos[p][0]
            if not left:
                self.pairs.append([p])
                break
            q = min(left, key=lambda q: math.hypot(self.pos[q][0] - mx, self.pos[q][1] - self.pos[p][1]))
            left.remove(q)
            self.pairs.append([p, q])
        # Holo et logic les plus proches de chaque panneau
        holo_xy = {0: self.dev['FHP'], 1: self.dev['RHP'], 2: self.dev['THP']}
        logic_xy = {FLD: self.dev['FLD'], RLD: self.dev['RLD'], PSIF: self.dev['FPSI'], PSIR: self.dev['RPSI']}
        self.near_holo = {p: min(holo_xy, key=lambda h: self._d(holo_xy[h], self.pos[p])) for p in MOVABLE}
        self.near_logic = {p: min(logic_xy, key=lambda d: self._d(logic_xy[d], self.pos[p])) for p in MOVABLE}
        self.look = {p: self._look(self.near_holo[p], holo_xy[self.near_holo[p]], self.pos[p]) for p in MOVABLE}

    @staticmethod
    def _d(a, b):
        return math.hypot(a[0] - b[0], a[1] - b[1])

    def _look(self, h, hxy, pxy):
        """Position de moveHP() pour que le holo h regarde vers le panneau (approximation)."""
        cx, cy = CENTER_XY
        dx, dy = pxy[0] - hxy[0], pxy[1] - hxy[1]
        if h == 2:
            horiz = dx
            vert = -dy                               # vers l'arrière = haut
        else:
            horiz = dx if h == 0 else -dx            # le holo arrière regarde dans l'autre sens
            vert = self._d(hxy, (cx, cy)) - self._d(pxy, (cx, cy))   # vers le sommet = haut
        hz = 1 if horiz > 15 else -1 if horiz < -15 else 0
        vt = 1 if vert > 20 else -1 if vert < -20 else 0
        return {(0, 0): CENTER, (1, 0): UP, (-1, 0): DOWN, (0, -1): LEFT, (1, -1): UP_LEFT, (-1, -1): DOWN_LEFT,
                (0, 1): RIGHT, (1, 1): UP_RIGHT, (-1, 1): DOWN_RIGHT}[(vt, hz)]


class Choreographer:
    def __init__(self, a, title_ms, dome, seed):
        self.a = a
        self.dome = dome
        self.seed = seed
        self.events = []
        self.title_end = title_ms + 300
        self.logic_state = {}      # appareil -> dernière commande envoyée
        self.logic_time = {}
        self.pending_rld = None    # commande de la logic arrière différée après le titre
        self.ppos = {p: 0 for p in MOVABLE}
        self.ptime = {p: -10**9 for p in MOVABLE}
        self.holo_time = {h: -10**9 for h in HOLOS}
        self.holo_pos = {h: CENTER for h in HOLOS}
        self.holo_led = {}
        self.motifs = []           # (temps, motif, niveau, étiquette) pour la prévisualisation

    def rng(self, *key):
        return random.Random('%s/%s' % (self.seed, '/'.join(str(k) for k in key)))

    # ---- émission ---------------------------------------------------------------------

    def logic(self, t, dev, effect, color=0, speed=0, force=False):
        cmd = 'LE%d%02d%d%d00' % (dev, effect, color, min(9, max(0, speed)))
        if not force and self.logic_state.get(dev) == cmd:
            return
        if t - self.logic_time.get(dev, -10**9) < LOGIC_MIN_GAP_MS:
            return
        if dev == RLD and t < self.title_end:
            self.pending_rld = cmd
            self.logic_state[dev] = cmd
            return
        self.logic_state[dev] = cmd
        self.logic_time[dev] = t
        self._cmd(t, cmd)

    def holo_led_cmd(self, t, holo, func, opts=''):
        cmd = 'HP%s0%02d%s' % (HOLO_LETTER[holo], func, opts)
        targets = HOLOS if holo == HOLO_ALL else [holo]
        if all(self.holo_led.get(h) == cmd[3:] for h in targets):
            return
        for h in targets:
            self.holo_led[h] = cmd[3:]
        self._cmd(t, cmd)

    def _cmd(self, t, cmd):
        self.events.append({'t': int(t), 'kind': 'cmd', 'cmd': cmd})

    def panels(self, t, ids, pct, move_ms):
        """Déplace des panneaux. Un panneau commandé il y a moins de PANEL_MIN_GAP_MS ignore
        une ouverture ; une fermeture est seulement retardée, pour qu'aucun ne reste ouvert."""
        t = int(t)
        pct = int(pct)
        move_ms = int(max(PANEL_MIN_MOVE_MS, move_ms))
        groups = {}
        for p in ids:
            if p not in self.ppos or self.ppos[p] == pct:
                continue
            free = self.ptime[p] + PANEL_MIN_GAP_MS
            if t >= free:
                at = t
            elif pct < self.ppos[p]:
                at = free
            else:
                continue
            self.ppos[p] = pct
            self.ptime[p] = at
            groups[at] = groups.get(at, 0) | bit(p)
        for at, mask in groups.items():
            self.events.append({'t': at, 'kind': 'panels', 'mask': mask, 'pct': pct, 'ms': move_ms})

    def pulse(self, t, ids, pct, move_ms, hold_ms):
        """Ouvre puis referme après hold_ms."""
        self.panels(t, ids, pct, move_ms)
        self.panels(t + max(PANEL_MIN_GAP_MS, hold_ms), ids, 0, move_ms)

    def holo(self, t, which, pos, move_ms):
        targets = HOLOS if which == HOLO_ALL else [which]
        ok = [h for h in targets if t - self.holo_time[h] >= HOLO_MIN_GAP_MS and self.holo_pos[h] != pos]
        if not ok:
            return
        for h in ok:
            self.holo_time[h] = t
            self.holo_pos[h] = pos
        w = HOLO_ALL if len(ok) == 3 else ok[0]
        self.events.append({'t': int(t), 'kind': 'holo', 'holo': w, 'pos': pos, 'ms': int(move_ms)})
        for h in ok[1:] if w != HOLO_ALL else []:
            self.events.append({'t': int(t), 'kind': 'holo', 'holo': h, 'pos': pos, 'ms': int(move_ms)})

    def solo(self, t, p, pct, hold_ms, color=None, look=True):
        """Un seul panneau : le holo le plus proche le regarde, la logic voisine change de couleur."""
        self.pulse(t, [p], pct, 150, hold_ms)
        if look:
            self.holo(t, self.dome.near_holo[p], self.dome.look[p], 250)
        if color:
            self.logic(t, self.dome.near_logic[p], SOLID, color)

    # ---- outils -----------------------------------------------------------------------

    def beat_ms(self, i):
        b = self.a['beats']
        if i + 1 < len(b):
            return (b[i + 1] - b[i]) * 1000
        return 60000.0 / self.a['tempo']

    def color_at(self, i):
        """Couleur de la mesure (harmonie dominante), 1–9."""
        c = self.a['beat_color']
        j0 = i - ((i - self.a['downbeat_phase']) % 4)
        bar = [c[j] for j in range(max(0, j0), min(len(c), j0 + 4))] or [c[i]]
        # couleur la plus fréquente de la mesure
        idx = [1 + int(x * 9) % 9 for x in bar]
        return max(set(idx), key=idx.count)

    @staticmethod
    def companion(color):
        return 1 + (color - 1 + 4) % 9

    def speed_for(self, i, unit_ms):
        """Vitesse LogicEngine (0–9) pour qu'un clignotement dure environ un temps."""
        return int(min(9, max(0, round(self.beat_ms(i) / unit_ms) - 1)))

    def pick_other(self, rng, pool, last):
        choices = [p for p in pool if p != last] or pool
        return rng.choice(choices)

    # ---- motifs de panneaux (un appel par battement) ----------------------------------
    # b : dictionnaire du battement (t, bm, i, j = rang dans la section, pos dans la mesure,
    #     down, kick, snare, high, low, bal, col, comp, tier) ; m : état du motif de la section

    def m_breathe(self, b, m):
        # Le dôme respire : tout s'entrouvre sur deux temps, se referme sur les deux suivants
        if b['j'] % 2 == 0:
            amp = int(15 + 20 * min(1.0, b['level'] * 2))
            self.panels(b['t'], MOVABLE, amp if b['j'] % 4 == 0 else 0, b['bm'] * 1.8)

    def m_sparkle(self, b, m):
        # Brèves ouvertures d'un panneau au hasard sur les aigus
        if b['high'] > 0.45 or b['down']:
            p = self.pick_other(m['rng'], MOVABLE, m.get('last'))
            m['last'] = p
            self.solo(b['t'] + (b['bm'] / 2 if b['j'] % 2 else 0), p, 35, 300, b['comp'])

    def m_morse(self, b, m):
        # Un panneau tape le rythme : coup bref, ou long si la note suivante tarde
        p = m.setdefault('p', m['rng'].choice(MOVABLE))
        hits = [x for x in b['hits'] if x[1] > 0.45]
        for k, (x, s) in enumerate(hits):
            nxt = hits[k + 1][0] if k + 1 < len(hits) else b['t'] + b['bm']
            hold = 260 if nxt - x < b['bm'] * 0.75 else min(b['bm'] * 0.9, nxt - x - 50)
            self.pulse(x, [p], 70, 150, hold)
        if b['j'] % 8 == 0:
            self.holo(b['t'], self.dome.near_holo[p], self.dome.look[p], 300)
            self.logic(b['t'], self.dome.near_logic[p], SOLID, b['comp'])

    def m_solo(self, b, m):
        # Un seul panneau à la fois sur les accents, jamais deux fois le même d'affilée
        accent = b['snare'] > 0.5 or b['kick'] > 0.6 if b['tier'] >= 1 else (b['down'] or b['high'] > 0.55)
        if accent:
            p = self.pick_other(m['rng'], MOVABLE, m.get('last'))
            m['last'] = p
            self.solo(b['t'], p, 100 if b['tier'] >= 1 else 70, b['bm'] * 0.75, b['comp'])

    def m_knight(self, b, m):
        # Un panneau voyage d'un bout à l'autre de l'anneau extérieur, puis revient
        line = self.dome.line
        n = len(line)
        k = b['j'] % (2 * n - 2)
        idx = k if k < n else 2 * n - 2 - k
        p = line[idx]
        self.panels(b['t'], [q for q in line if q != p], 0, 200)
        self.panels(b['t'], [p], 100, 180)
        if b['j'] % 4 == 0:
            self.holo(b['t'], self.dome.near_holo[p], self.dome.look[p], 300)

    def m_wave(self, b, m):
        # Vague autour du dôme : chaque temps (demi-temps à l'apogée) ouvre le suivant
        ring = self.dome.ring if m['dir'] > 0 else self.dome.ring[::-1]
        steps = [b['t'], b['t'] + b['bm'] / 2] if b['tier'] == 3 and b['bm'] >= 500 else [b['t']]
        for t in steps:
            k = m.get('k', 0)
            self.panels(t, [ring[(k - 2) % len(ring)]], 0, 180)
            self.panels(t, [ring[k % len(ring)]], 100 if b['tier'] >= 2 else 80, 180)
            m['k'] = k + 1

    def m_stereo(self, b, m):
        # Les panneaux suivent la stéréo : côté droit du public quand le son est à droite
        bal = b['bal']
        if bal > 0.08:
            on, off = self.dome.right, self.dome.left
        elif bal < -0.08:
            on, off = self.dome.left, self.dome.right
        else:
            on, off = [], MOVABLE
        pct = int(min(100, 40 + 300 * abs(bal))) // 10 * 10
        self.panels(b['t'], off, 0, 200)
        if on and (b['kick'] > 0.3 or b['down']):
            self.panels(b['t'], on, pct, 200)
        elif not on and b['kick'] > 0.6:
            self.pulse(b['t'], PIES, 60, 150, b['bm'] * 0.5)

    def m_heartbeat(self, b, m):
        # Boum-boum : deux panneaux voisins battent coup sur coup sur la grosse caisse
        pair = m.setdefault('pair', m['rng'].choice([p for p in self.dome.pairs if len(p) == 2]))
        if (b['kick'] > 0.45 or b['down']) and b['bm'] >= 420:
            self.pulse(b['t'], [pair[0]], 60, 150, 250)
            self.pulse(b['t'] + 170, [pair[1]], 60, 150, 250)

    def m_callresp(self, b, m):
        # Question / réponse : un groupe « parle » une mesure, l'autre répond la suivante
        a, c = m.setdefault('groups', m['rng'].choice([(self.dome.outer, PIES), (self.dome.left, self.dome.right)]))
        bar = (b['i'] - self.a['downbeat_phase']) // 4
        grp = a if bar % 2 == 0 else c
        if b['down']:
            self.panels(b['t'], a + c, 0, 200)
            self.panels(b['t'] + PANEL_MIN_GAP_MS, grp, 80, 200)
        elif b['pos'] == 2:
            self.panels(b['t'], grp, 0, 250)

    def m_wheel(self, b, m):
        # Roue des pie : un seul ouvert à la fois, qui tourne ; plus vite à l'apogée
        ring = self.dome.pie_ring if m['dir'] > 0 else self.dome.pie_ring[::-1]
        steps = [b['t'], b['t'] + b['bm'] / 2] if b['tier'] == 3 and b['bm'] >= 500 else [b['t']]
        for t in steps:
            k = m.get('k', 0)
            self.panels(t, [ring[(k - 1) % 4]], 0, 150)
            self.panels(t, [ring[k % 4]], 100, 150)
            m['k'] = k + 1
        if b['down'] and b['kick'] > 0.6:
            self.pulse(b['t'], MINIS, 100, 150, 300)

    def m_mirror(self, b, m):
        # Miroir : les paires symétriques s'ouvrent ensemble, l'une après l'autre
        pairs = self.dome.pairs
        k = m.get('k', 0)
        self.panels(b['t'], pairs[(k - 1) % len(pairs)], 0, 180)
        self.panels(b['t'], pairs[k % len(pairs)], 100, 180)
        m['k'] = k + 1
        if b['tier'] == 3 and b['kick'] > 0.7 and not any(p in PIES for p in pairs[k % len(pairs)]):
            self.pulse(b['t'], PIES, 100, 150, b['bm'] * 0.5)

    def m_dominos(self, b, m):
        # Dominos : ouverture en cascade sur un temps, fermeture dans le même ordre au suivant ;
        # la mesure suivante dans l'autre sens
        seq = m.setdefault('seq', m['rng'].choice([self.dome.line[1:6], self.dome.pie_ring, self.dome.line[:5]]))
        bar = (b['i'] - self.a['downbeat_phase']) // 4
        if bar % 2:
            seq = seq[::-1]
        if b['pos'] in (0, 2):
            opening = True
        elif b['pos'] in (1, 3):
            opening = False
        d = b['bm'] / len(seq)
        for k, p in enumerate(seq):
            self.panels(b['t'] + k * d, [p], 100 if opening else 0, max(150, d * 1.5))

    def m_accordion(self, b, m):
        # Accordéon : rangées de l'avant vers l'arrière sur deux temps, retour sur les deux suivants
        rows = self.dome.rows
        span = b['bm'] * 2 / len(rows)
        if b['pos'] == 0:
            for k, r in enumerate(rows):
                self.panels(b['t'] + k * span, r, 80, max(150, span))
        elif b['pos'] == 2:
            for k, r in enumerate(rows[::-1]):
                self.panels(b['t'] + k * span, r, 0, max(150, span))

    def m_groups(self, b, m):
        # Groupes par type : pie sur la grosse caisse, petits et moyens sur la caisse claire,
        # clin d'œil des mini-panneaux toutes les deux mesures
        t, bm = b['t'], b['bm']
        if b['kick'] > 0.55:
            self.pulse(t, PIES, 100, min(300, bm * 0.4), bm / 2)
        if b['pos'] in (1, 3) and b['snare'] > 0.5:
            self.pulse(t, SMALLS + MEDIUMS, 80, min(300, bm * 0.4), bm / 2)
        if b['down'] and ((b['i'] - self.a['downbeat_phase']) // 4) % 2 == 0:
            self.pulse(t, MINIS, 100, 200, bm)

    def m_equalizer(self, b, m):
        # Égaliseur : le nombre de types de panneaux ouverts suit les graves, et retombe entre
        # deux temps comme l'aiguille d'un vumètre
        lvl = max(1, min(len(EQ_ORDER), int(round(max(b['low'], b['kick']) * len(EQ_ORDER)))))
        if b['bm'] >= 2 * PANEL_MIN_GAP_MS:
            steps = [(b['t'], lvl), (b['t'] + b['bm'] / 2, 0)]
        else:
            # Tempo rapide : la retombée se fait un temps sur deux
            steps = [(b['t'], lvl if b['j'] % 2 == 0 else 0)]
        for t, L in steps:
            opened = [p for grp in EQ_ORDER[:L] for p in grp]
            self.panels(t, opened, 100, 150)
            self.panels(t, [p for p in MOVABLE if p not in opened], 0, 150)

    def tremble(self, t, bm, p):
        """Roulement de batterie : un panneau vibre entre 20 et 45 %."""
        x = t
        k = 0
        while x < t + bm - PANEL_MIN_GAP_MS:
            self.panels(x, [p], 45 if k % 2 == 0 else 20, 150)
            x += PANEL_MIN_GAP_MS
            k += 1
        self.panels(t + bm, [p], 0, 150)

    # ---- chorégraphie -----------------------------------------------------------------

    def build(self):
        a = self.a
        beats = a['beats']
        n = len(beats)
        phase = a['downbeat_phase']
        secs = a['sections']
        bright = sum(a['beat_high']) / max(1, n) > 0.42

        # Niveau d'intensité de chaque section
        levels = [s['level'] for s in secs]
        top = max(levels) if levels else 0
        for s in secs:
            L = s['level']
            s['tier'] = 0 if L < 0.2 else 1 if L < 0.4 else 2 if L < 0.58 else 3
            if L >= 0.45 and top - L < 0.06:
                s['tier'] = 3
        # L'apogée doit rester un moment fort : au plus ~35 % de la chanson, sinon les sections
        # d'apogée les moins intenses redescendent au niveau 2
        def share3():
            return sum(s['end_beat'] - s['start_beat'] for s in secs if s['tier'] == 3) / max(1, n)
        for s in sorted([s for s in secs if s['tier'] == 3], key=lambda s: s['level']):
            if share3() <= 0.35 or sum(1 for x in secs if x['tier'] == 3) <= 1:
                break
            s['tier'] = 2
        tier_of = [0] * n
        sec_of = [0] * n
        for k, s in enumerate(secs):
            for i in range(s['start_beat'], s['end_beat']):
                if i < n:
                    tier_of[i] = s['tier']
                    sec_of[i] = k

        # Motif de chaque section : les sections de même étiquette (refrains) reprennent le même
        # motif et le même tirage, jamais deux fois de suite le même motif
        by_label = {}
        prev = None
        for k, s in enumerate(secs):
            pool = [x for x in MOTIFS[s['tier']] if x != 'stereo' or a['is_stereo']]
            key = (s.get('label', k), s['tier'])
            if key in by_label and by_label[key] != prev:
                s['motif'] = by_label[key]
            else:
                s['motif'] = self.pick_other(self.rng('motif', k), pool, prev)
                by_label.setdefault(key, s['motif'])
            prev = s['motif']

        # Montées : les 4 mesures avant une section plus intense d'au moins 2 niveaux, ou avant l'apogée
        buildup = [None] * n
        for k in range(1, len(secs)):
            prev_s, cur = secs[k - 1], secs[k]
            if cur['tier'] == 3 and prev_s['tier'] <= 2 or cur['tier'] - prev_s['tier'] >= 2:
                b0 = cur['start_beat']
                for i in range(max(prev_s['start_beat'], b0 - 16), b0):
                    buildup[i] = b0

        t_start = a['sound_start'] * 1000
        t_end = a['sound_end'] * 1000
        kicks = a['kicks']
        snares = a['snares']

        # Coup de théâtre : le battement le plus fort de la chanson (hors montée et hors fin)
        cands = [i for i in range(n) if buildup[i] is None and beats[i] * 1000 < t_end - 6000
                 and beats[i] * 1000 > self.title_end and not (secs and secs[sec_of[i]]['start_beat'] == i)]
        peak = max(cands, key=lambda i: a['beat_level'][i] + a['beat_low'][i]) if cands else None

        # Intro : couleur de l'harmonie, holos au centre
        c0 = self.color_at(0)
        self.logic(0, FLD, SOLID, c0)
        self.logic(0, RLD, SOLID, c0)
        self.logic(0, PSIF, SOLID, self.companion(c0))
        self.logic(0, PSIR, SOLID, self.companion(c0))
        self.holo_led_cmd(0, HOLO_ALL, 3, '%d3' % HOLO_COLOR[c0])

        fire_next = not bright
        ki = si = 0
        rot = 0
        motif = None
        skip_until = -1

        for i, tb_s in enumerate(beats):
            t = tb_s * 1000
            if t > t_end - 2500:
                break
            bm = self.beat_ms(i)
            tier = tier_of[i]
            k = sec_of[i]
            sec = secs[k] if secs else {'start_beat': 0, 'motif': 'solo', 'tier': 0}
            pos = (i - phase) % 4
            down = pos == 0
            col = self.color_at(i)
            comp = self.companion(col)
            sec_start = secs and sec['start_beat'] == i
            prev_tier = secs[k - 1]['tier'] if sec_start and k > 0 else None
            low = a['beat_low'][i]
            bal = a['beat_balance'][i]

            # Titre fini : la logic arrière reprend l'état courant
            if self.pending_rld and t >= self.title_end:
                self._cmd(self.title_end, self.pending_rld)
                self.logic_time[RLD] = self.title_end
                self.pending_rld = None

            # Attaques proches de ce battement
            while ki < len(kicks) and kicks[ki][0] * 1000 < t - bm / 2:
                ki += 1
            while si < len(snares) and snares[si][0] * 1000 < t - bm / 2:
                si += 1
            kick = max([s for (x, s) in kicks[ki:ki + 4] if abs(x * 1000 - t) < bm / 3] or [0])
            snare = max([s for (x, s) in snares[si:si + 4] if abs(x * 1000 - t) < bm / 3] or [0])
            hits = sorted((x * 1000, s) for (x, s) in kicks[ki:ki + 8] + snares[si:si + 8]
                          if t <= x * 1000 < t + bm)
            roll = sum(1 for (x, s) in snares[si:si + 12] if t <= x * 1000 < t + bm and s > 0.3) >= 3

            # ---- montée : les panneaux s'ouvrent un par un jusqu'à mi-course, puis se figent
            # (suspense) ; les logics accélèrent, noir complet sur le dernier temps
            if buildup[i] is not None:
                b0 = buildup[i]
                beats_left = b0 - i
                if i == 0 or buildup[i - 1] != b0:
                    motif = None
                    order = self.rng('fill', b0).choice([self.dome.ring, self.dome.ring[::-1],
                                                        [p for r in self.dome.rows for p in r]])
                    fill_beats = max(1, beats_left - 4)
                    for q, p in enumerate(order):
                        self.panels(t + q * fill_beats * bm / len(order), [p], 50, 400)
                    self.panels(t, [p for p in MOVABLE if p not in order], 0, 300)
                    self.motifs.append({'t': int(t), 'motif': 'montée et suspense', 'tier': tier})
                if down and beats_left > 1:
                    bars_left = max(1, (beats_left + 3) // 4)
                    self.logic(t, FLD, FLASH, col, bars_left - 1)
                    self.logic(t, RLD, FLASH, col, bars_left - 1)
                    self.holo(t, HOLO_ALL, UP if bars_left <= 2 else CENTER, bm)
                if beats_left == 1:
                    self.logic(t, FLD, LIGHTSOUT, force=True)
                    self.logic(t, RLD, LIGHTSOUT, force=True)
                    self.logic(t, PSIF, LIGHTSOUT)
                    self.logic(t, PSIR, LIGHTSOUT)
                    self.holo_led_cmd(t, HOLO_ALL, 96)
                continue

            # ---- coup de théâtre : tout s'ouvre, holos blancs, puis un temps de noir complet
            if i == peak:
                self.panels(t, MOVABLE, 100, 150)
                self.holo_led_cmd(t, HOLO_ALL, 5, str(HOLO_WHITE))
                self.logic(t, FLD, FLASH, col, 0, force=True)
                self.logic(t, RLD, FLASH, comp, 0, force=True)
                self.holo(t, HOLO_ALL, UP, 150)
                self.panels(t + bm, MOVABLE, 0, 200)
                for d in (FLD, RLD, PSIF, PSIR):
                    self.logic(t + bm, d, LIGHTSOUT, force=True)
                self.holo_led_cmd(t + bm, HOLO_ALL, 96)
                self.motifs.append({'t': int(t), 'motif': 'coup de théâtre', 'tier': tier})
                skip_until = i + 1
                continue
            if i <= skip_until:
                continue

            # ---- début de section --------------------------------------------------
            if sec_start or motif is None:
                motif = {'name': sec['motif'], 'rng': self.rng('m', sec.get('label', k), sec['tier']),
                         'dir': 1, 'start': sec['start_beat']}
                motif['dir'] = motif['rng'].choice([1, -1])
                self.motifs.append({'t': int(t), 'motif': MOTIF_NAMES[motif['name']], 'tier': tier,
                                    'label': sec.get('label')})
            if sec_start:
                if tier == 3:
                    # Explosion : tout s'ouvre, feu ou arc-en-ciel. Une apogée qui en suit une
                    # autre change seulement d'effet.
                    eff = FIRE if fire_next else RAINBOW
                    fire_next = not fire_next
                    if prev_tier != 3:
                        # Tout s'ouvre un temps, puis le motif de l'apogée prend le relais
                        self.panels(t, MOVABLE, 100, 200)
                        self.panels(t + bm, MOVABLE, 0, 200)
                    else:
                        # Nouveau motif d'apogée : on repart des panneaux fermés
                        self.panels(t, MOVABLE, 0, 200)
                    self.logic(t, FLD, eff, 0, 0 if eff == FIRE else 1)
                    self.logic(t, RLD, eff, 0, 0 if eff == FIRE else 1)
                    self.logic(t, PSIF, RAINBOW, 0, 0)
                    self.logic(t, PSIR, RAINBOW, 0, 0)
                    self.holo_led_cmd(t, HOLO_ALL, 6)
                    self.holo(t, HOLO_ALL, UP, 200)
                    continue
                if prev_tier is not None and prev_tier >= 2 and tier == 0:
                    # Coupure : noir, panneaux fermés, holos au centre
                    self.panels(t, MOVABLE, 0, 300)
                    self.logic(t, FLD, LIGHTSOUT, force=True)
                    self.logic(t, RLD, LIGHTSOUT, force=True)
                    self.holo(t, HOLO_ALL, CENTER, 400)
                    self.holo_led_cmd(t, HOLO_ALL, 96)
                    continue
                if tier == 2 and i > 0 and buildup[i - 1] is not None:
                    # Fin de montée : tout claque grand ouvert un temps, puis se referme
                    self.panels(t, MOVABLE, 100, 150)
                    self.panels(t + bm, MOVABLE, 0, 250)
                    self.logic(t, FLD, SOLID, col)
                    self.logic(t, RLD, SOLID, comp)
                    self.holo_led_cmd(t, HOLO_ALL, 5, str(HOLO_COLOR[col]))
                    continue
                self.panels(t, MOVABLE, 0, 250)

            # ---- lumières et holos selon le niveau ------------------------------------
            if tier == 0:
                if down:
                    eff = NORMAL if a['beat_level'][i] < 0.08 else SOLID
                    self.logic(t, FLD, eff, col if eff else 0)
                    self.logic(t, RLD, eff, col if eff else 0)
                if down and ((i - phase) // 4) % 2 == 0:
                    self.logic(t, PSIF, SOLID, comp)
                    self.logic(t, PSIR, SOLID, comp)
                    self.holo_led_cmd(t, HOLO_ALL, 3, '%d3' % HOLO_COLOR[col])
                    # R2 regarde d'où vient le son, sinon balayage lent
                    h = HOLOS[rot % 3]
                    rot += 1
                    if a['is_stereo'] and abs(bal) > 0.12:
                        p = RIGHT if bal > 0 else LEFT
                    else:
                        p = [LEFT, CENTER, RIGHT, UP][(i // 8) % 4]
                    self.holo(t, h, p, 800)
            elif tier == 1:
                if down:
                    self.logic(t, FLD, FLIPFLOP, col, self.speed_for(i, 200))
                    self.logic(t, RLD, FLIPFLOP, comp, self.speed_for(i, 200))
                    self.logic(t, PSIF, FLASH, comp, self.speed_for(i, 250))
                    self.logic(t, PSIR, FLASH, col, self.speed_for(i, 250))
                    self.holo_led_cmd(t, HOLO_ALL, 5, str(HOLO_COLOR[col]))
                    h = HOLOS[rot % 3]
                    rot += 1
                    if a['is_stereo'] and abs(bal) > 0.12:
                        p = RIGHT if bal > 0 else LEFT
                    else:
                        p = LEFT if rot % 2 else RIGHT
                    self.holo(t, h, p, 350)
            else:
                a_col, b_col = (col, comp) if i % 2 == 0 else (comp, col)
                if tier == 2:
                    # Couleurs échangées sur chaque temps entre l'avant et l'arrière
                    self.logic(t, FLD, SOLID, a_col)
                    self.logic(t, RLD, SOLID, b_col)
                    self.logic(t, PSIF, SOLID, b_col)
                    self.logic(t, PSIR, SOLID, a_col)
                    self.holo_led_cmd(t, HOLO_ALL, 5, str(HOLO_COLOR[a_col]))
                # R2 hoche la tête en rythme : chaque temps à l'apogée, tous les deux temps sinon
                if tier == 3 or pos % 2 == 0:
                    step = i if tier == 3 else i // 2
                    self.holo(t, HOLO_ALL, UP if step % 2 == 0 else DOWN, min(300, bm * 0.6))

            # ---- panneaux : motif de la section, vibration sur les roulements -----------
            b = {'t': t, 'bm': bm, 'i': i, 'j': i - motif['start'], 'pos': pos, 'down': down,
                 'kick': kick, 'snare': snare, 'high': a['beat_high'][i], 'low': low, 'bal': bal,
                 'level': a['beat_level'][i], 'col': col, 'comp': comp, 'tier': tier, 'hits': hits}
            getattr(self, 'm_' + motif['name'])(b, motif)
            lead = motif.setdefault('lead', motif['rng'].choice(MOVABLE))
            if roll and tier >= 1 and self.ppos[lead] == 0 and t - self.ptime[lead] >= PANEL_MIN_GAP_MS:
                self.tremble(t, bm, lead)

        # Fin : tout se ferme, puis un salut propre à la chanson
        te = max(t_start, t_end - 2600)
        self.panels(te, MOVABLE, 0, 350)
        self.holo(te, HOLO_ALL, CENTER, 500)
        self.logic(te, FLD, SOLID, self.color_at(n - 1), force=True)
        self.logic(te, RLD, SOLID, self.color_at(n - 1), force=True)
        self.holo_led_cmd(te, HOLO_ALL, 3, '%d3' % HOLO_COLOR[self.color_at(n - 1)])
        bow = self.rng('bow').choice(['pie', 'wave', 'wheel', 'mini'])
        tb = te + 500
        if bow == 'pie':
            self.panels(tb, PIES, 50, 400)
            self.panels(t_end - 400, PIES, 0, 300)
        elif bow == 'wave':
            ring = self.dome.ring
            self.panels(tb, MOVABLE, 70, 300)
            span = (t_end - 400 - (tb + 400)) / len(ring)
            for q, p in enumerate(ring):
                self.panels(tb + 400 + q * span, [p], 0, 200)
        elif bow == 'wheel':
            # La roue des pie ralentit et s'arrête
            x, step, q = tb, 250, 0
            while x + step < t_end - 400:
                self.panels(x, [self.dome.pie_ring[(q - 1) % 4]], 0, 150)
                self.panels(x, [self.dome.pie_ring[q % 4]], 100, 150)
                x += step
                step = int(step * 1.35)
                q += 1
            self.panels(x, PIES, 0, 200)
        else:
            self.pulse(tb, MINIS, 100, 150, 300)
            self.pulse(tb + 800, MINIS, 100, 150, 300)
        self.motifs.append({'t': int(te), 'motif': 'salut final (%s)' % bow, 'tier': 0})

        self.events.sort(key=lambda e: e['t'])
        return self.merge()

    def merge(self):
        """Regroupe les commandes simultanées en un seul événement (moins de mémoire flash)."""
        out = []
        for e in self.events:
            last = out[-1] if out else None
            if last and e['kind'] == 'cmd' and last['kind'] == 'cmd' and last['t'] == e['t']:
                last['cmd'] += '\n' + e['cmd']
            elif (last and e['kind'] == 'panels' and last['kind'] == 'panels' and last['t'] == e['t']
                  and last['pct'] == e['pct'] and last['ms'] == e['ms']):
                last['mask'] |= e['mask']
            else:
                out.append(dict(e))
        return out


def choreograph(analysis, title, font_h, layout_path=None):
    adv = font_advances(font_h)
    title_ms = title_scroll_ms(title, adv)
    seed = hashlib.md5(analysis['file'].encode('utf-8')).hexdigest()[:8]
    ch = Choreographer(analysis, title_ms, Dome(layout_path), seed)
    events = ch.build()
    duration_ms = int(analysis['sound_end'] * 1000 + 800)
    return events, duration_ms, title_ms, [s['tier'] for s in analysis['sections']], ch.motifs
