"""Specifica condivisa di SATURA (clone Max for Live delle funzioni di Saturn 2).

Unica fonte di verita' per: parametri Live, Param di gen~, target di modulazione,
sorgenti, stili di distorsione. Usata da gen_dsp.py, gen_ui.py e build_amxd.py.
"""

MAXBANDS = 6
NXLFO = 4
NEG = 4
NEF = 4
NMIDI = 4
NXY = 6
NSLOTS = 32
NSTEPS = 16

# ---------------------------------------------------------------------------
# Stili di distorsione
# campi: name, cat, shtype, dscale, dofs, bias, prehp, preeq_f, preeq_g, preeq_q,
#        posteq_f, posteq_g, posteq_q, postlp, cab_f, cab_g, special, inner
# shtype: 0 tanh, 1 tubo asimm., 2 nastro(atan), 3 cubica, 4 trasformatore,
#         5 amplificatore 2 stadi, 6 tubo rotto, 7 foldback, 8 rettifica, 9 hard clip
# special: 0 nessuno, 1 smudge, 2 breakdown, 3 destroy
STYLES = [
    # --- Tube
    ("Subtle Tube",        "Tube",        1, 0.35, 0,  0.05, 0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   0, 0.6),
    ("Clean Tube",         "Tube",        1, 0.6,  0,  0.10, 0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   0, 1.0),
    ("Warm Tube",          "Tube",        1, 0.8,  0,  0.25, 0,    150,  1.5, 0.7, 6000, -1.5, 0.7, 16000, 0,    0,   0, 1.0),
    ("Juicy Tube",         "Tube",        1, 1.0,  6,  0.40, 0,    800,  3.0, 0.6, 3000, -2,  0.7, 15000, 0,    0,   0, 1.0),
    ("Broken Tube",        "Tube",        6, 1.0,  6,  0.35, 60,   1200, 4.0, 0.8, 0,    0,   0.7, 12000, 0,    0,   0, 1.0),
    # --- Tape
    ("Subtle Tape",        "Tape",        2, 0.35, 0,  0.0,  0,    5000, 4.0, 0.6, 5000, -4,  0.6, 18000, 90,   1.0, 0, 0.6),
    ("Clean Tape",         "Tape",        2, 0.6,  0,  0.0,  0,    5000, 5.0, 0.6, 5000, -5,  0.6, 16000, 90,   1.5, 0, 1.0),
    ("Warm Tape",          "Tape",        2, 0.8,  0,  0.05, 0,    5000, 6.0, 0.6, 5000, -6,  0.6, 13000, 80,   2.5, 0, 1.0),
    ("Extreme Tape",       "Tape",        2, 1.0,  9,  0.10, 0,    4500, 8.0, 0.6, 4500, -8,  0.6, 9000,  70,   3.0, 0, 1.0),
    # --- Saturation
    ("Subtle Saturation",  "Saturation",  3, 0.35, 0,  0.0,  0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   0, 0.6),
    ("Gentle Saturation",  "Saturation",  3, 0.7,  0,  0.0,  0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   0, 1.0),
    ("Heavy Saturation",   "Saturation",  3, 1.1,  6,  0.05, 0,    0,    0,   0.7, 0,    0,   0.7, 18000, 0,    0,   0, 1.0),
    # --- Amplifier
    ("American Clean",     "Amplifier",   5, 0.6,  3,  0.10, 70,   600,  2.0, 0.7, 500,  -3,  0.8, 7500,  2400, 2.0, 0, 1.0),
    ("American Crunch",    "Amplifier",   5, 0.9,  15, 0.20, 90,   700,  4.0, 0.7, 450,  -5,  0.9, 6000,  2500, 3.0, 0, 1.0),
    ("British Crunch",     "Amplifier",   5, 0.9,  15, 0.25, 100,  900,  5.0, 0.8, 1000, 2,   0.8, 5500,  2200, 3.0, 0, 1.0),
    ("British Lead",       "Amplifier",   5, 1.0,  27, 0.25, 120,  1000, 6.0, 0.8, 800,  2,   0.9, 5000,  2000, 4.0, 0, 1.0),
    ("Smooth Amp",         "Amplifier",   5, 0.9,  18, 0.15, 90,   700,  3.0, 0.6, 600,  -2,  0.7, 4500,  1800, 2.0, 0, 1.0),
    ("Crunchy Amp",        "Amplifier",   5, 1.0,  21, 0.30, 110,  1400, 6.0, 0.9, 700,  -4,  0.9, 6000,  2800, 4.0, 0, 1.0),
    ("Screaming Amp",      "Amplifier",   5, 1.1,  33, 0.30, 150,  1600, 8.0, 1.0, 500,  -6,  1.0, 5500,  3000, 5.0, 0, 1.0),
    # --- Transformer
    ("Subtle Transformer", "Transformer", 4, 0.4,  0,  0.10, 0,    60,   4.0, 0.7, 60,   -4,  0.7, 20000, 0,    0,   0, 0.6),
    ("Warm Transformer",   "Transformer", 4, 0.8,  0,  0.25, 0,    70,   6.0, 0.7, 70,   -5,  0.7, 17000, 0,    0,   0, 1.0),
    ("Lush Transformer",   "Transformer", 4, 1.0,  3,  0.45, 0,    80,   7.0, 0.7, 80,   -5,  0.7, 14000, 3500, 1.5, 0, 1.0),
    # --- FX
    ("Smudge",             "FX",          0, 1.0,  0,  0.0,  0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   1, 1.0),
    ("Breakdown",          "FX",          9, 1.0,  6,  0.0,  30,   0,    0,   0.7, 0,    0,   0.7, 16000, 0,    0,   2, 1.0),
    ("Foldback",           "FX",          7, 1.0,  0,  0.0,  0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   0, 1.0),
    ("Rectify",            "FX",          8, 1.0,  0,  0.0,  0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   0, 1.0),
    ("Destroy",            "FX",          9, 1.0,  3,  0.0,  0,    0,    0,   0.7, 0,    0,   0.7, 20000, 0,    0,   3, 1.0),
]
STYLE_NAMES = [s[0] for s in STYLES]
DEFAULT_STYLE = STYLE_NAMES.index("Warm Tape")
STYLE_FIELDS = ["shtype", "dscale", "dofs", "bias", "prehp", "preeq_f", "preeq_g", "preeq_q",
                "posteq_f", "posteq_g", "posteq_q", "postlp", "cab_f", "cab_g", "special", "inner"]

