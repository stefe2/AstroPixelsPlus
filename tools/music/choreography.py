"""Règles de chorégraphie : transforme l'analyse d'une chanson en événements pour le droïde.

Chaque section de la chanson reçoit un niveau d'intensité (0 calme, 1 groove, 2 énergie,
3 apogée). Les règles de chaque niveau sont décrites dans docs/music.md.
Les temps des événements sont en millisecondes depuis le début du fichier audio.
"""

import os
import re

# Types de panneaux (masques de servoSettings[] dans src/main.cpp)
SMALL, MEDIUM, BIG, PIE, TOP, MINI = 0x01, 0x02, 0x04, 0x08, 0x10, 0x20
ALL = SMALL | MEDIUM | BIG | PIE | TOP | MINI
PANEL_NAMES = {SMALL: 'SMALL_PANEL', MEDIUM: 'MEDIUM_PANEL', BIG: 'BIG_PANEL',
               PIE: 'PIE_PANEL', TOP: 'TOP_PIE_PANEL', MINI: 'MINI_PANEL'}
# Ordre de l'« égaliseur » : du plus discret au plus spectaculaire
EQ_ORDER = [MINI, SMALL, MEDIUM, PIE, BIG, TOP]

# Appareils LogicEngine (LE<id>…)
FLD, RLD, PSIF, PSIR = 1, 3, 4, 5

# Effets LogicEngine
NORMAL, SOLID, FLASH, FLIPFLOP, RAINBOW, LIGHTSOUT, FIRE = 0, 5, 6, 7, 10, 14, 22

# Couleurs LogicEngine 1–9 dans l'ordre du cercle chromatique, et code couleur holo équivalent
LOGIC_COLORS = ['rouge', 'orange', 'jaune', 'vert', 'cyan', 'bleu', 'violet', 'magenta', 'rose']
HOLO_COLOR = {1: 1, 2: 7, 3: 2, 4: 3, 5: 4, 6: 5, 7: 8, 8: 6, 9: 6}

# Positions de moveHP()
DOWN, CENTER, UP, LEFT, RIGHT = 0, 1, 2, 3, 6
HOLOS = [0, 1, 2]          # avant, arrière, dessus (MUSIC_HOLO_*)
HOLO_ALL = 3
HOLO_LETTER = {0: 'F', 1: 'R', 2: 'T', 3: 'A'}

# Logic arrière : 27 pixels de large, défilement d'un pixel toutes les 50 ms, 2 s d'écran noir avant
RLD_WIDTH = 27
SCROLL_MS_PER_PX = 50
TEXT_BLANK_MS = 2000

# Limites mécaniques
PANEL_MIN_GAP_MS = 250     # entre deux commandes d'un même type de panneau
PANEL_MIN_MOVE_MS = 150
HOLO_MIN_GAP_MS = 300
LOGIC_MIN_GAP_MS = 110


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


