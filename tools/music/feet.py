"""Piste des pieds pour R2-Bling : jambes (2 × 2 hoses de 25 LEDs) et face (33 LEDs).

R2-Bling reçoit le même :MUnn qu'AstroPixels (fil Kyber → dôme dédoublé dans le corps) et déroule
cette piste de son côté. Elle suit le plan calculé pour le dôme (choreography.py) : mêmes sections,
mêmes couleurs, mêmes montées, même coup de théâtre, mêmes refrains.

La piste contient peu d'événements :
- 'beat'  : un par battement, avec les graves, les aigus, la stéréo et les attaques ; R2-Bling
            cale ses effets dessus (il ne calcule pas le tempo lui-même) ;
- 'hit'   : attaques entre les battements, seulement pendant l'effet morse ;
- 'legs'  : effet des jambes (nom, couleur, deuxième couleur, paramètre) ;
- 'front' : effet de la face.
Les effets eux-mêmes sont dessinés par R2-Bling ; la prévisualisation les imite (preview_template.html).
"""

import random

# Effets des jambes (niveau 0 = battery box, 24 = pied) et de la face (0 = gauche du public)
LEG_FX = {
    'breathe': 'respiration', 'sparkle': 'scintillement', 'morse': 'morse', 'wave': 'onde',
    'bounce': 'rebond', 'vu': 'vumètre', 'strobe': 'stroboscope', 'pingpong': 'ping-pong',
    'cross': 'hoses croisés', 'fill': 'montée', 'freeze': 'suspense', 'black': 'noir',
    'bolt': 'éclair', 'fire': 'feu', 'disco': 'disco', 'white': 'flash blanc', 'bow': 'salut',
    'idle': 'repos',
}
FRONT_FX = {
    'vu': 'vumètre centré', 'sweep': 'balayage', 'balance': 'balance', 'heart': 'battement',
    'breathe': 'respiration', 'sparkle': 'scintillement', 'fill': 'montée', 'freeze': 'suspense',
    'black': 'noir', 'fire': 'feu', 'disco': 'disco', 'white': 'flash blanc', 'bow': 'salut',
    'idle': 'repos',
}
LEG_POOL = {
    0: ['breathe', 'sparkle', 'morse'],
    1: ['wave', 'bounce', 'morse', 'breathe'],
    2: ['pingpong', 'cross', 'bounce', 'wave'],
    3: ['vu', 'strobe'],
}
FRONT_POOL = {
    0: ['breathe', 'balance', 'heart', 'sparkle'],
    1: ['sweep', 'heart', 'balance'],
    2: ['sweep', 'vu', 'balance', 'heart'],
    3: ['vu', 'sweep'],
}
WHITE = 0       # couleur 0 = blanc ; 1–9 = couleurs des logics (LOGIC_COLORS)

# Drapeaux d'un battement
KICK, SNARE, DOWN, ROLL = 1, 2, 4, 8


def _byte(x):
    return int(max(0, min(255, round(x * 255))))


