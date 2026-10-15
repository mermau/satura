#include "Engine.h"

namespace sdsp
{
using namespace spec;

namespace
{
// tanh veloce: Padé (7,6) vicino allo zero, forma esponenziale oltre (errore massimo 1.4e-7)
inline double ftanh (double x)
{
    const double a = std::abs (x);
    if (a < 2.5)
    {
        const double x2 = x * x;
        return x * (135135 + x2 * (17325 + x2 * (378 + x2))) / (135135 + x2 * (62370 + x2 * (3150 + 28 * x2)));
    }
    const double e = std::exp (-2 * a), t = (1 - e) / (1 + e);
    return x < 0 ? -t : t;
}
// curve di saturazione (stesse della versione gen~)
inline double shapeFn (double x, double t, double bias)
{
    if (t < 0.5) return ftanh (x);
    if (t < 1.5) return ftanh (x + bias) - ftanh (bias);
    if (t < 2.5) return std::atan (x * 1.5707963) * 0.6366198;
    if (t < 3.5) { const double xc = clampd (x * 0.6666667, -1, 1); return xc * 1.5 - xc * xc * xc * 0.5; }
    if (t < 4.5) return ftanh (x + bias * x * x * 0.5);
    if (t < 5.5) { const double a = ftanh (x * 0.7 + bias) - ftanh (bias); return ftanh (a * 4) * 0.8 + a * 0.2; }
    if (t < 6.5) { const double dz = (x > 0 ? 1 : (x < 0 ? -1 : 0)) * std::max (std::abs (x) - 0.06, 0.0) * 1.07; return ftanh (dz + bias) - ftanh (bias); }
    if (t < 7.5) return 1 - 4 * std::abs (wrap01 (x * 0.25 + 0.25) - 0.5);
    if (t < 8.5) return ftanh (std::abs (x) * 0.75 + x * 0.25);
    return clampd (x, -1, 1);
}
inline double normOf (const ParamInfo& p, double v)
{
    double n;
    if (p.map == Map::exp) n = std::log (std::max (v, (double) p.min) / p.min) / std::log ((double) p.max / p.min);
    else if (p.map == Map::pow3) n = std::cbrt (std::max ((v - p.min) / (p.max - p.min), 0.0));
    else n = (v - p.min) / (p.max - p.min);
    return clampd (n, 0, 1);
}
inline double denormOf (const ParamInfo& p, double n)
{
    if (p.map == Map::exp) return p.min * std::pow ((double) p.max / p.min, n);
    if (p.map == Map::pow3) return p.min + (p.max - p.min) * n * n * n;
    return p.min + (p.max - p.min) * n;
}
inline double curveFn (int c, double x)
{
    switch (c)
    {
        case 1: return (std::exp (x * 3) - 1) / (std::exp (3.0) - 1);
        case 2: return std::log (1 + x * 19) / std::log (20.0);
        case 3: return x * x;
        case 4: return std::sqrt (x);
        case 5: return 0.5 - 0.5 * std::cos (kPi * x);
        default: return x;
    }
}
} // namespace

Engine::Engine()
{
    for (auto& v : view) v.store (0.0f, std::memory_order_relaxed);
    for (int i = 0; i < NP; ++i) praw[(size_t) i] = params()[(size_t) i].def;
    prepare (48000.0);
}

int Engine::latencyFor (bool lp, int hq) const
{
    if (! lp) return 0;
    Oversampler tmp;
    tmp.prepare (hq == 0 ? 0 : (hq == 1 ? 3 : 5), Oversampler::fir);
    return lps.latency() + tmp.latency();
}

void Engine::prepare (double sampleRate)
{
    fs = sampleRate;
    const int mult = std::max (1, (int) std::lround (fs / 48000.0));
    int m2 = 1;
    while (m2 < mult) m2 <<= 1;
    lps.prepare (fs, 2048 * m2, 256);
    for (auto& b : bch)
        for (auto& c : b)
        {
            c.smudge.assign (8192, 0.0);
            c.dryDelay.prepare (256);
        }
    const int maxLat = 2048 * m2 + 256 + 256;
    bypassDelayL.prepare (maxLat);
    bypassDelayR.prepare (maxLat);
    kFast = 1.0 - std::exp (-1.0 / (0.004 * fs));
    kSlow = 1.0 - std::exp (-1.0 / (0.03 * fs));
    kByp = 1.0 - std::exp (-1.0 / (0.012 * fs));
    kDyn[0] = 1.0 - std::exp (-1.0 / (0.003 * fs));
    kDyn[1] = 1.0 - std::exp (-1.0 / (0.12 * fs));
    linPhase = praw[(size_t) P::linphase] > 0.5f;
    osLog2 = praw[(size_t) P::hq] < 0.5f ? 0 : (praw[(size_t) P::hq] < 1.5f ? 3 : 5);
    updateStructure();
    reset();
}

void Engine::updateStructure()
{
    const auto mode = linPhase ? Oversampler::fir : Oversampler::iir;
    for (auto& b : bch)
        for (auto& c : b)
        {
            c.os.prepare (osLog2, mode);
            c.osDry.prepare (osLog2, mode);
        }
    lps.reset();
    curLatency = linPhase ? lps.latency() + bch[0][0].os.latency() : 0;
}

void Engine::reset()
{
    for (auto& ch : xoSt) for (auto& x : ch) { for (auto& s : x.lp) s.reset(); for (auto& s : x.hp) s.reset(); }
    for (auto& ch : apSt) for (auto& b : ch) for (auto& x : b) for (auto& s : x) s.reset();
    for (auto& b : bch)
        for (auto& c : b)
        {
            c.fb.reset(); c.prehp.reset(); c.preeq.reset(); c.posteq.reset(); c.postlp.reset(); c.cab.reset();
            for (auto& t : c.tone) t.reset();
            c.dcx = c.dcy = c.fbLast = c.env = c.bdPrev = 0; c.bdFlip = 1; c.dCnt = c.dHold = 0;
            std::fill (c.smudge.begin(), c.smudge.end(), 0.0);
            c.os.reset(); c.osDry.reset(); c.dryDelay.reset();
        }
    lps.reset();
    bypassDelayL.reset(); bypassDelayR.reset();
    ctl = 0;
    beatInc = 2.0 / fs;
    // valori iniziali immediati (niente rampe all'avvio)
    controlBlock();
    for (int c = 0; c < 2; ++c) { igain[c].v = igain[c].t; ogain[c].v = ogain[c].t; }
    gmix.v = gmix.t; byp.v = byp.t; fbScale.v = fbScale.t = 1;
    for (auto& s : bsm) { s.gain[0].v = s.gain[0].t; s.gain[1].v = s.gain[1].t; s.mix.v = s.mix.t; s.lvl[0].v = s.lvl[0].t; s.lvl[1].v = s.lvl[1].t; s.aud.v = s.aud.t; s.fba.v = s.fba.t; s.en.v = s.en.t; }
}

void Engine::setParams (const float* raw) { std::copy (raw, raw + NP, praw.begin()); }
void Engine::setConfig (const float* c) { std::copy (c, c + CFG::SIZE, cfg.begin()); }

double Engine::sourceMidi (int k) const
{
    const int b = CFG::MIDI_BASE + k * CFG::MIDI_STRIDE;
    if (cfg[(size_t) b] < 0.5f) return 0;
    const int input = (int) cfg[(size_t) b + 1], cc = clampd (cfg[(size_t) b + 2], 0, 127), curve = (int) cfg[(size_t) b + 3];
    double x;
    switch (input)
    {
        case 0: x = midiVel; break;
        case 1: x = midiNote / 127.0; break;
        case 2: return (midiBend < 0 ? -1 : 1) * curveFn (curve, std::abs (midiBend));
        case 3: x = midiMw; break;
        case 4: x = midiAt; break;
        default: x = midiCc[(size_t) cc] / 127.0;
    }
    return curveFn (curve, clampd (x, 0, 1));
}

void Engine::handleMidi (const MidiEvent& e)
{
    const int type = e.st & 0xF0;
    if (type == 0x90 && e.d2 > 0)
    {
        midiVel = e.d2 / 127.0; midiNote = e.d1;
        midiTrig += 1;
        if (heldNotes == 0) midiLTrig += 1;
        ++heldNotes;
        midiGate = 1;
    }
    else if (type == 0x80 || (type == 0x90 && e.d2 == 0))
    {
        heldNotes = std::max (0, heldNotes - 1);
        if (heldNotes == 0) midiGate = 0;
    }
    else if (type == 0xB0)
    {
        midiCc[e.d1 & 127] = e.d2;
        if (e.d1 == 1) midiMw = e.d2 / 127.0;
        if (e.d1 == 123 || e.d1 == 120) { heldNotes = 0; midiGate = 0; }
    }
    else if (type == 0xE0) midiBend = clampd ((((int) e.d2 << 7) + e.d1 - 8192) / 8191.0, -1, 1);
    else if (type == 0xD0) midiAt = e.d1 / 127.0;
    else if (type == 0xA0) midiAt = e.d2 / 127.0;
}

void Engine::designCrossoversIir()
{
    double prev = 5;
    for (int i = 0; i < nb - 1; ++i)
    {
        double f = std::max (pv[(size_t) (P::xf1 + i)], prev * 1.02);
        f = clampd (f, 10.0, fs * 0.45);
        prev = f;
        xfreq[(size_t) i] = f;
        auto& x = xo[(size_t) i];
        x.slope = (int) pv[(size_t) (P::xs1 + i)];
        const auto I = BiquadCoefs::identity();
        using B = BiquadCoefs;
        for (auto& c : x.lp) c = I;
        for (auto& c : x.hp) c = I;
        x.ap[0] = x.ap[1] = I;
        switch (x.slope)
        {
            case 0: x.lp[0] = B::make (B::lp1, fs, f); break;
            case 1:
                x.lp[0] = x.lp[1] = B::make (B::lp1, fs, f);
                x.hp[0] = x.hp[1] = B::make (B::hp1, fs, f);
                x.ap[0] = B::make (B::ap1, fs, f);
                break;
            case 2:
                x.lp[0] = x.lp[1] = B::make (B::lp, fs, f);
                x.hp[0] = x.hp[1] = B::make (B::hp, fs, f);
                x.ap[0] = B::make (B::ap, fs, f);
                break;
            default:
                x.lp[0] = x.lp[2] = B::make (B::lp, fs, f, 0.5411961);
                x.lp[1] = x.lp[3] = B::make (B::lp, fs, f, 1.3065630);
                x.hp[0] = x.hp[2] = B::make (B::hp, fs, f, 0.5411961);
                x.hp[1] = x.hp[3] = B::make (B::hp, fs, f, 1.3065630);
                x.ap[0] = B::make (B::ap, fs, f, 0.5411961);
                x.ap[1] = B::make (B::ap, fs, f, 1.3065630);
                break;
        }
    }
}

void Engine::controlBlock()
{
    const double crate = fs / CR;
    const auto& PI = params();

    // ---- livelli di trigger
    for (int i = 0; i < 8; ++i)
    {
        trigDb[(size_t) i] = gainToDb (std::max (trigPeak[(size_t) i], 1.0e-6));
        view[(size_t) (VIEW::TRIG + i)].store ((float) trigDb[(size_t) i], std::memory_order_relaxed);
        trigPeak[(size_t) i] = 0;
    }

    // ---- valori grezzi: servono per le sorgenti prima della modulazione (uso dei valori dell'ultimo blocco)
    // ---- XLFO
    for (int k = 0; k < NXLFO; ++k)
    {
        const int xb = CFG::XLFO_BASE + k * CFG::XLFO_STRIDE;
        auto& L = lfo[(size_t) k];
        double out = 0;
        if (cfg[(size_t) xb] > 0.5f)
        {
            const int ns = (int) clampd (std::floor (cfg[(size_t) xb + 1]), 1, 16);
            const bool sync = cfg[(size_t) xb + 2] > 0.5f;
            const double rt = cfg[(size_t) xb + 3];
            const bool snap = cfg[(size_t) xb + 4] > 0.5f;
            const int pb = P::x1_rate + k * 6;
            const double rate = pv[(size_t) pb], div = pv[(size_t) pb + 1], ofs = pv[(size_t) pb + 2];
            const double bal = pv[(size_t) pb + 3] * 0.01, gl = pv[(size_t) pb + 4] * 0.01, pho = pv[(size_t) pb + 5] * 0.01;
            const double curT = rt > 1.5 ? midiLTrig : midiTrig;
            const bool trg = rt > 0.5 && curT != L.lastTrig;
            L.lastTrig = curT;
            double ph;
            if (sync)
            {
                const double bars = std::pow (2.0, 4 - std::floor (div));
                const double cyc = bars * 4 / std::max (ofs, 0.01);
                if (trg) L.syncOfs = -wrap01 (beatPos / cyc);
                ph = wrap01 (beatPos / cyc + L.syncOfs);
            }
            else
            {
                if (trg) L.acc = 0;
                L.acc = wrap01 (L.acc + rate / crate);
                ph = L.acc;
            }
            ph = wrap01 (ph + pho);
            const double h = 0.5 + 0.45 * bal;
            const double wp = ph < h ? ph * 0.5 / h : 0.5 + (ph - h) * 0.5 / (1 - h);
            const double pos = wp * ns;
            const int si = std::min ((int) std::floor (pos), ns - 1);
            const double fr = pos - si;
            if (si != L.lastStep)
            {
                L.lastStep = si;
                if (cfg[(size_t) (xb + 8 + si * 4 + 1)] > 0.5f) L.rnd[si] = noise();
            }
            const int sb = xb + 8 + si * 4;
            const int nsi = si + 1 >= ns ? 0 : si + 1;
            const int nbx = xb + 8 + nsi * 4;
            double v0 = cfg[(size_t) sb] , v1 = cfg[(size_t) nbx];
            if (cfg[(size_t) sb + 1] > 0.5f) v0 = L.rnd[si];
            if (cfg[(size_t) nbx + 1] > 0.5f) v1 = L.rnd[nsi];
            if (snap) { v0 = std::floor (v0 * 12 + 0.5) / 12; v1 = std::floor (v1 * 12 + 0.5) / 12; }
            const double g = clampd (gl + cfg[(size_t) sb + 3], 0, 1);
            double t = 0;
            if (g > 0.0001) t = clampd ((fr - (1 - g)) / g, 0, 1);
            const double cv = cfg[(size_t) sb + 2];
            double ct = t;
            if (cv > 0.5 && cv < 1.5) ct = t * t;
            else if (cv > 1.5 && cv < 2.5) ct = std::sqrt (t);
            else if (cv > 2.5) ct = 0.5 - 0.5 * std::cos (kPi * t);
            out = v0 + (v1 - v0) * ct;
            if (snap) out *= 0.10034;   // 1.0 = un'ottava sulla frequenza di feedback
            view[(size_t) (VIEW::LFOPH + k)].store ((float) wp, std::memory_order_relaxed);
            view[(size_t) (VIEW::LFOSTEP + k)].store ((float) si, std::memory_order_relaxed);
        }
        srcv[(size_t) (SRC::xlfo1 + k)] = out;
    }

    // ---- Envelope generator
    for (int k = 0; k < NEG; ++k)
    {
        const int b = CFG::EG_BASE + k * CFG::EG_STRIDE;
        auto& E = eg[(size_t) k];
        double out = 0;
        if (cfg[(size_t) b] > 0.5f)
        {
            const double trig = cfg[(size_t) b + 1], range = cfg[(size_t) b + 2];
            const double as = cfg[(size_t) b + 3], ds = cfg[(size_t) b + 4], rs = cfg[(size_t) b + 5];
            const int pb = P::e1_thr + k * 7;
            const double thr = pv[(size_t) pb], del = pv[(size_t) pb + 1], att = pv[(size_t) pb + 2], dec = pv[(size_t) pb + 3];
            const double sus = pv[(size_t) pb + 4] * 0.01, hold = pv[(size_t) pb + 5], rel = pv[(size_t) pb + 6];
            double gate = 0;
            bool fresh = false;
            if (trig > 7.5)
            {
                gate = midiGate > 0.5 ? 1 : 0;
                if (midiTrig != E.lastTrig) { E.lastTrig = midiTrig; fresh = true; }
            }
            else
            {
                const double ldb = trigDb[(size_t) clampd (std::floor (trig), 0, 7)];
                gate = E.gate > 0.5 ? (ldb > thr - 3 ? 1 : 0) : (ldb > thr ? 1 : 0);
            }
            if ((gate > 0.5 && E.gate < 0.5) || fresh) { E.stage = 1; E.t = 0; E.start = E.lvl; }
            if (gate < 0.5 && E.gate > 0.5 && E.stage >= 1 && E.stage <= 4) { E.stage = 5; E.t = 0; E.start = E.lvl; }
            E.t += 1000.0 / crate;
            if (E.stage == 1 && E.t >= del) { E.stage = 2; E.t = 0; }
            if (E.stage == 2)
            {
                const double pr = att > 0.01 ? std::min (E.t / att, 1.0) : 1.0;
                E.lvl = E.start + (1 - E.start) * std::pow (pr, std::pow (4.0, -as));
                if (pr >= 1) { E.stage = 3; E.t = 0; }
            }
            if (E.stage == 3)
            {
                const double pr = dec > 0.01 ? std::min (E.t / dec, 1.0) : 1.0;
                E.lvl = 1 + (sus - 1) * std::pow (pr, std::pow (4.0, -ds));
                if (pr >= 1) E.stage = 4;
            }
            if (E.stage == 4) E.lvl = sus;
            if (E.stage == 5 && E.t >= hold) { E.stage = 6; E.t = 0; E.start = E.lvl; }
            if (E.stage == 6)
            {
                const double pr = rel > 0.01 ? std::min (E.t / rel, 1.0) : 1.0;
                E.lvl = E.start * (1 - std::pow (pr, std::pow (4.0, -rs)));
                if (pr >= 1) { E.stage = 0; E.lvl = 0; }
            }
            E.gate = gate;
            out = range > 0.5 ? E.lvl - sus : E.lvl;
        }
        view[(size_t) (VIEW::EGSTAGE + k)].store ((float) E.stage, std::memory_order_relaxed);
        srcv[(size_t) (SRC::eg1 + k)] = out;
    }

    // ---- Envelope follower
    for (int k = 0; k < NEF; ++k)
    {
        const int b = CFG::EF_BASE + k * CFG::EF_STRIDE;
        auto& F = ef[(size_t) k];
        double out = 0;
        if (cfg[(size_t) b] > 0.5f)
        {
            const int trig = (int) clampd (std::floor (cfg[(size_t) b + 1]), 0, 7);
            const bool trans = cfg[(size_t) b + 2] > 0.5f;
            const int pb = P::f1_att + k * 2;
            const double ca = std::exp (-1.0 / (std::max (pv[(size_t) pb], 0.05) * 0.001 * crate));
            const double cr = std::exp (-1.0 / (std::max (pv[(size_t) pb + 1], 0.5) * 0.001 * crate));
            const double lin = dbToGain (trigDb[(size_t) trig]);
            if (trans)
            {
                const double k1 = std::exp (-1.0 / (0.0005 * crate)), k2 = std::exp (-1.0 / (0.03 * crate)), k3 = std::exp (-1.0 / (0.3 * crate));
                F.fast = lin > F.fast ? lin + (F.fast - lin) * k1 : lin + (F.fast - lin) * k2;
                F.slow = lin > F.slow ? lin + (F.slow - lin) * k2 : lin + (F.slow - lin) * k3;
                const double tr = clampd ((gainToDb (std::max (F.fast, 1e-6)) - gainToDb (std::max (F.slow, 1e-6))) / 18.0, 0, 1);
                F.env = tr > F.env ? tr + (F.env - tr) * ca : tr + (F.env - tr) * cr;
                out = F.env;
            }
            else
            {
                if (! F.init) { F.env = -90; F.init = true; }
                const double ldb = std::max (gainToDb (std::max (lin, 1e-6)), -90.0);
                F.env = ldb > F.env ? ldb + (F.env - ldb) * ca : ldb + (F.env - ldb) * cr;
                out = clampd ((F.env + 60) / 60, 0, 1);
            }
        }
        else F.init = false;
        srcv[(size_t) (SRC::ef1 + k)] = out;
    }

    // ---- MIDI e XY
    for (int k = 0; k < NMIDI; ++k) srcv[(size_t) (SRC::midi1 + k)] = sourceMidi (k);
    for (int k = 0; k < NXY; ++k)
    {
        const int b = CFG::XY_BASE + k * CFG::XY_STRIDE;
        double x = pv[(size_t) (P::xy1_x + k * 2)] * 0.01, y = pv[(size_t) (P::xy1_y + k * 2)] * 0.01;
        if (cfg[(size_t) b + 2] > 0.5f) { x = (x + 1) * 0.5; y = (y + 1) * 0.5; }
        if (cfg[(size_t) b] < 0.5f) x = y = 0;
        srcv[(size_t) (SRC::xy1_x + k * 2)] = x;
        srcv[(size_t) (SRC::xy1_y + k * 2)] = y;
    }
    srcv[0] = 0;
    for (int i = 0; i < NSRC; ++i) view[(size_t) (VIEW::SRC + i)].store ((float) srcv[(size_t) i], std::memory_order_relaxed);

    // ---- slot di modulazione (prima gli slot che modulano altri slot)
    std::fill (msum.begin(), msum.end(), 0.0);
    for (int s = 0; s < NSLOTS; ++s)
    {
        const int sb = CFG::SLOT_BASE + s * CFG::SLOT_STRIDE;
        if (cfg[(size_t) sb] > 0.5f)
        {
            const int tg = (int) cfg[(size_t) sb + 2];
            if (tg >= SLOT0 && tg < NT)
            {
                double am = cfg[(size_t) sb + 3];
                if (cfg[(size_t) sb + 4] > 0.5f) am = -am;
                msum[(size_t) tg] += srcv[(size_t) clampd (cfg[(size_t) sb + 1], 0, NSRC - 1)] * am;
            }
        }
    }
    for (int s = 0; s < NSLOTS; ++s)
    {
        const int sb = CFG::SLOT_BASE + s * CFG::SLOT_STRIDE;
        double amf = 0;
        if (cfg[(size_t) sb] > 0.5f)
        {
            const int tg = (int) cfg[(size_t) sb + 2];
            amf = clampd (cfg[(size_t) sb + 3] + msum[(size_t) (SLOT0 + s)], -1, 1);
            if (cfg[(size_t) sb + 4] > 0.5f) amf = -amf;
            if (tg >= 0 && tg < SLOT0) msum[(size_t) tg] += srcv[(size_t) clampd (cfg[(size_t) sb + 1], 0, NSRC - 1)] * amf;
        }
        view[(size_t) (SLOT0 + s)].store ((float) amf, std::memory_order_relaxed);
    }

    // ---- valori effettivi dei parametri
    for (int p = 0; p < NP; ++p)
    {
        const auto& I = PI[(size_t) p];
        double v = praw[(size_t) p];
        if (I.target >= 0)
        {
            const double n = clampd (normOf (I, v) + msum[(size_t) I.target], 0, 1);
            view[(size_t) I.target].store ((float) n, std::memory_order_relaxed);
            v = denormOf (I, n);
        }
        pv[(size_t) p] = v;
    }

    // ---- struttura (fase lineare / qualità): dissolvenza prima del cambio
    const bool wantLin = pv[(size_t) P::linphase] > 0.5;
    const int wantOs = pv[(size_t) P::hq] < 0.5 ? 0 : (pv[(size_t) P::hq] < 1.5 ? 3 : 5);
    if (wantLin != linPhase || wantOs != osLog2) structTarget = 0;
    msMode = pv[(size_t) P::msmode] > 0.5;
    unrestricted = pv[(size_t) P::unrestricted] > 0.5;

    // ---- globali
    const double gi = dbToGain (pv[(size_t) P::gain_in]), pi = pv[(size_t) P::pan_in] * 0.01;
    igain[0].t = gi * std::min (1.0, 1 - pi); igain[1].t = gi * std::min (1.0, 1 + pi);
    const double go = dbToGain (pv[(size_t) P::gain_out]), po = pv[(size_t) P::pan_out] * 0.01;
    ogain[0].t = go * std::min (1.0, 1 - po); ogain[1].t = go * std::min (1.0, 1 + po);
    gmix.t = pv[(size_t) P::gmix] * 0.01;
    byp.t = pv[(size_t) P::bypass] > 0.5 ? 1 : 0;
    nb = (int) clampd (std::floor (pv[(size_t) P::nbands]), 1, MB);

    // ---- crossover
    designCrossoversIir();
    if (linPhase)
    {
        int sl[MB - 1];
        for (int i = 0; i < nb - 1; ++i) sl[i] = xo[(size_t) i].slope;
        lps.setCrossovers (nb, xfreq.data(), sl);
    }

    // ---- bande
    double anySolo = 0;
    for (int b = 0; b < nb; ++b) anySolo += pv[(size_t) bandParam (b, BP::solo)];
    const auto& ST = styles();
    using B = BiquadCoefs;
    for (int b = 0; b < nb; ++b)
    {
        const auto bp = [&] (int off) { return pv[(size_t) bandParam (b, off)]; };
        const auto& S = ST[(size_t) clampd (std::floor (bp (BP::style)), 0, NSTYLES - 1)];
        auto& C = bc[(size_t) b];
        C.sht = S.shtype; C.bias = S.bias; C.special = S.special; C.inner = S.inner;
        const double dr = bp (BP::drive), dp = bp (BP::dpan) * 0.01;
        const double drv[2] = { dr * std::min (1.0, 1 - dp), dr * std::min (1.0, 1 + dp) };
        for (int c = 0; c < 2; ++c)
        {
            const double g = dbToGain (drv[c] * S.dscale + S.dofs);
            double comp = 0.25 / std::max (std::abs (shapeFn (0.25 * g, S.shtype, S.bias)), 0.0001);
            if (S.shtype > 6.5 && S.shtype < 7.5) comp = 0.7;
            if (S.shtype > 7.5 && S.shtype < 8.5) comp *= 1.6;
            C.comp[c] = clampd (comp, 0.05, 1.2);
            bsm[(size_t) b].gain[c].t = g;
        }
        C.dnorm = dr / 48.0;
        C.dyn = bp (BP::dyn) * 0.01;
        auto& sm = bsm[(size_t) b];
        sm.mix.t = bp (BP::mix) * 0.01;
        const double lv = bp (BP::level);
        const double lg = lv <= -59.9 ? 0.0 : dbToGain (lv);
        const double lp = bp (BP::lpan) * 0.01;
        sm.lvl[0].t = lg * std::min (1.0, 1 - lp);
        sm.lvl[1].t = lg * std::min (1.0, 1 + lp);
        sm.aud.t = anySolo > 0.5 ? bp (BP::solo) : 1 - bp (BP::mute);
        sm.fba.t = bp (BP::fba) * 0.0095;
        sm.en.t = bp (BP::en);
        C.prehp = S.prehp > 1 ? B::make (B::hp, fs, S.prehp) : B::identity();
        C.preeq = std::abs (S.preeqG) > 0.01 ? B::make (B::peak, fs, S.preeqF, S.preeqQ, S.preeqG) : B::identity();
        C.posteq = std::abs (S.posteqG) > 0.01 ? B::make (B::peak, fs, S.posteqF, S.posteqQ, S.posteqG) : B::identity();
        C.postlp = S.postlp < 19999 ? B::make (B::lp, fs, S.postlp) : B::identity();
        C.cab = std::abs (S.cabG) > 0.01 ? B::make (B::peak, fs, S.cabF, 1.2, S.cabG) : B::identity();
        C.tone[0] = B::make (B::lowshelf, fs, 120, 0.7071, bp (BP::bass));
        C.tone[1] = B::make (B::peak, fs, 800, 0.6, bp (BP::mid));
        C.tone[2] = B::make (B::highshelf, fs, 5000, 0.7071, bp (BP::treb));
        C.tone[3] = B::make (B::peak, fs, 2800, 1.4, bp (BP::pres));
        C.fb = B::make (B::bp, fs, bp (BP::fbf), 6.0);
    }

    // ---- meter e informazioni
    for (int i = 0; i < 4; ++i) { view[(size_t) (VIEW::METER + i)].store ((float) gainToDb (std::max (meters[(size_t) i], 1e-6)), std::memory_order_relaxed); meters[(size_t) i] = 0; }
    for (int i = 0; i < MB; ++i) { view[(size_t) (VIEW::BANDOUT + i)].store ((float) gainToDb (std::max (meters[(size_t) (4 + i)], 1e-6)), std::memory_order_relaxed); meters[(size_t) (4 + i)] = 0; }
    view[(size_t) VIEW::LATENCY].store ((float) curLatency, std::memory_order_relaxed);
    view[(size_t) VIEW::LATENCY + 1].store ((float) fs, std::memory_order_relaxed);
}

void Engine::process (float* Lp, float* Rp, int nch, const float* scL, const float* scR, int n,
                      const MidiEvent* ev, int nev, const Transport& tp)
{
    ScopedFlushDenormals noDenormals;
    if (tp.valid && tp.playing) beatPos = tp.ppq;
    beatInc = (tp.valid ? tp.bpm : 120.0) / 60.0 / fs;
    int ei = 0;
    const int aud = audition.load (std::memory_order_relaxed);
    const double kSm = kFast;

    for (int i = 0; i < n; ++i)
    {
        while (ei < nev && ev[ei].pos <= i) handleMidi (ev[ei++]);
        if (ctl == 0) controlBlock();
        if (++ctl >= CR) ctl = 0;
        beatPos += beatInc;

        // ---- dissolvenza dei cambi strutturali
        if (structTarget < 0.5)
        {
            structFade -= 1.0 / (0.005 * fs);
            if (structFade <= 0)
            {
                structFade = 0;
                linPhase = pv[(size_t) P::linphase] > 0.5;
                osLog2 = pv[(size_t) P::hq] < 0.5 ? 0 : (pv[(size_t) P::hq] < 1.5 ? 3 : 5);
                updateStructure();
                for (auto& b : bch) for (auto& c : b) { c.os.reset(); c.osDry.reset(); c.dryDelay.reset(); }
                if (linPhase)
                {
                    int sl[MB - 1];
                    for (int x = 0; x < nb - 1; ++x) sl[x] = xo[(size_t) x].slope;
                    lps.setCrossovers (nb, xfreq.data(), sl);
                }
                structTarget = 1;
            }
        }
        else if (structFade < 1) structFade = std::min (1.0, structFade + 1.0 / (0.01 * fs));

        const double inL = Lp[i], inR = nch > 1 ? Rp[i] : Lp[i];
        const double sL = scL ? scL[i] : 0.0, sR = scR ? scR[i] : (scL ? scL[i] : 0.0);
        const double ig0 = igain[0].next (kSm), ig1 = igain[1].next (kSm);
        double xL = inL * ig0, xR = inR * ig1;
        meters[0] = std::max (meters[0], std::abs (inL));
        meters[1] = std::max (meters[1], std::abs (inR));
        trigPeak[0] = std::max (trigPeak[0], std::max (std::abs (xL), std::abs (xR)));
        trigPeak[1] = std::max (trigPeak[1], std::max (std::abs (sL), std::abs (sR)));
        ienv = std::max (std::max (std::abs (xL), std::abs (xR)), ienv * 0.9997);
        fbScale.t = (unrestricted || ienv > 0.0001) ? 1 : 0;
        const double fbs = fbScale.next (0.0003);
        if (msMode) { const double m = (xL + xR) * 0.5, s = (xL - xR) * 0.5; xL = m; xR = s; }

        // ---- divisione in bande
        double band[MB][2] {};
        if (linPhase)
        {
            std::complex<double> o[MB];
            for (auto& v : o) v = 0;
            lps.process (xL, xR, o);
            for (int b = 0; b < nb; ++b) { band[b][0] = o[b].real(); band[b][1] = o[b].imag(); }
        }
        else
        {
            for (int c = 0; c < 2; ++c)
            {
                double rest = c == 0 ? xL : xR;
                for (int x = 0; x < nb - 1; ++x)
                {
                    auto& X = xo[(size_t) x];
                    auto& S = xoSt[(size_t) c][(size_t) x];
                    double lpv = rest;
                    for (int q = 0; q < 4; ++q) lpv = S.lp[q].process (X.lp[q], lpv);
                    double hpv;
                    if (X.slope == 0) hpv = rest - lpv;
                    else
                    {
                        hpv = rest;
                        for (int q = 0; q < 4; ++q) hpv = S.hp[q].process (X.hp[q], hpv);
                        if (X.slope == 1) hpv = -hpv;
                    }
                    band[x][c] = lpv;
                    rest = hpv;
                }
                band[nb - 1][c] = rest;
                // compensazione di fase: ogni banda attraversa gli allpass dei crossover superiori
                for (int b = 0; b < nb - 1; ++b)
                    for (int j = b + 1; j < nb - 1; ++j)
                    {
                        auto& A = apSt[(size_t) c][(size_t) b][(size_t) j];
                        band[b][c] = A[0].process (xo[(size_t) j].ap[0], band[b][c]);
                        band[b][c] = A[1].process (xo[(size_t) j].ap[1], band[b][c]);
                    }
            }
        }

        // ---- elaborazione delle bande
        double sum[2] { 0, 0 }, drySum[2] { 0, 0 };
        const bool gmixNeed = gmix.v < 0.99999 || gmix.t < 0.99999;
        const int F = 1 << osLog2;
        const int smw = smudgePos;
        smudgePos = (smudgePos + 1) & 8191;
        for (int b = 0; b < nb; ++b)
        {
            auto& C = bc[(size_t) b];
            auto& sm = bsm[(size_t) b];
            const double fba = sm.fba.next (kSm), mix = sm.mix.next (kSm), en = sm.en.next (kSm), audible = sm.aud.next (kSm);
            // il ramo asciutto sovracampionato (IIR) serve solo se qualcosa lo miscela
            if (mix < 0.99999 || sm.mix.t < 0.99999 || en < 0.99999 || sm.en.t < 0.99999 || gmixNeed) dryHold[(size_t) b] = (int) (fs * 0.5);
            else if (dryHold[(size_t) b] > 0) --dryHold[(size_t) b];
            const bool needDry = dryHold[(size_t) b] > 0;
            for (int c = 0; c < 2; ++c)
            {
                auto& S = bch[(size_t) b][(size_t) c];
                const double xb0 = band[b][c];
                trigPeak[(size_t) (2 + b)] = std::max (trigPeak[(size_t) (2 + b)], std::abs (xb0));
                const double gv = sm.gain[c].next (kSm);
                const double lvl = sm.lvl[c].next (kSm);
                // feedback
                const double fbv = S.fb.process (C.fb, S.fbLast);
                double u = xb0 + fbv * fba * fbs;
                u = S.prehp.process (C.prehp, u);
                u = S.preeq.process (C.preeq, u);
                if (C.special > 0.5 && C.special < 1.5)
                {
                    // Smudge: ritardo dipendente dal segnale
                    const double d = 1 + (0.5 + 0.5 * std::tanh (u * 3)) * C.dnorm * 0.02 * fs;
                    double r = smw - d;
                    while (r < 0) r += 8192;
                    const int ri = (int) r;
                    const double fr = r - ri;
                    const double v = S.smudge[(size_t) ri] + (S.smudge[(size_t) ((ri + 1) & 8191)] - S.smudge[(size_t) ri]) * fr;
                    S.smudge[(size_t) smw] = u;
                    u = v * (0.4 + 0.5 * C.dnorm) + u * (0.6 - 0.5 * C.dnorm);
                }
                else if (C.special > 1.5 && C.special < 2.5)
                {
                    // Breakdown: sottottava a flip-flop sugli attraversamenti dello zero
                    if (S.bdPrev < 0 && u >= 0) S.bdFlip = -S.bdFlip;
                    S.bdPrev = u;
                    u = u + (u * S.bdFlip - u) * (0.6 + 0.4 * C.dnorm);
                }
                else if (C.special > 2.5)
                {
                    // Destroy: riduzione di bit e di frequenza di campionamento
                    S.dCnt += 1;
                    if (S.dCnt >= 1 + std::floor (C.dnorm * C.dnorm * 40.0 * fs / 48000.0))
                    {
                        S.dCnt = 0;
                        const double q = std::pow (2.0, 15 - C.dnorm * 13);
                        S.dHold = std::floor (u * q + 0.5) / q;
                    }
                    u = S.dHold;
                }
                // saturazione (con oversampling)
                const double xg = u * gv;
                double y;
                if (F == 1)
                {
                    const double ys = shapeFn (xg, C.sht, C.bias) * C.comp[c];
                    y = u + (ys - u) * C.inner;
                }
                else
                {
                    S.os.up (xg, osBuf);
                    const double ig = 1.0 / gv;
                    for (int k = 0; k < F; ++k)
                    {
                        const double uo = osBuf[k] * ig;
                        const double ys = shapeFn (osBuf[k], C.sht, C.bias) * C.comp[c];
                        osBuf[k] = uo + (ys - uo) * C.inner;
                    }
                    y = S.os.down (osBuf);
                }
                // segnale asciutto allineato al percorso sovracampionato
                double dry;
                if (F == 1) dry = xb0;
                else if (linPhase) dry = S.dryDelay.process (xb0, S.os.latency());
                else if (needDry)
                {
                    if (! S.dryActive) { S.osDry.reset(); S.dryActive = true; }
                    S.osDry.up (xb0, osDryBuf);
                    dry = S.osDry.down (osDryBuf);
                }
                else { S.dryActive = false; dry = 0; }
                y = S.posteq.process (C.posteq, y);
                y = S.postlp.process (C.postlp, y);
                y = S.cab.process (C.cab, y);
                const double dcy = y - S.dcx + 0.9995 * S.dcy;
                S.dcx = y; S.dcy = dcy; y = dcy;
                for (int q = 0; q < 4; ++q) y = S.tone[q].process (C.tone[q], y);
                S.fbLast = clampd (y, -4, 4);
                if (! std::isfinite (S.fbLast)) S.fbLast = 0;
                // dinamica: compressione (a destra) o gate/espansione (a sinistra)
                const double ay = std::abs (y);
                S.env += (ay - S.env) * (ay > S.env ? kDyn[0] : kDyn[1]);
                double dg = 1;
                if (C.dyn > 0.001)
                {
                    const double edb = gainToDb (std::max (S.env, 1e-6));
                    const double thr = -6 - 24 * C.dyn, rat = 1 + 9 * C.dyn;
                    const double ov = edb - thr;
                    const double gr = ov > 0 ? -ov * (1 - 1 / rat) : 0;
                    dg = dbToGain (gr - thr * (1 - 1 / rat) * 0.5);
                }
                else if (C.dyn < -0.001)
                {
                    const double edb = gainToDb (std::max (S.env, 1e-6));
                    const double thr = -70 - 50 * C.dyn, rat = 1 - 7 * C.dyn;
                    const double un = thr - edb;
                    dg = dbToGain (un > 0 ? std::max (-un * (rat - 1), -90.0) : 0.0);
                }
                y *= dg;
                y = dry + (y - dry) * mix;
                y = dry + (y - dry) * en;
                y *= lvl * audible;
                meters[(size_t) (4 + b)] = std::max (meters[(size_t) (4 + b)], std::abs (y));
                sum[c] += y;
                drySum[c] += dry;
            }
        }

        // ---- somma, M/S, uscita, mix globale, bypass
        if (msMode)
        {
            double t0 = sum[0] + sum[1], t1 = sum[0] - sum[1];
            sum[0] = t0; sum[1] = t1;
            t0 = drySum[0] + drySum[1]; t1 = drySum[0] - drySum[1];
            drySum[0] = t0; drySum[1] = t1;
        }
        const double og0 = ogain[0].next (kSm), og1 = ogain[1].next (kSm);
        const double gm = gmix.next (kSm);
        const double d0 = drySum[0] / std::max (ig0, 1e-4), d1 = drySum[1] / std::max (ig1, 1e-4);
        double w0 = sum[0] * og0, w1 = sum[1] * og1;
        w0 = d0 + (w0 - d0) * gm;
        w1 = d1 + (w1 - d1) * gm;
        w0 *= structFade; w1 *= structFade;
        const double bv = byp.next (kByp);
        const double rL = bypassDelayL.process (inL, curLatency), rR = bypassDelayR.process (inR, curLatency);
        double oL = w0 + (rL - w0) * bv, oR = w1 + (rR - w1) * bv;
        if (aud == 1) { oL = xL; oR = xR; }
        else if (aud == 2) { oL = sL; oR = sR; }
        else if (aud >= 3 && aud - 3 < nb) { oL = band[aud - 3][0]; oR = band[aud - 3][1]; }
        if (! std::isfinite (oL)) oL = 0;
        if (! std::isfinite (oR)) oR = 0;
        meters[2] = std::max (meters[2], std::abs (oL));
        meters[3] = std::max (meters[3], std::abs (oR));
        const int sp = specPos.load (std::memory_order_relaxed);
        specRing[(size_t) sp] = (float) ((oL + oR) * 0.5);
        specPos.store ((sp + 1) & (SPEC_SIZE - 1), std::memory_order_release);
        if (nch > 1) { Lp[i] = (float) oL; Rp[i] = (float) oR; }
        else Lp[i] = (float) ((oL + oR) * 0.5);
    }
}
} // namespace sdsp
