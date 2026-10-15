"""Genera Source/Spec.h e resources/factory_presets.json da spec.py e gen_ui.py (stessa fonte della versione Max)."""
import json, os


def fl(v):
    return repr(float(v)) + "f"

import spec as S
from gen_ui import F

ROOT = os.path.join(os.path.dirname(__file__), "..")


def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


MAPS = {"lin": "Map::lin", "exp": "Map::exp", "pow3": "Map::pow3", "db": "Map::db", "enum": "Map::choice", "int": "Map::integer"}

out = []
w = out.append
w("// Generato da tools/gen_spec_h.py: non modificare a mano.")
w("#pragma once")
w("#include <array>")
w("")
w("namespace spec")
w("{")
for k in ["MAXBANDS", "NXLFO", "NEG", "NEF", "NMIDI", "NXY", "NSLOTS", "NSTEPS", "NP", "NT", "NSRC", "SLOT0", "NBP"]:
    w("constexpr int %s = %d;" % (k, getattr(S, k)))
w("constexpr int NSTYLES = %d;" % len(S.STYLES))
w("constexpr int DEFAULT_STYLE = %d;" % S.DEFAULT_STYLE)
w("")
w("enum class Map { lin, exp, pow3, db, choice, integer };")
w("")
w("struct ParamInfo { const char* id; const char* label; const char* ui; float min, max, def; Map map; int unit; bool mod; int target; const char* group; const char* choices; int invisible; };")
w("")
w("// unit: 0 nessuna, 1 valore, 2 tempo (ms), 3 Hz, 4 dB, 5 %, 6 pan")
w("inline const std::array<ParamInfo, NP>& params()")
w("{")
w("    static const std::array<ParamInfo, NP> p {{")
for p in S.P:
    ch = "|".join(p["enum"]) if p["enum"] else ""
    w("        { %s, %s, %s, %s, %s, %s, %s, %d, %s, %d, %s, %s, %d }," % (
        cstr(p["name"]), cstr(p["label"]), cstr(p["ui"]), fl(p["min"]), fl(p["max"]), fl(p["default"]), MAPS[p["map"]], p["unit"],
        "true" if p["mod"] else "false", p["target"], cstr(p["group"]), cstr(ch), p["invisible"]))
w("    }};")
w("    return p;")
w("}")
w("")
# indici dei parametri
w("namespace P")
w("{")
for p in S.P:
    w("    constexpr int %s = %d;" % (p["name"], p["index"]))
w("}")
w("")
w("// offset dei parametri di banda (indice = b1_en + banda*NBP + offset)")
w("namespace BP")
w("{")
for j, b in enumerate(S.BAND_PARAMS):
    w("    constexpr int %s = %d;" % (b[0], j))
w("}")
w("inline constexpr int bandParam (int band, int off) { return P::b1_en + band * NBP + off; }")
w("")
w("struct Style { const char* name; const char* cat; float shtype, dscale, dofs, bias, prehp, preeqF, preeqG, preeqQ, posteqF, posteqG, posteqQ, postlp, cabF, cabG, special, inner; };")
w("inline const std::array<Style, NSTYLES>& styles()")
w("{")
w("    static const std::array<Style, NSTYLES> s {{")
for st in S.STYLES:
    w("        { %s, %s, %s }," % (cstr(st[0]), cstr(st[1]), ", ".join(fl(v) for v in st[2:])))
w("    }};")
w("    return s;")
w("}")
w("")
w("inline const char* const targetIds[NT] = { %s };" % ", ".join(cstr(t) for t in S.TARGETS))
w("inline const char* const sourceIds[NSRC] = { %s };" % ", ".join(cstr(s) for s in S.SOURCES))
w("namespace SRC")
w("{")
for i, s in enumerate(S.SOURCES):
    w("    constexpr int %s = %d;" % (s.replace("none", "none_"), i))
w("}")
w("inline const char* const syncDivs[%d] = { %s };" % (len(S.SYNC_DIVS), ", ".join(cstr(d) for d in S.SYNC_DIVS)))
w("")
w("// buffer di configurazione (interfaccia -> motore)")
w("namespace CFG")
w("{")
for k, v in S.CFG.items():
    w("    constexpr int %s = %d;" % (k, v))
w("}")
w("// buffer di visualizzazione (motore -> interfaccia)")
w("namespace VIEW")
w("{")
for k, v in S.VIEW.items():
    w("    constexpr int %s = %d;" % (k, v))
w("}")
w("} // namespace spec")
os.makedirs(os.path.join(ROOT, "Source"), exist_ok=True)
open(os.path.join(ROOT, "Source", "Spec.h"), "w").write("\n".join(out) + "\n")

# preset di fabbrica (stesso formato JSON della versione Max)
os.makedirs(os.path.join(ROOT, "resources"), exist_ok=True)
json.dump([dict(name=f["name"], cat=f["cat"], data=f["data"]) for f in F],
          open(os.path.join(ROOT, "resources", "factory_presets.json"), "w"), indent=1)
print("Spec.h:", len(S.P), "parametri,", len(S.TARGETS), "target,", len(S.SOURCES), "sorgenti,", len(F), "preset")
