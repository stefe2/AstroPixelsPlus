"""Vérifie qu'une chorégraphie générée respecte le format du firmware et les limites du droïde."""

import re

import choreography as ch

LE_RE = re.compile(r'^LE[1345]\d{6}$')
HP_RE = re.compile(r'^HP[FRTA]0\d{2}\d{0,2}$')


def validate(events, duration_ms, title_ms):
    """Renvoie (erreurs, statistiques)."""
    errors = []
    last_t = -1
    panel_last = {m: None for m in ch.PANEL_NAMES}
    per_second = {}
    for e in events:
        t = e['t']
        if t < last_t:
            errors.append('événements non triés à %d ms' % t)
        last_t = t
        if not 0 <= t <= duration_ms:
            errors.append('événement hors de la chanson à %d ms' % t)
        per_second[t // 1000] = per_second.get(t // 1000, 0) + 1
        if e['kind'] == 'cmd':
            for c in e['cmd'].split('\n'):
                if c.startswith('LE'):
                    if not LE_RE.match(c):
                        errors.append('commande LE invalide %r à %d ms' % (c, t))
                    elif c[2] == '3' and t < title_ms:
                        errors.append('logic arrière modifiée pendant le titre (%r à %d ms)' % (c, t))
                elif not HP_RE.match(c):
                    errors.append('commande holo invalide %r à %d ms' % (c, t))
            if len(e['cmd']) > 200:
                errors.append('chaîne de commandes trop longue à %d ms' % t)
        elif e['kind'] == 'panels':
            if not 0 < e['mask'] <= ch.ALL:
                errors.append('masque de panneaux invalide à %d ms' % t)
            if not 0 <= e['pct'] <= 100 or not ch.PANEL_MIN_MOVE_MS <= e['ms'] <= 65535:
                errors.append('mouvement de panneaux invalide à %d ms' % t)
            for m in ch.PANEL_NAMES:
                if e['mask'] & m:
                    if panel_last[m] is not None and t - panel_last[m] < ch.PANEL_MIN_GAP_MS:
                        errors.append('%s commandé deux fois en %d ms à %d ms'
                                      % (ch.PANEL_NAMES[m], t - panel_last[m], t))
                    panel_last[m] = t
        elif e['kind'] == 'holo':
            if not 0 <= e['pos'] <= 8 or not 0 <= e['holo'] <= 3 or not 0 < e['ms'] <= 65535:
                errors.append('mouvement de holo invalide à %d ms' % t)
        else:
            errors.append('type d\'événement inconnu à %d ms' % t)
    stats = {'max_per_second': max(per_second.values()) if per_second else 0,
             'avg_per_second': len(events) / max(1.0, duration_ms / 1000)}
    return errors, stats
