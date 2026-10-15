// SATURA - oversampling a più stadi 2x con filtri half-band
//  - modalità IIR: polifase ad allpass (fase quasi lineare in banda, nessuna latenza intera)
//  - modalità FIR: half-band a fase lineare (Linear Phase), latenza intera compensabile
#pragma once
#include "Basics.h"
#include "OsCoefs.h"
#include <array>

namespace sdsp
{
// ---- stadio polifase IIR: due rami di allpass del primo ordine (al rate basso)
class HalfbandIir
{
public:
    void setCoefs (const double* c, int n)
    {
        na = nb = 0;
        for (int i = 0; i < n; ++i)
        {
            if ((i & 1) == 0) ca[na++] = c[i];
            else cb[nb++] = c[i];
        }
        reset();
    }
    void reset()
    {
        for (auto* s : { &ua, &ub, &da, &db }) for (auto& v : *s) v = { 0, 0 };
    }
    // 1 campione -> 2 campioni
    inline void up (double x, double* out)
    {
        out[0] = chain (x, ca, na, ua);
        out[1] = chain (x, cb, nb, ub);
    }
    // 2 campioni -> 1 campione
    inline double down (const double* in)
    {
        const double a = chain (in[0], ca, na, da);
        const double b = chain (prevOdd, cb, nb, db);
        prevOdd = in[1];
        return 0.5 * (a + b);
    }

private:
    struct St { double x1, y1; };
    static inline double chain (double x, const double* c, int n, std::array<St, 8>& st)
    {
        for (int i = 0; i < n; ++i)
        {
            const double y = c[i] * (x - st[(size_t) i].y1) + st[(size_t) i].x1;
            st[(size_t) i].x1 = x;
            st[(size_t) i].y1 = y;
            x = y;
        }
        return x;
    }
    double ca[8] {}, cb[8] {};
    int na = 0, nb = 0;
    std::array<St, 8> ua {}, ub {}, da {}, db {};
    double prevOdd = 0;
};

// ---- stadio FIR half-band a fase lineare
class HalfbandFir
{
public:
    void setCoefs (const double* evenTaps, int totalLen)
    {
        len = totalLen;
        c = (len - 1) / 2;
        m = (c - 1) / 2;
        nt = c + 1;                 // tap pari non nulli (j = 0, 2, ..., len-1)
        taps.assign (evenTaps, evenTaps + nt);
        nu = nt + 1;
        nd = len + 2;
        hu.assign ((size_t) (2 * nu), 0.0);
        hd.assign ((size_t) (2 * nd), 0.0);
        pu = pd = 0;
        reset();
    }
    void reset() { std::fill (hu.begin(), hu.end(), 0.0); std::fill (hd.begin(), hd.end(), 0.0); pu = pd = 0; }
    int latencyHigh() const { return c; }   // in campioni al rate alto, per up o per down

    // buffer circolari scritti due volte: h[k] = campione di k passi fa, senza spostamenti
    inline void up (double x, double* out)
    {
        pu = pu == 0 ? nu - 1 : pu - 1;
        hu[(size_t) pu] = hu[(size_t) (pu + nu)] = x;
        const double* h = hu.data() + pu;
        double acc = 0;
        for (int i = 0; i < nt; ++i) acc += taps[(size_t) i] * h[i];   // tap j = 2i -> x[n - i]
        out[0] = 2.0 * acc;
        out[1] = h[m];
    }
    inline double down (const double* in)
    {
        pd = pd == 0 ? nd - 1 : pd - 1;
        hd[(size_t) pd] = hd[(size_t) (pd + nd)] = in[0];
        pd = pd == 0 ? nd - 1 : pd - 1;
        hd[(size_t) pd] = hd[(size_t) (pd + nd)] = in[1];
        const double* hdp = hd.data() + pd;
        // decimazione sulla fase pari: y = sum_j h[j] v[2n - j], con v[2n] = hd[1]
        double acc = 0;
        for (int i = 0; i < nt; ++i) acc += taps[(size_t) i] * hdp[2 * i + 1];
        return acc + 0.5 * hdp[c + 1];
    }

private:
    int len = 3, c = 1, m = 0, nt = 2;
    std::vector<double> taps, hu, hd;
    int nu = 1, nd = 1, pu = 0, pd = 0;
};

// ---- catena: fattore 1, 8 o 32
class Oversampler
{
public:
    enum Mode { iir, fir };