class Feet:
    def __init__(self, a, plan, seed, color_at, companion):
        self.a = a
        self.plan = plan
        self.seed = seed
        self.color_at = color_at
        self.companion = companion
        self.events = []
        self.state = {'legs': None, 'front': None}
        self.effects = []          # pour la prévisualisation : (t, jambes, face)

    def rng(self, *key):
        return random.Random('feet/%s/%s' % (self.seed, '/'.join(str(k) for k in key)))

    def set(self, t, zone, fx, col=WHITE, col2=WHITE, p=0):
        key = (fx, col, col2, p)
        if self.state[zone] == key:
            return
        self.state[zone] = key
        self.events.append({'t': int(t), 'kind': zone, 'fx': fx, 'col': col, 'col2': col2, 'p': int(p)})

    def both(self, t, fx, col=WHITE, col2=WHITE, p=0):
        self.set(t, 'legs', fx, col, col2, p)
        self.set(t, 'front', fx, col, col2, p)

    @staticmethod
    def pick(rng, pool, last):
        return rng.choice([x for x in pool if x != last] or pool)

    def beat_info(self):
        """Attaques proches de chaque battement (mêmes seuils que le dôme)."""
        a = self.a
        beats = [b * 1000 for b in a['beats']]
        out = []
        for i, t in enumerate(beats):
            bm = (beats[i + 1] - t) if i + 1 < len(beats) else 60000.0 / a['tempo']
            kick = max([s for (x, s) in a['kicks'] if abs(x * 1000 - t) < bm / 3] or [0])
            snare = max([s for (x, s) in a['snares'] if abs(x * 1000 - t) < bm / 3] or [0])
            roll = sum(1 for (x, s) in a['snares'] if t <= x * 1000 < t + bm and s > 0.3) >= 3
            flags = ((KICK if kick > 0.5 else 0) | (SNARE if snare > 0.5 else 0)
                     | (DOWN if (i - a['downbeat_phase']) % 4 == 0 else 0) | (ROLL if roll else 0))
            out.append((t, bm, flags))
        return out

    def build(self, t_end):
        a = self.a
        secs = a['sections']
        beats = self.beat_info()
        n = len(beats)

        # Un événement par battement
        for i, (t, bm, flags) in enumerate(beats):
            if t > t_end:
                break
            self.events.append({'t': int(t), 'kind': 'beat', 'bass': _byte(a['beat_low'][i]),
                                'high': _byte(a['beat_high'][i]),
                                'bal': int(max(-127, min(127, round(a['beat_balance'][i] * 127)))) if a['is_stereo'] else 0,
                                'flags': flags})

        # Moments forts du plan du dôme, par ordre chronologique
        builds = self.plan['buildups']
        peak = self.plan['peak']

        def in_buildup(t):
            return any(b['t'] <= t < b['drop'] for b in builds)

        # Effets de chaque section : même étiquette et même niveau = même effet (refrains)
        last_leg = last_front = None
        chosen = {}
        for k, s in enumerate(secs):
            if s['start_beat'] >= n:
                break
            t0 = beats[s['start_beat']][0]
            if t0 > t_end:
                break
            tier = s['tier']
            col = self.color_at(s['start_beat'])
            comp = self.companion(col)
            key = (s.get('label', k), tier)
            if key in chosen and chosen[key][0] != last_leg:
                leg, front = chosen[key]
            else:
                r = self.rng('section', key)
                leg = self.pick(r, LEG_POOL[tier], last_leg)
                front = self.pick(r, FRONT_POOL[tier], last_front)
                chosen.setdefault(key, (leg, front))
            last_leg, last_front = leg, front
            s['feet'] = (leg, front)

            if in_buildup(t0):
                continue
            expl = self.plan['explosions'].get(k)
            if expl:
                # Explosion : éclair blanc jusqu'aux pieds, puis feu ou disco comme les logics
                # (feu ↔ feu, arc-en-ciel ↔ disco) pendant 4 mesures, puis l'effet de l'apogée
                bm = beats[s['start_beat']][1]
                big = 'fire' if expl == 'fire' else 'disco'
                self.set(t0, 'legs', 'bolt', WHITE, col, 250)
                self.set(t0, 'front', 'white', WHITE, col, 400)
                self.both(t0 + 300, big)
                j = min(s['end_beat'], s['start_beat'] + 16, n - 1)
                if j < s['end_beat']:
                    self.set(beats[j][0], 'legs', leg, col, comp)
                    self.set(beats[j][0], 'front', front, comp, col)
                continue
            self.set(t0, 'legs', leg, col, comp)
            self.set(t0, 'front', front, comp, col)
            # Les couleurs suivent l'harmonie : mise à jour à chaque changement de couleur de mesure
            prev = col
            for i in range(s['start_beat'], min(s['end_beat'], n)):
                if not beats[i][2] & DOWN:
                    continue
                c = self.color_at(i)
                if c != prev and not in_buildup(beats[i][0]) and beats[i][0] < t_end:
                    self.set(beats[i][0], 'legs', leg, c, self.companion(c))
                    self.set(beats[i][0], 'front', front, self.companion(c), c)
                    prev = c

        # Montées : les jambes se remplissent depuis le pied, se figent à mi-hauteur la dernière
        # mesure (suspense), noir complet sur le dernier temps
        for b in builds:
            i0 = next((i for i, x in enumerate(beats) if x[0] >= b['t']), n - 1)
            bm = beats[i0][1]
            col = self.color_at(i0)
            freeze = b['drop'] - 4 * bm
            self.both(b['t'], 'fill', col, WHITE, max(1, freeze - b['t']))
            self.both(freeze, 'freeze', col, WHITE)
            self.both(b['drop'] - bm, 'black')

        # Coup de théâtre : flash blanc sur tout le droïde, un temps de noir, puis la section reprend
        if peak is not None:
            i0 = next((i for i, x in enumerate(beats) if x[0] >= peak), n - 1)
            bm = beats[i0][1]
            self.both(peak, 'white', WHITE, WHITE, int(bm))
            self.both(peak + bm, 'black')
            s = next((s for s in secs if s['start_beat'] <= i0 < s['end_beat']), None)
            if s and i0 + 2 < n:
                c = self.color_at(i0 + 2)
                leg, front = s.get('feet', ('breathe', 'breathe'))
                if self.plan['explosions'].get(secs.index(s)) and i0 + 2 < s['start_beat'] + 16:
                    leg = front = 'fire' if self.plan['explosions'][secs.index(s)] == 'fire' else 'disco'
                self.set(beats[i0 + 2][0], 'legs', leg, c, self.companion(c))
                self.set(beats[i0 + 2][0], 'front', front, self.companion(c), c)

        # Fin : l'énergie remonte vers la battery box, puis retour au repos
        te = self.plan['end']
        col = self.color_at(n - 1)
        self.both(te, 'bow', col, WHITE, max(500, int(t_end - te)))
        self.both(t_end, 'idle')

        # Les réglages posés par les montées et le coup de théâtre remplacent ceux des sections
        # pendant leur durée : on retire les réglages de section qui tombent dedans
        blocked = [(b['t'], b['drop']) for b in builds]
        if peak is not None:
            i0 = next((i for i, x in enumerate(beats) if x[0] >= peak), n - 1)
            blocked.append((peak, beats[min(n - 1, i0 + 2)][0] - 1))
        blocked.append((te, t_end + 1))
        moments = {'fill', 'freeze', 'black', 'white', 'bow', 'idle', 'bolt'}
        kept = []
        for e in self.events:
            if (e['kind'] in ('legs', 'front') and e['fx'] not in moments
                    and any(x0 <= e['t'] < x1 for x0, x1 in blocked)):
                continue
            kept.append(e)
        # Moments posés aux bornes : la section suivante reprend à la retombée
        kept.sort(key=lambda e: (e['t'], e['kind'] != 'beat'))
        self.events = self._dedupe(kept)

        # Morse : les attaques entre les battements, seulement quand les jambes tapent le rythme
        spans = self._spans('legs', 'morse')
        for (x, s) in sorted(a['kicks'] + a['snares']):
            x *= 1000
            if s > 0.45 and any(t0 <= x < t1 for t0, t1 in spans):
                self.events.append({'t': int(x), 'kind': 'hit', 'strength': _byte(s)})
        self.events.sort(key=lambda e: (e['t'], e['kind'] != 'beat'))

        for e in self.events:
            if e['kind'] in ('legs', 'front'):
                self.effects.append({'t': e['t'], 'zone': e['kind'], 'fx': e['fx']})
        return self.events

    def _spans(self, zone, fx):
        out, start = [], None
        for e in self.events:
            if e['kind'] != zone:
                continue
            if e['fx'] == fx and start is None:
                start = e['t']
            elif e['fx'] != fx and start is not None:
                out.append((start, e['t']))
                start = None
        if start is not None:
            out.append((start, 10**9))
        return out

    @staticmethod
    def _dedupe(events):
        """Retire les réglages identiques au précédent de la même zone."""
        last = {}
        out = []
        for e in events:
            if e['kind'] in ('legs', 'front'):
                key = (e['fx'], e['col'], e['col2'], e['p'])
                if last.get(e['kind']) == key:
                    continue
                last[e['kind']] = key
            out.append(e)
        return out


def feet_track(analysis, plan):
    """Piste des pieds d'une chanson ; plan = plan du dôme renvoyé par choreography.choreograph()."""
    f = Feet(analysis, plan, plan['seed'], plan['color_at'], plan['companion'])
    t_end = analysis['sound_end'] * 1000
    return f.build(t_end)
