"""Assembla satura_ui.js: specifica + preset di fabbrica + sorgenti ui/*.js"""
import json
import copy
from spec import *

S = STYLE_NAMES.index


def def_struct():
    return dict(
        xlfo=[dict(active=0, sync=0, retrig=0, snap=0, steps=[dict(v=1, r=0, c=3, g=1), dict(v=-1, r=0, c=3, g=1)]) for _ in range(NXLFO)],
        eg=[dict(active=0, trig=0, range=0, **{"as": 0.3}, ds=0.5, rs=0.5) for _ in range(NEG)],
        ef=[dict(active=0, trig=0, mode=0) for _ in range(NEF)],
        midi=[dict(active=0, input=0, cc=1, curve=0) for _ in range(NMIDI)],
        xy=[dict(active=0, mode=0, range=0) for _ in range(NXY)],
        slots=[], order=[], names={}, presetName="")


def slot(src, tgt, amt, inv=0):
    return dict(on=1, src=SIDX[src], tgt=TIDX[tgt], amt=amt, inv=inv)


def preset(name, cat, params=None, struct=None):
    return dict(name=name, cat=cat, data=dict(format="satura-preset", version=1, params=params or {}, struct=struct or def_struct()))


def band(b, **kw):
    return {"b%d_%s" % (b, k): v for k, v in kw.items()}


def P_(*dicts):
    out = {}
    for d in dicts:
        out.update(d)
    return out


F = []
F.append(preset("Default Setting", "Default"))

# --- Mastering
F.append(preset("Gentle Tape Master", "Mastering", P_(band(1, style=S("Subtle Tape"), drive=9, level=-0.5), dict(linphase=1, hq=1))))
F.append(preset("Multiband Warmth", "Mastering", P_(
    dict(nbands=3, xf1=200, xf2=4000, linphase=1, hq=1),
    band(1, style=S("Subtle Transformer"), drive=10, level=-0.5),
    band(2, style=S("Subtle Tube"), drive=8, level=-0.5),
    band(3, style=S("Subtle Tape"), drive=6, level=-0.3))))
F.append(preset("Air Exciter", "Mastering", P_(
    dict(nbands=2, xf1=7000, hq=1),
    band(1, style=S("Subtle Saturation"), drive=0, mix=0),
    band(2, style=S("Clean Tube"), drive=18, mix=35, pres=1.5))))

# --- Mix Bus
F.append(preset("Glue Bus", "Mix Bus", P_(band(1, style=S("Clean Tape"), drive=10, dyn=20, mix=80, level=-1))))
F.append(preset("Console Drive", "Mix Bus", P_(band(1, style=S("Warm Transformer"), drive=12, mix=100, level=-1.5, treb=-0.5))))

# --- Drums
F.append(preset("Drum Crush", "Drums", P_(
    dict(nbands=2, xf1=150),
    band(1, style=S("Warm Transformer"), drive=14, level=-1),
    band(2, style=S("Heavy Saturation"), drive=22, dyn=40, mix=70, level=-2))))
st = def_struct()
st["ef"][0].update(active=1, trig=0, mode=1)
st["order"] = ["ef1"]
st["names"] = {"ef1": "Transients"}
st["slots"] = [slot("ef1", "b1_drive", 0.45)]
F.append(preset("Transient Drive", "Drums", P_(band(1, style=S("Juicy Tube"), drive=6, level=-1), dict(f1_att=0.3, f1_rel=60)), st))

# --- Bass
F.append(preset("Bass Growl", "Bass", P_(
    dict(nbands=2, xf1=180),
    band(1, style=S("Subtle Tube"), drive=4),
    band(2, style=S("British Crunch"), drive=24, mix=60, level=-3))))
F.append(preset("Sub Saturator", "Bass", P_(
    dict(nbands=2, xf1=120, xs1=3),
    band(1, style=S("Lush Transformer"), drive=20, level=-2, bass=1),
    band(2, style=S("Subtle Tape"), drive=0, mix=0))))

# --- Vocals
F.append(preset("Vocal Presence", "Vocals", P_(
    dict(nbands=3, xf1=300, xf2=3000),
    band(1, style=S("Subtle Tape"), drive=0, mix=0),
    band(2, style=S("Clean Tube"), drive=10, level=-1),
    band(3, style=S("Subtle Tape"), drive=8, pres=2, level=-0.5))))

