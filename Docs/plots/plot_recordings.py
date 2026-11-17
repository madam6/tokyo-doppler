"""
Plots the two in-game recordings in Docs/audio (captured from the master submix in PIE):

  gunshot_delay.wav   one rifle shot through MSS_DelayLineGunshot (200 ms, feedback 0.5)
  flyby.wav           the maglev passing a player standing under the track
  flyby_pitch.csv     time, pitch (semitones) printed by the Doppler MetaSound during the fly-by,
                      plus the value predicted from the thruster's logged path

    python Docs/plots/plot_recordings.py
"""
import csv
import wave
from pathlib import Path

import numpy as np

import make_plots as mp

AUDIO = Path(__file__).resolve().parents[1] / "audio"


def load_wav(path):
    with wave.open(str(path)) as w:
        rate, ch, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
        raw = w.readframes(w.getnframes())
    dtype = {2: np.int16, 4: np.int32}[width]
    data = np.frombuffer(raw, dtype=dtype).astype(np.float64) / np.iinfo(dtype).max
    return rate, data.reshape(-1, ch).mean(axis=1)


def envelope_db(x, rate, win_ms=5):
    n = max(1, int(rate * win_ms / 1000))
    frames = len(x) // n
    rms = np.sqrt(np.mean(x[: frames * n].reshape(frames, n) ** 2, axis=1))
    t = (np.arange(frames) + 0.5) * n / rate
    return t, 20 * np.log10(np.maximum(rms, 1e-6))


def gunshot(theme):
    rate, x = load_wav(AUDIO / "gunshot_delay.wav")
    t, db = envelope_db(x, rate, win_ms=20)
    onset = t[np.argmax(db > db.max() - 30)]
    t = t - onset
    db = db - db.max()
    fig, ax, th = mp.styled_axes(
        theme,
        "Recorded in game: one rifle shot through the delay node",
        "RMS level in 20 ms windows. The shot is longer than the 200 ms delay, so the repeats overlap into one decaying tail.",
    )
    ax.plot(t, db, color=th["series"][0], linewidth=1.6)
    # Feedback 0.5 means -6 dB per 200 ms repeat, i.e. a -30 dB/s slope. Fit only the intercept.
    fit = (t > 0.5) & (t < 1.6)
    intercept = np.mean(db[fit] + 30 * t[fit])
    ref_t = np.array([0.4, 1.75])
    ax.plot(ref_t, intercept - 30 * ref_t, color=th["muted"], linewidth=1.2, linestyle=(0, (4, 3)))
    ax.annotate("-6 dB per 200 ms repeat\n(feedback 0.5)", (1.1, intercept - 33), xytext=(-14, -30),
                textcoords="offset points", fontsize=9, color=th["text"], ha="right")
    cut = t[np.nonzero(db > -70)[0][-1]]
    ax.annotate("source stops when the\nWave Player finishes", (cut, -45), xytext=(12, 0),
                textcoords="offset points", fontsize=9, color=th["text"], va="center")
    ax.set_xlim(-0.1, 2.6)
    ax.set_ylim(-60, 3)
    mp.label_axes(ax, th, "Time after the shot (s)", "Level (dB, relative to peak)")
    fig.subplots_adjust(left=0.085)
    mp.save(fig, "ingame_gunshot", theme)


def flyby(theme):
    rows = list(csv.DictReader(open(AUDIO / "flyby_pitch.csv")))
    t_m = np.array([float(r["t"]) for r in rows if r["measured"]])
    s_m = np.array([float(r["measured"]) for r in rows if r["measured"]])
    t_p = np.array([float(r["t"]) for r in rows if r["predicted"]])
    s_p = np.array([float(r["predicted"]) for r in rows if r["predicted"]])
    fig, ax, th = mp.styled_axes(
        theme,
        "Recorded in game: pitch shift during a fly-by",
        "Player standing still under the track. Line: predicted from the thruster's path. Dots: what the MetaSound printed.",
    )
    ax.plot(t_p, s_p, color=th["series"][0], linewidth=1.6, label="predicted from geometry")
    ax.plot(t_m, s_m, "o", color=th["series"][1], markersize=6, markeredgecolor=th["surface"], markeredgewidth=1.2,
            label="Doppler node output (printed every 0.5 s)")
    ax.axhline(0, color=th["grid"], linewidth=1)
    ax.annotate("train stops at the\nend of the line", (12.5, 0), xytext=(0, 14), textcoords="offset points",
                fontsize=9, color=th["text"], va="bottom")
    fig.subplots_adjust(left=0.085)
    leg = ax.legend(loc="lower right", bbox_to_anchor=(1.0, 1.01), frameon=False, fontsize=9, ncol=2,
                    handlelength=1.6, columnspacing=1.4)
    for txt in leg.get_texts():
        txt.set_color(th["text"])
    mp.label_axes(ax, th, "Time (s)", "Pitch shift (semitones)")
    mp.save(fig, "ingame_flyby", theme)


if __name__ == "__main__":
    for theme in mp.THEMES:
        gunshot(theme)
        flyby(theme)
