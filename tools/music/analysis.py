"""Analyse d'un fichier audio pour la génération de chorégraphies.

Tout ce qui est extrait est exprimé en secondes depuis le début du fichier, tel que le Kyber
le joue. Les niveaux sont normalisés par chanson (0–1), pour que chaque morceau ait ses
moments calmes et ses moments forts, quel que soit son volume.
"""

import json
import os

import numpy as np
import librosa

SR = 22050
HOP = 512

# Cercle des quintes : deux accords voisins donnent des couleurs voisines
FIFTHS = [0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5]   # C G D A E B F# C# G# D# A# F

ANALYSIS_VERSION = 3


def _norm(x, lo=5, hi=95):
    """Ramène x entre 0 et 1 entre ses percentiles lo et hi."""
    a, b = np.percentile(x, lo), np.percentile(x, hi)
    if b - a < 1e-9:
        return np.zeros_like(x)
    return np.clip((x - a) / (b - a), 0.0, 1.0)


def _band_energy(S, freqs, fmin, fmax):
    idx = (freqs >= fmin) & (freqs < fmax)
    return np.sqrt(np.mean(S[idx, :] ** 2, axis=0))


def analyze(path):
    y_stereo, sr = librosa.load(path, sr=SR, mono=False)
    if y_stereo.ndim == 1:
        y_stereo = np.vstack([y_stereo, y_stereo])
    y = np.mean(y_stereo, axis=0)
    duration = len(y) / sr

    # Début et fin du son (les MP3 ont souvent du silence autour)
    rms = librosa.feature.rms(y=y, hop_length=HOP)[0]
    times = librosa.frames_to_time(np.arange(len(rms)), sr=sr, hop_length=HOP)
    loud = rms > max(1e-4, 0.02 * np.max(rms))
    sound_start = float(times[np.argmax(loud)]) if loud.any() else 0.0
    sound_end = float(times[len(loud) - 1 - np.argmax(loud[::-1])]) if loud.any() else duration

    # Tempo et battements
    onset_env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=HOP)
    tempo, beat_frames = librosa.beat.beat_track(onset_envelope=onset_env, sr=sr, hop_length=HOP)
    tempo = float(np.atleast_1d(tempo)[0])
    beats = librosa.frames_to_time(beat_frames, sr=sr, hop_length=HOP)
    beats = beats[(beats >= sound_start) & (beats <= sound_end)]
    if len(beats) < 8:
        # Morceau sans pulsation nette : grille régulière à 90 BPM
        tempo = 90.0
        beats = np.arange(sound_start, sound_end, 60.0 / tempo)

    # Énergie par bande (graves, médiums, aigus) et globale
    S = np.abs(librosa.stft(y, n_fft=2048, hop_length=HOP))
    freqs = librosa.fft_frequencies(sr=sr, n_fft=2048)
    low = _norm(_band_energy(S, freqs, 20, 150))
    mid = _norm(_band_energy(S, freqs, 150, 2000))
    high = _norm(_band_energy(S, freqs, 2000, 8000))
    level = _norm(rms)

    # Attaques dans les graves (grosse caisse) et dans les aigus (caisse claire, cymbales)
    def onsets_in(band_lo, band_hi):
        env = librosa.onset.onset_strength(S=librosa.amplitude_to_db(S[(freqs >= band_lo) & (freqs < band_hi), :]),
                                           sr=sr, hop_length=HOP)
        fr = librosa.onset.onset_detect(onset_envelope=env, sr=sr, hop_length=HOP, units='frames')
        strength = _norm(env)[fr] if len(fr) else np.array([])
        t = librosa.frames_to_time(fr, sr=sr, hop_length=HOP)
        return [(float(a), float(b)) for a, b in zip(t, strength)]

    kicks = onsets_in(20, 150)
    snares = onsets_in(2000, 8000)

    # Mesures : on groupe les battements par 4, en choisissant la phase où les graves sont
    # les plus forts sur le premier temps
    beat_fr = librosa.time_to_frames(beats, sr=sr, hop_length=HOP)
    beat_fr = np.clip(beat_fr, 0, len(low) - 1)
    phase_score = [np.sum(low[beat_fr[p::4]]) for p in range(4)]
    downbeat_phase = int(np.argmax(phase_score))

    # Valeurs par battement
    def per_beat(x):
        out = []
        for i, f in enumerate(beat_fr):
            f2 = beat_fr[i + 1] if i + 1 < len(beat_fr) else min(len(x), f + 20)
            out.append(float(np.mean(x[f:max(f + 1, f2)])))
        return out

    beat_level = per_beat(level)
    beat_low = per_beat(low)
    beat_high = per_beat(high)

    # Stéréo : balance gauche/droite par battement (-1 gauche, +1 droite)
    rl = librosa.feature.rms(y=y_stereo[0], hop_length=HOP)[0]
    rr = librosa.feature.rms(y=y_stereo[1], hop_length=HOP)[0]
    bal = (rr - rl) / np.maximum(rr + rl, 1e-6)
    beat_balance = per_beat(bal)
    is_stereo = float(np.mean(np.abs(bal))) > 0.03

    # Harmonie : couleur par mesure, d'après la note dominante sur le cercle des quintes
    chroma = librosa.feature.chroma_stft(S=S ** 2, sr=sr, hop_length=HOP)
    beat_color = []
    for i, f in enumerate(beat_fr):
        f2 = beat_fr[i + 1] if i + 1 < len(beat_fr) else f + 20
        c = np.mean(chroma[:, f:max(f + 1, f2)], axis=1)
        pitch = int(np.argmax(c))
        beat_color.append(FIFTHS.index(pitch) / 12.0)   # 0–1 sur le cercle

    # Sections (couplet, refrain…) : regroupement des battements par ressemblance
    mfcc = librosa.feature.mfcc(y=y, sr=sr, hop_length=HOP, n_mfcc=13)
    feat = np.vstack([librosa.util.sync(chroma, beat_fr), librosa.util.sync(mfcc, beat_fr)])
    feat = librosa.util.normalize(feat, axis=1)
    k = int(np.clip(round((sound_end - sound_start) / 22.0), 2, 12))
    k = min(k, max(2, len(beats) // 8))
    bounds = librosa.segment.agglomerative(feat, k)
    bounds = sorted(set(int(b) for b in bounds) | {0})
    # Pas de section de moins de 4 battements
    merged = [bounds[0]]
    for b in bounds[1:]:
        if b - merged[-1] >= 4:
            merged.append(b)
    sections = []
    for i, b in enumerate(merged):
        e = merged[i + 1] if i + 1 < len(merged) else len(beats)
        if e <= b:
            continue
        sections.append({
            'start_beat': b,
            'end_beat': e,
            'level': float(np.mean(beat_level[b:e])),
            'low': float(np.mean(beat_low[b:e])),
        })

    return {
        'version': ANALYSIS_VERSION,
        'file': os.path.basename(path),
        'duration': duration,
        'sound_start': sound_start,
        'sound_end': sound_end,
        'tempo': tempo,
        'beats': [float(b) for b in beats],
        'downbeat_phase': downbeat_phase,
        'beat_level': beat_level,
        'beat_low': beat_low,
        'beat_high': beat_high,
        'beat_balance': beat_balance,
        'beat_color': beat_color,
        'is_stereo': is_stereo,
        'kicks': kicks,
        'snares': snares,
        'sections': sections,
    }


def analyze_cached(path, cache_dir):
    """Analyse avec cache (l'analyse d'un morceau prend quelques secondes)."""
    os.makedirs(cache_dir, exist_ok=True)
    cache = os.path.join(cache_dir, os.path.basename(path) + '.json')
    if os.path.exists(cache) and os.path.getmtime(cache) >= os.path.getmtime(path):
        with open(cache, encoding='utf-8') as f:
            data = json.load(f)
        if data.get('version') == ANALYSIS_VERSION:
            return data
    data = analyze(path)
    with open(cache, 'w', encoding='utf-8') as f:
        json.dump(data, f)
    return data