class Choreographer:
    def __init__(self, a, title_ms):
        self.a = a
        self.events = []
        self.title_end = title_ms + 300
        self.logic_state = {}      # appareil -> dernière commande envoyée
        self.logic_time = {}
        self.pending_rld = None    # commande de la logic arrière différée après le titre
        self.panel_pos = {m: 0 for m in PANEL_NAMES}
        self.panel_time = {m: -10**9 for m in PANEL_NAMES}
        self.holo_time = {h: -10**9 for h in HOLOS}
        self.holo_pos = {h: CENTER for h in HOLOS}
        self.holo_led = {}

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

    def panels(self, t, mask, pct, move_ms):
        move_ms = int(max(PANEL_MIN_MOVE_MS, move_ms))
        sel = 0
        for m in PANEL_NAMES:
            if mask & m and self.panel_pos[m] != pct and t - self.panel_time[m] >= PANEL_MIN_GAP_MS:
                sel |= m
        if not sel:
            return
        for m in PANEL_NAMES:
            if sel & m:
                self.panel_pos[m] = pct
                self.panel_time[m] = t
        self.events.append({'t': int(t), 'kind': 'panels', 'mask': sel, 'pct': int(pct), 'ms': move_ms})

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

        # Montées : les 4 mesures avant une section plus intense d'au moins 2 niveaux, ou avant l'apogée
        buildup = [None] * n
        for k in range(1, len(secs)):
            prev, cur = secs[k - 1], secs[k]
            if cur['tier'] == 3 and prev['tier'] <= 2 or cur['tier'] - prev['tier'] >= 2:
                b0 = cur['start_beat']
                for i in range(max(prev['start_beat'], b0 - 16), b0):
                    buildup[i] = b0

        t_start = a['sound_start'] * 1000
        t_end = a['sound_end'] * 1000

        # Intro : couleur de l'harmonie, holos au centre
        c0 = self.color_at(0)
        self.logic(0, FLD, SOLID, c0)
        self.logic(0, RLD, SOLID, c0)
        self.logic(0, PSIF, SOLID, self.companion(c0))
        self.logic(0, PSIR, SOLID, self.companion(c0))
        self.holo_led_cmd(0, HOLO_ALL, 3, '%d3' % HOLO_COLOR[c0])

        fire_next = not bright
        kicks = a['kicks']
        snares = a['snares']
        ki = si = 0
        rot = 0
        last_eq = None

        for i, tb_s in enumerate(beats):
            t = tb_s * 1000
            if t > t_end - 2500:
                break
            bm = self.beat_ms(i)
            tier = tier_of[i]
            k = sec_of[i]
            pos = (i - phase) % 4
            down = pos == 0
            col = self.color_at(i)
            comp = self.companion(col)
            sec_start = secs and secs[k]['start_beat'] == i
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

            # ---- montée : les panneaux s'entrouvrent, les logics accélèrent -------------
            if buildup[i] is not None:
                beats_left = buildup[i] - i
                if down and beats_left > 1:
                    bars_left = max(1, (beats_left + 3) // 4)
                    self.panels(t, ALL, int(10 * (5 - min(4, bars_left))), bm * 3.5)
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

            # ---- début de section --------------------------------------------------
            if sec_start:
                if tier == 3:
                    # Explosion : tout s'ouvre, feu ou arc-en-ciel. Une apogée qui en suit une
                    # autre change seulement d'effet.
                    eff = FIRE if fire_next else RAINBOW
                    fire_next = not fire_next
                    if prev_tier != 3:
                        self.panels(t, ALL, 100, 200)
                    self.logic(t, FLD, eff, 0, 0 if eff == FIRE else 1)
                    self.logic(t, RLD, eff, 0, 0 if eff == FIRE else 1)
                    self.logic(t, PSIF, RAINBOW, 0, 0)
                    self.logic(t, PSIR, RAINBOW, 0, 0)
                    self.holo_led_cmd(t, HOLO_ALL, 6)
                    self.holo(t, HOLO_ALL, UP, 200)
                    last_eq = None
                    continue
                if prev_tier is not None and prev_tier >= 2 and tier == 0:
                    # Coupure : noir, panneaux fermés, holos au centre
                    self.panels(t, ALL, 0, 300)
                    self.logic(t, FLD, LIGHTSOUT, force=True)
                    self.logic(t, RLD, LIGHTSOUT, force=True)
                    self.holo(t, HOLO_ALL, CENTER, 400)
                    self.holo_led_cmd(t, HOLO_ALL, 96)
                    continue
                if tier <= 1:
                    self.panels(t, ALL, 0, 300)
                elif tier == 2:
                    if i > 0 and buildup[i - 1] is not None:
                        # Fin de montée : tout s'ouvre en grand un temps, puis se referme
                        self.panels(t, ALL, 100, 200)
                        self.panels(t + bm, ALL, 0, 250)
                        self.logic(t, FLD, SOLID, col)
                        self.logic(t, RLD, SOLID, comp)
                        self.holo_led_cmd(t, HOLO_ALL, 5, str(HOLO_COLOR[col]))
                        continue
                    self.panels(t, ALL, 0, 250)

            # ---- niveau 0 : calme ----------------------------------------------------
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

            # ---- niveau 1 : groove ---------------------------------------------------
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
                    # Le dôme « respire » : un type de panneau s'entrouvre sur les temps forts
                    if low > 0.45 or kick > 0.6:
                        m = [SMALL, MEDIUM, PIE, MINI][(i // 4) % 4]
                        self.panels(t, m, 60, min(400, bm * 0.5))
                if pos == 1:
                    self.panels(t, ALL, 0, min(400, bm * 0.5))

            # ---- niveaux 2 et 3 : énergie et apogée ----------------------------------
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

                if tier == 3:
                    # Égaliseur : le nombre de types de panneaux ouverts suit les graves
                    lvl = max(1, min(6, int(round(max(low, kick) * 6))))
                    if lvl != last_eq:
                        opened = 0
                        for m in EQ_ORDER[:lvl]:
                            opened |= m
                        self.panels(t, opened, 100, 150)
                        self.panels(t, ALL & ~opened, 0, 150)
                        last_eq = lvl
                else:
                    half = t + bm / 2
                    if kick > 0.55:
                        self.panels(t, PIE, 100, min(300, bm * 0.4))
                        self.panels(half, PIE, 0, min(300, bm * 0.4))
                    if pos in (1, 3) and snare > 0.5:
                        self.panels(t, SMALL | MEDIUM, 80, min(300, bm * 0.4))
                        self.panels(half, SMALL | MEDIUM, 0, min(300, bm * 0.4))
                    if down and ((i - phase) // 4) % 2 == 0:
                        self.panels(t, TOP, 100, 200)
                        self.panels(t + bm, TOP, 0, 200)

        # Fin : tout se ferme, salut du panneau du dessus
        te = max(t_start, t_end - 2200)
        self.panels(te, ALL, 0, 350)
        self.holo(te, HOLO_ALL, CENTER, 500)
        self.logic(te, FLD, SOLID, self.color_at(n - 1), force=True)
        self.logic(te, RLD, SOLID, self.color_at(n - 1), force=True)
        self.holo_led_cmd(te, HOLO_ALL, 3, '%d3' % HOLO_COLOR[self.color_at(n - 1)])
        self.panel_time = {m: -10**9 for m in PANEL_NAMES}
        self.panels(t_end - 1300, TOP, 100, 300)
        self.panels(t_end - 400, TOP, 0, 300)

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


def choreograph(analysis, title, font_h):
    adv = font_advances(font_h)
    title_ms = title_scroll_ms(title, adv)
    ch = Choreographer(analysis, title_ms)
    events = ch.build()
    duration_ms = int(analysis['sound_end'] * 1000 + 800)
    return events, duration_ms, title_ms, [s['tier'] for s in analysis['sections']]
