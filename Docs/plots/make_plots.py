"""
Regenerates the README figures in Docs/images/.

    python -m pip install matplotlib numpy
    python Docs/plots/make_plots.py

The curves come from the same maths as the C++ nodes:
  Delay line  (DelayLineMetasoundNode.cpp)
      w[n] = x[n] + g * w[n-N]          buffer contents
      y[n] = x[n] + w[n-N]              output
      =>  H(z) = (1 + (1-g) z^-N) / (1 - g z^-N)
  Doppler     (DopplerEffectMetasoundNode.cpp)
      ratio = (c + v_o) / (c - v_s), clamped to >= 0
"""
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

OUT = Path(__file__).resolve().parents[1] / "images"
OUT.mkdir(parents=True, exist_ok=True)

THEMES = {
    "light": dict(surface="#fcfcfb", text="#0b0b0b", muted="#52514e", grid="#e4e3df",
                  series=["#2a78d6", "#eb6834", "#1baf7a"]),
    "dark": dict(surface="#1a1a19", text="#f0efec", muted="#c3c2b7", grid="#34332f",
                 series=["#3987e5", "#d95926", "#199e70"]),
}


def styled_axes(theme, title, subtitle, figsize=(9, 4.2)):
    t = THEMES[theme]
    fig, ax = plt.subplots(figsize=figsize, dpi=100)
    fig.patch.set_facecolor(t["surface"])
    ax.set_facecolor(t["surface"])
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(t["grid"])
    ax.tick_params(colors=t["muted"], labelsize=9, length=0, pad=6)
    ax.grid(True, color=t["grid"], linewidth=0.6)
    ax.set_axisbelow(True)
    fig.text(0.07, 0.95, title, fontsize=12.5, color=t["text"], weight="bold", ha="left", va="top")
    fig.text(0.07, 0.885, subtitle, fontsize=9.5, color=t["muted"], ha="left", va="top")
    fig.subplots_adjust(left=0.07, right=0.97, top=0.76, bottom=0.14)
    return fig, ax, t


def label_axes(ax, t, xlabel, ylabel):
    ax.set_xlabel(xlabel, color=t["muted"], fontsize=9.5)
    ax.set_ylabel(ylabel, color=t["muted"], fontsize=9.5)


def save(fig, name, theme):
    path = OUT / f"{name}_{theme}.svg"
    fig.savefig(path, facecolor=fig.get_facecolor(), metadata={"Date": None})
    plt.close(fig)
    print("wrote", path.relative_to(OUT.parents[1]))


def delay_response(theme):
    fs, delay_s = 48000, 0.010
    n = int(fs * delay_s)
    f = np.linspace(0.5, 500, 6000)
    zN = np.exp(-1j * 2 * np.pi * f / fs * n)
    fig, ax, t = styled_axes(
        theme,
        "Delay line magnitude response",
        "10 ms delay at 48 kHz. Peaks every 1/delay = 100 Hz, notches halfway between.",
    )
    gains = [0.0, 0.5, 0.8]
    for g, color in zip(gains, t["series"]):
        h = (1 + (1 - g) * zN) / (1 - g * zN)
        db = 20 * np.log10(np.maximum(np.abs(h), 1e-6))
        peak_db = 20 * np.log10((2 - g) / (1 - g))
        ax.plot(f, db, color=color, linewidth=1.6, solid_capstyle="round",
                label=f"feedback {g:g}  (peaks {peak_db:+.1f} dB)")
    ax.set_xlim(0, 500)
    ax.set_ylim(-30, 19)
    ax.set_xticks(range(0, 501, 50))
    label_axes(ax, t, "Frequency (Hz)", "Gain (dB)")
    leg = ax.legend(loc="lower right", bbox_to_anchor=(1.0, 1.01), frameon=False, fontsize=9, ncol=3,
                    handlelength=1.6, columnspacing=1.4)
    for txt in leg.get_texts():
        txt.set_color(t["text"])
    save(fig, "delay_response", theme)


def delay_impulse(theme):
    delay_s, g = 0.25, 0.6
    fig, ax, t = styled_axes(
        theme,
        "Delay line impulse response",
        "250 ms delay, feedback 0.6. The first echo is full level, then each repeat is scaled by g.",
        figsize=(9, 3.6),
    )
    times = np.arange(0, 11) * delay_s
    amps = np.array([1.0, 1.0] + [g ** k for k in range(1, 10)])
    color = t["series"][0]
    ax.vlines(times, 0, amps, color=color, linewidth=1.6)
    ax.plot(times, amps, "o", color=color, markersize=6, markeredgecolor=t["surface"], markeredgewidth=1.5)
    ax.annotate("dry input", (0, 1.0), xytext=(8, 4), textcoords="offset points", fontsize=9, color=t["text"])
    ax.annotate("1st echo", (delay_s, 1.0), xytext=(8, 4), textcoords="offset points", fontsize=9, color=t["text"])
    ax.annotate("× 0.6 per repeat", (2 * delay_s, g), xytext=(10, 4), textcoords="offset points", fontsize=9, color=t["text"])
    ax.axhline(0, color=t["grid"], linewidth=1)
    ax.set_xlim(-0.08, 2.65)
    ax.set_ylim(0, 1.18)
    label_axes(ax, t, "Time (s)", "Amplitude")
    fig.subplots_adjust(top=0.76, bottom=0.17)
    save(fig, "delay_impulse", theme)


def doppler_curve(theme):
    c = 343.0
    fig, ax, t = styled_axes(
        theme,
        "Doppler pitch multiplier vs. source speed",
        "Stationary listener, c = 343 m/s (the node takes cm/s, default 34300). Negative speed = moving away.",
    )
    color = t["series"][0]
    v = np.linspace(-200, c - 1e-3, 4000)
    ax.plot(v, c / (c - v), color=color, linewidth=1.6, solid_capstyle="round")
    ax.plot([c, 450], [0, 0], color=color, linewidth=1.6, solid_capstyle="round")
    ax.axvspan(c, 450, color=t["grid"], alpha=0.6, linewidth=0)
    ax.axvline(c, color=t["muted"], linewidth=1, linestyle=(0, (3, 3)))
    ax.text(c + 8, 3.55, "Mach 1\nsupersonic: output\nclamped to 0", fontsize=9, color=t["text"], va="top")
    for speed, side in [(30, "approaching"), (-30, "receding")]:
        r = c / (c - speed)
        semis = 12 * np.log2(r)
        ax.plot([speed], [r], "o", color=color, markersize=7, markeredgecolor=t["surface"], markeredgewidth=1.5)
        # Approaching label sits above-left of its point, receding label below-left, both clear of the curve
        offset = (-12, 14) if speed > 0 else (-12, -32)
        ax.annotate(f"train {side} at 30 m/s\n×{r:.3f} ({semis:+.1f} semitones)", (speed, r), xytext=offset,
                    textcoords="offset points", fontsize=9, color=t["text"], ha="right")
    ax.set_xlim(-200, 450)
    ax.set_ylim(-0.1, 4)
    label_axes(ax, t, "Source speed towards listener (m/s)", "Pitch multiplier")
    save(fig, "doppler_curve", theme)


if __name__ == "__main__":
    for theme in THEMES:
        delay_response(theme)
        delay_impulse(theme)
        doppler_curve(theme)
