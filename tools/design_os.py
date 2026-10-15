# Progetto dei filtri half-band per l'oversampling di SATURA
import numpy as np, math
from scipy import signal

# ---- polifase IIR (algoritmo di de Soras / hiir)
def acc_num(q, order, c):
    i=0; j=1; acc=0.0
    while True:
        qi = q**(i*(i+1)) * math.sin((i*2+1)*c*math.pi/order) * j
        acc += qi; j=-j; i+=1
        if abs(qi) < 1e-100: return acc
def acc_den(q, order, c):
    i=1; j=-1; acc=0.0
    while True:
        qi = q**(i*i) * math.cos(i*2*c*math.pi/order) * j
        acc += qi; j=-j; i+=1
        if abs(qi) < 1e-100: return acc
def iir_coefs(nbr, transition):
    k = math.tan((1-2*transition)*math.pi/4)**2
    kk = (1-k*k)**0.25
    e = 0.5*(1-kk)/(1+kk); e2=e*e; e4=e2*e2
    q = e*(1+e4*(2+e4*(15+150*e4)))
    order = nbr*2+1
    out=[]
    for idx in range(nbr):
        c=idx+1
        num = acc_num(q,order,c)*q**0.25
        den = acc_den(q,order,c)+0.5
        ww=num/den; wwsq=ww*ww
        x=math.sqrt((1-wwsq*k)*(1-wwsq/k))/(1+wwsq)
        out.append((1-x)/(1+x))
    return out

def iir_halfband_response(coefs, w):
    # H(z) = 0.5*(A0(z^2) + z^-1 A1(z^2)), A = prod (c + z^-2)/(1 + c z^-2)
    z = np.exp(1j*w)
    def ap(cs):
        h = np.ones_like(z)
        for c in cs: h *= (c + z**-2)/(1 + c*z**-2)
        return h
    a0 = ap(coefs[0::2]); a1 = ap(coefs[1::2])
    return 0.5*(a0 + z**-1*a1)

for nbr, tr in [(12, 0.01), (8, 0.04), (6,0.1),(4,0.2)]:
    cs = iir_coefs(nbr, tr)
    w = np.linspace(0, np.pi, 20000)
    H = iir_halfband_response(cs, w)
    db = 20*np.log10(np.abs(H)+1e-30)
    f = w/np.pi  # 1 = Nyquist del rate sovracampionato
    sb = db[f > 0.5+tr].max(); pb = np.abs(db[f < 0.5-tr]).max()
    print(nbr, tr, 'stopband max %.1f dB, passband ripple %.6f dB' % (sb, pb))

print('--- scelta per stadi')
def best_iir(tr, attn=100):
    for n in range(1, 20):
        cs = iir_coefs(n, tr)
        w = np.linspace(0, np.pi, 40000); H = iir_halfband_response(cs, w); db=20*np.log10(np.abs(H)+1e-30); f=w/np.pi
        if db[f > 0.5+tr].max() < -attn: return n, cs
TRS = [0.045, 0.27, 0.36, 0.36, 0.36]
IIR = []
for k,tr in enumerate(TRS):
    n, cs = best_iir(tr); IIR.append(cs); print('IIR stadio', k+1, 'tr', tr, 'coeff', n)

def best_fir(tr, attn=100):
    for m in range(1, 200):
        N = 4*m+3
        h = signal.remez(N, [0, 0.25-tr/2, 0.25+tr/2, 0.5], [1, 0], weight=[1, 1])
        # forza half-band esatta
        c = (N-1)//2
        for i in range(N):
            if i != c and (i - c) % 2 == 0: h[i] = 0
        h[c] = 0.5
        w, H = signal.freqz(h, worN=40000); f = w/np.pi
        if 20*np.log10(np.abs(H[f > 0.5+tr]).max()+1e-30) < -attn and np.abs(20*np.log10(np.abs(H[f<0.5-tr]))).max() < 0.001:
            return N, h
FIR = []
for k,tr in enumerate(TRS):
    N, h = best_fir(tr); FIR.append(h); print('FIR stadio', k+1, 'taps', N)
import json
json.dump(dict(iir=IIR, fir=[list(h) for h in FIR], trs=TRS), open('os_design.json','w'))