# --- Guitar
F.append(preset("Plexi Crunch", "Guitar", P_(band(1, style=S("British Crunch"), drive=22, level=-4, mid=1.5))))
F.append(preset("Lead Screamer", "Guitar", P_(band(1, style=S("Screaming Amp"), drive=28, level=-6, pres=2), dict(hq=1))))
F.append(preset("Clean Combo", "Guitar", P_(band(1, style=S("American Clean"), drive=18, level=-2, treb=1))))

# --- Synth
F.append(preset("Analog Warmth", "Synth", P_(band(1, style=S("Warm Tube"), drive=12, fba=15, fbf=800, level=-1.5))))

# --- Creative
st = def_struct()
st["xlfo"][0].update(active=1, sync=1)
st["order"] = ["xlfo1"]
st["slots"] = [slot("xlfo1", "b1_drive", 0.6)]
F.append(preset("Wobble Smudge", "Creative", P_(band(1, style=S("Smudge"), drive=30, level=-2), dict(x1_div=6)), st))
F.append(preset("Broken Radio", "Creative", P_(
    dict(nbands=3, xf1=500, xf2=3500),
    band(1, style=S("Subtle Tube"), drive=0, level=-60),
    band(2, style=S("Destroy"), drive=26, mix=80, level=0, mid=3),
    band(3, style=S("Broken Tube"), drive=20, level=-12))))
st = def_struct()
st["xlfo"][0].update(active=1)
st["xy"][0].update(active=1, mode=0)
st["order"] = ["xy1", "xlfo1"]
st["names"] = {"xy1": "Fold/Mix"}
st["slots"] = [slot("xlfo1", "b1_drive", 0.35), slot("xy1_x", "b1_drive", 0.5), slot("xy1_y", "b1_mix", 0.5)]
F.append(preset("Fold Sweep", "Creative", P_(band(1, style=S("Foldback"), drive=18, level=-3), dict(x1_rate=0.25)), st))
F.append(preset("Octave Down Fuzz", "Creative", P_(band(1, style=S("Breakdown"), drive=30, dpan=-20, level=-4))))

# --- Modulation
st = def_struct()
st["eg"][0].update(active=1, trig=0)
st["order"] = ["eg1"]
st["names"] = {"eg1": "Ducker"}
st["slots"] = [slot("eg1", "b1_mix", 0.8, inv=1)]
F.append(preset("Ducking Grit", "Modulation", P_(band(1, style=S("Heavy Saturation"), drive=24, level=-3), dict(e1_att=2, e1_dec=120, e1_sus=30, e1_rel=250, e1_thr=-30)), st))
st = def_struct()
st["xlfo"][0].update(active=1, snap=1, sync=1, steps=[dict(v=v, r=0, c=0, g=0) for v in [0, 0.25, 0.5833, 0.25, 1, 0.5833, 0.25, -1]])
st["order"] = ["xlfo1"]
st["names"] = {"xlfo1": "Arp"}
st["slots"] = [slot("xlfo1", "b1_fbf", 1.0)]
F.append(preset("Arp Feedback", "Modulation", P_(band(1, style=S("Warm Tube"), drive=10, fba=55, fbf=440, level=-3), dict(x1_div=7)), st))
st = def_struct()
st["xy"][0].update(active=1, mode=0)
st["xy"][1].update(active=1, mode=1, range=1)
st["order"] = ["xy1", "xy2"]
st["names"] = {"xy1": "Drive/Mix", "xy2": "Feedback"}
st["slots"] = [slot("xy1_x", "b1_drive", 0.5), slot("xy1_y", "b1_mix", 0.5), slot("xy2_y", "b1_fba", 0.8)]
F.append(preset("XY Performance", "Modulation", P_(band(1, style=S("Crunchy Amp"), drive=16, level=-3, mix=60)), st))

for f in F:
    f["data"]["struct"]["presetName"] = f["name"]


def build():
    spec = json.loads(spec_json())
    parts = ["// SATURA - interfaccia per Max for Live (jsui). Generato da build/gen_ui.py.\n",
             "var SPEC = " + json.dumps(spec, separators=(",", ":")) + ";\n",
             "var FACTORY = " + json.dumps(F, separators=(",", ":")) + ";\n"]
    for fn in ["core.js", "gfx.js", "events.js"]:
        parts.append(open("ui/" + fn).read())
    return "\n".join(parts)


if __name__ == "__main__":
    js = build()
    open("../dist/src/satura_ui.js", "w").write(js)
    print("satura_ui.js:", len(js), "caratteri,", js.count("\n"), "righe,", len(F), "preset di fabbrica")