SYNC_DIVS = ["16", "8", "4", "2", "1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"]
SYNC_BARS = [16, 8, 4, 2, 1, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625]

# ---------------------------------------------------------------------------
# Parametri. map: lin | exp | pow3 | db (dB con -inf al minimo) | enum | int
# unitstyle Live: 0 int,1 float,2 time,3 Hz,4 dB,5 %,6 pan,9 custom
P = []


def par(name, label, mn, mx, de, mapt, unit, mod=False, enum=None, inv=0, grp="", ui=None):
    P.append(dict(name=name, label=label, min=mn, max=mx, default=de, map=mapt, unit=unit,
                  mod=mod, enum=enum, invisible=inv, group=grp, ui=ui or label))


# Globali (visibili nella striscia del device)
par("gain_in", "Input", -36, 36, 0, "lin", 4, True, grp="global")
par("pan_in", "Input Pan", -100, 100, 0, "lin", 6, True, grp="global")
par("gain_out", "Output", -36, 36, 0, "lin", 4, True, grp="global")
par("pan_out", "Output Pan", -100, 100, 0, "lin", 6, True, grp="global")
par("gmix", "Mix", 0, 100, 100, "lin", 5, True, grp="global")
par("hq", "High Quality", 0, 2, 0, "enum", 0, enum=["Off", "Good", "Superb"], grp="global")
par("linphase", "Linear Phase", 0, 1, 0, "enum", 0, enum=["Off", "On"], grp="global")
par("msmode", "Channel Mode", 0, 1, 0, "enum", 0, enum=["Stereo", "Mid/Side"], grp="global")
par("unrestricted", "Unrestricted FB", 0, 1, 0, "enum", 0, enum=["Off", "On"], grp="global")
par("bypass", "Bypass", 0, 1, 0, "enum", 0, enum=["Off", "On"], grp="global")
par("nbands", "Bands", 1, MAXBANDS, 1, "int", 0, inv=1, grp="global")

XF_DEF = [200, 2000, 5000, 9000, 14000]
for i in range(1, MAXBANDS):
    par("xf%d" % i, "Crossover %d" % i, 10, 30000, XF_DEF[i - 1], "exp", 3, True, grp="xover")
