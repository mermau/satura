import json
d = json.load(open('os_design.json'))
o = ["// Generato da tools/gen_os_h.py (progetto in tools/design_os.py): filtri half-band per l'oversampling", "#pragma once", "", "namespace oscoefs", "{"]
for k, cs in enumerate(d['iir']):
    o.append("inline constexpr int iirN%d = %d;" % (k, len(cs)))
    o.append("inline constexpr double iir%d[] = { %s };" % (k, ", ".join("%.17g" % c for c in cs)))
for k, h in enumerate(d['fir']):
    N = len(h); c = (N - 1) // 2
    # solo i tap pari (non centrali): h[c±1], h[c±3]... memorizzati come j = c-1-2i per i = 0..m
    taps = [h[j] for j in range(0, N) if (j - c) % 2 != 0]
    s = sum(taps); taps = [t * 0.5 / s for t in taps]   # guadagno in continua esatto
    o.append("inline constexpr int firN%d = %d;   // lunghezza totale (centro %d)" % (k, N, c))
    o.append("inline constexpr double fir%d[] = { %s };   // tap non nulli fuori dal centro, in ordine j = 0, 2, 4..." % (k, ", ".join("%.17g" % t for t in taps)))
o.append("}")
open('../Source/dsp/OsCoefs.h', 'w').write("\n".join(o) + "\n")
print("ok")