    void prepare (int factorLog2, Mode m)
    {
        stages = factorLog2;
        mode = m;
        const double* iirC[5] = { oscoefs::iir0, oscoefs::iir1, oscoefs::iir2, oscoefs::iir3, oscoefs::iir4 };
        const int iirN[5] = { oscoefs::iirN0, oscoefs::iirN1, oscoefs::iirN2, oscoefs::iirN3, oscoefs::iirN4 };
        const double* firC[5] = { oscoefs::fir0, oscoefs::fir1, oscoefs::fir2, oscoefs::fir3, oscoefs::fir4 };
        const int firN[5] = { oscoefs::firN0, oscoefs::firN1, oscoefs::firN2, oscoefs::firN3, oscoefs::firN4 };
        double lat = 0;
        for (int s = 0; s < 5; ++s)
        {
            iirSt[(size_t) s].setCoefs (iirC[s], iirN[s]);
            firSt[(size_t) s].setCoefs (firC[s], firN[s]);
            if (s < stages) lat += 2.0 * firSt[(size_t) s].latencyHigh() / (double) (1 << (s + 1));
        }
        // ritardo aggiuntivo al rate più alto per rendere intera la latenza (solo FIR)
        padHigh = 0;
        latencyBase = 0;
        if (mode == fir && stages > 0)
        {
            const int F = 1 << stages;
            const double frac = std::ceil (lat - 1e-9) - lat;
            padHigh = (int) std::lround (frac * F);
            latencyBase = (int) std::lround (lat + (double) padHigh / F);
        }
        pad.prepare (padHigh + 1);
        reset();
    }
    void reset()
    {
        for (auto& s : iirSt) s.reset();
        for (auto& s : firSt) s.reset();
        pad.reset();
    }
    int factor() const { return 1 << stages; }
    int latency() const { return latencyBase; }   // solo in modalità FIR (in IIR non c'è latenza intera)

    // x -> buf[factor]
    inline void up (double x, double* buf)
    {
        buf[0] = x;
        int n = 1;
        for (int s = 0; s < stages; ++s)
        {
            for (int i = 0; i < n; ++i)   // in ordine temporale
            {
                double o[2];
                if (mode == iir) iirSt[(size_t) s].up (buf[i], o); else firSt[(size_t) s].up (buf[i], o);
                tmp[2 * i] = o[0];
                tmp[2 * i + 1] = o[1];
            }
            n *= 2;
            for (int i = 0; i < n; ++i) buf[i] = tmp[i];
        }
        if (mode == fir && padHigh > 0)
            for (int i = 0; i < n; ++i) buf[i] = pad.process (buf[i], padHigh);
    }
    // buf[factor] -> y
    inline double down (double* buf)
    {
        int n = 1 << stages;
        for (int s = stages - 1; s >= 0; --s)
        {
            n /= 2;
            for (int i = 0; i < n; ++i)
                buf[i] = mode == iir ? iirSt[(size_t) s].down (buf + 2 * i) : firSt[(size_t) s].down (buf + 2 * i);
        }
        return buf[0];
    }

private:
    int stages = 0, padHigh = 0, latencyBase = 0;
    Mode mode = iir;
    std::array<HalfbandIir, 5> iirSt;
    std::array<HalfbandFir, 5> firSt;
    DelayLine pad;
    double tmp[64] {};
};
} // namespace sdsp