for i in range(1, MAXBANDS):
    par("xs%d" % i, "Slope %d" % i, 0, 3, 2, "enum", 0, enum=["6 dB/oct", "12 dB/oct", "24 dB/oct", "48 dB/oct"], grp="xover")

BAND_PARAMS = [
    # suffix, label, min, max, def, map, unit, mod, enum
    ("en", "Enabled", 0, 1, 1, "enum", 0, False, ["Off", "On"]),
    ("solo", "Solo", 0, 1, 0, "enum", 0, False, ["Off", "On"]),
    ("mute", "Mute", 0, 1, 0, "enum", 0, False, ["Off", "On"]),
    ("style", "Style", 0, len(STYLES) - 1, DEFAULT_STYLE, "enum", 0, False, STYLE_NAMES),
    ("mix", "Mix", 0, 100, 100, "lin", 5, True, None),
    ("fba", "Feedback", 0, 100, 0, "lin", 5, True, None),
    ("fbf", "Feedback Freq", 20, 20000, 1000, "exp", 3, True, None),
    ("dyn", "Dynamics", -100, 100, 0, "lin", 5, True, None),
    ("drive", "Drive", 0, 48, 6, "lin", 4, True, None),
    ("dpan", "Drive Pan", -100, 100, 0, "lin", 6, True, None),
    ("bass", "Bass", -12, 12, 0, "lin", 4, True, None),
    ("mid", "Mid", -12, 12, 0, "lin", 4, True, None),
    ("treb", "Treble", -12, 12, 0, "lin", 4, True, None),
    ("pres", "Presence", -12, 12, 0, "lin", 4, True, None),
    ("level", "Level", -60, 36, 0, "db", 4, True, None),
    ("lpan", "Level Pan", -100, 100, 0, "lin", 6, True, None),
]
NBP = len(BAND_PARAMS)
for b in range(1, MAXBANDS + 1):
    for (sfx, lab, mn, mx, de, mp, un, md, en) in BAND_PARAMS:
        par("b%d_%s" % (b, sfx), "B%d %s" % (b, lab), mn, mx, de, mp, un, md, enum=en, grp="band%d" % b, ui=lab)

XLFO_PARAMS = [
    ("rate", "Rate", 0.02, 500, 1.0, "exp", 3, True, None),
    ("div", "Sync", 0, len(SYNC_DIVS) - 1, 6, "enum", 0, False, SYNC_DIVS),
    ("ofs", "Offset", 0.5, 2.0, 1.0, "exp", 1, True, None),
    ("bal", "Balance", -100, 100, 0, "lin", 5, True, None),
    ("glide", "Glide", -100, 100, 0, "lin", 5, True, None),
    ("phase", "Phase", 0, 100, 0, "lin", 5, True, None),
]
for k in range(1, NXLFO + 1):
    for (sfx, lab, mn, mx, de, mp, un, md, en) in XLFO_PARAMS:
        par("x%d_%s" % (k, sfx), "XLFO%d %s" % (k, lab), mn, mx, de, mp, un, md, enum=en, grp="xlfo%d" % k, ui=lab)

EG_PARAMS = [
    ("thr", "Threshold", -60, 0, -24, "lin", 4, True, None),
    ("del", "Delay", 0, 2000, 0, "pow3", 2, True, None),
    ("att", "Attack", 0, 5000, 5, "pow3", 2, True, None),
    ("dec", "Decay", 0, 10000, 200, "pow3", 2, True, None),
    ("sus", "Sustain", 0, 100, 50, "lin", 5, True, None),
    ("hold", "Hold", 0, 5000, 0, "pow3", 2, True, None),
    ("rel", "Release", 0, 10000, 300, "pow3", 2, True, None),
]
for k in range(1, NEG + 1):
    for (sfx, lab, mn, mx, de, mp, un, md, en) in EG_PARAMS:
        par("e%d_%s" % (k, sfx), "EG%d %s" % (k, lab), mn, mx, de, mp, un, md, enum=en, grp="eg%d" % k, ui=lab)

EF_PARAMS = [
    ("att", "Attack", 0.1, 1000, 5, "exp", 2, True, None),
    ("rel", "Release", 1, 5000, 150, "exp", 2, True, None),
]
for k in range(1, NEF + 1):
    for (sfx, lab, mn, mx, de, mp, un, md, en) in EF_PARAMS:
        par("f%d_%s" % (k, sfx), "EF%d %s" % (k, lab), mn, mx, de, mp, un, md, enum=en, grp="ef%d" % k, ui=lab)

for k in range(1, NXY + 1):
    par("xy%d_x" % k, "XY%d X" % k, -100, 100, 0, "lin", 5, False, grp="xy%d" % k, ui="X")
    par("xy%d_y" % k, "XY%d Y" % k, -100, 100, 0, "lin", 5, False, grp="xy%d" % k, ui="Y")

for i, p in enumerate(P):
    p["index"] = i
PNAMES = [p["name"] for p in P]
PIDX = {p["name"]: p["index"] for p in P}
NP = len(P)

# Target di modulazione: parametri mod + livelli degli slot
TARGETS = [p["name"] for p in P if p["mod"]]
for s in range(NSLOTS):
    TARGETS.append("slot%d" % (s + 1))
TIDX = {n: i for i, n in enumerate(TARGETS)}
NT = len(TARGETS)
SLOT0 = TIDX["slot1"]
for p in P:
    p["target"] = TIDX.get(p["name"], -1)

# Sorgenti: 0 = nessuna
SOURCES = ["none"]
SOURCES += ["xlfo%d" % k for k in range(1, NXLFO + 1)]
SOURCES += ["eg%d" % k for k in range(1, NEG + 1)]
SOURCES += ["ef%d" % k for k in range(1, NEF + 1)]
SOURCES += ["midi%d" % k for k in range(1, NMIDI + 1)]
for k in range(1, NXY + 1):
    SOURCES += ["xy%d_x" % k, "xy%d_y" % k]
SIDX = {n: i for i, n in enumerate(SOURCES)}
NSRC = len(SOURCES)

# ---------------------------------------------------------------------------
# Layout del buffer di configurazione (scritto dalla UI, letto da gen~)
CFG = dict(
    SLOT_BASE=100, SLOT_STRIDE=8,          # on, src, tgt, amt, inv
    XLFO_BASE=400, XLFO_STRIDE=80,         # active, nsteps, sync, retrig, snap, -, -, -, steps*4 (val, rnd, curve, glide)
    EG_BASE=800, EG_STRIDE=16,             # active, trig, range, aslope, dslope, rslope
    EF_BASE=900, EF_STRIDE=16,             # active, trig, mode
    XY_BASE=1000, XY_STRIDE=4,             # active, mode, range
    MIDI_BASE=1100, MIDI_STRIDE=4,         # active, input, cc, curve
    SIZE=1280,
)
# Layout del buffer di visualizzazione (scritto da gen~, letto dalla UI)
VIEW = dict(
    TGT=0,          # NT valori normalizzati effettivi (0..1) / livello effettivo slot (-1..1)
    SRC=200,        # NSRC uscite sorgenti
    SCOPEPOS=240,
    TRIG=241,       # 8 livelli trigger in dB (main, sc, b1..b6)
    LFOPH=250,      # fase per XLFO
    LFOSTEP=254,    # passo corrente per XLFO
    EGSTAGE=258,    # stadio EG
    METER=262,      # in L, in R, out L, out R (dB)
    BANDOUT=266,    # picco uscita banda (dB)
    LATENCY=272,
    SIZE=512,
)
SCOPE_SIZE = 8192
TRIG_SOURCES = ["Main Input", "External Side Chain"] + ["Band %d Input" % b for b in range(1, MAXBANDS + 1)] + ["MIDI"]


def spec_json():
    import json
    return json.dumps(dict(P=P, STYLES=[dict(zip(["name", "cat"] + STYLE_FIELDS, s)) for s in STYLES],
                           STYLE_NAMES=STYLE_NAMES, SYNC_DIVS=SYNC_DIVS, TARGETS=TARGETS, TIDX=TIDX, SLOT0=SLOT0,
                           SOURCES=SOURCES, SIDX=SIDX, CFG=CFG, VIEW=VIEW, SCOPE_SIZE=SCOPE_SIZE,
                           MAXBANDS=MAXBANDS, NXLFO=NXLFO, NEG=NEG, NEF=NEF, NMIDI=NMIDI, NXY=NXY, NSLOTS=NSLOTS,
                           NSTEPS=NSTEPS, NBP=NBP, BAND_PARAMS=[b[0] for b in BAND_PARAMS], TRIG_SOURCES=TRIG_SOURCES,
                           XF_DEF=XF_DEF), separators=(",", ":"))


if __name__ == "__main__":
    open("test/spec.json", "w").write(spec_json())
