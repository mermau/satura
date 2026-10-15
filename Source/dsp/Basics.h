// SATURA - blocchi DSP di base (senza JUCE): FFT, biquad, progetto dei filtri
#pragma once
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#if defined(__SSE2__) || defined(_M_X64)
 #include <xmmintrin.h>
 #define SDSP_SSE 1
#endif

namespace sdsp
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

inline double clampd (double x, double a, double b) { return x < a ? a : (x > b ? b : x); }
inline double dbToGain (double db) { return std::pow (10.0, db / 20.0); }
inline double gainToDb (double g) { return 20.0 * std::log10 (std::max (g, 1.0e-9)); }
inline double wrap01 (double x) { x -= std::floor (x); return x; }

// azzera i denormali durante l'elaborazione (FTZ/DAZ su x86, FZ su ARM)
struct ScopedFlushDenormals
{
#if SDSP_SSE
    unsigned int old;
    ScopedFlushDenormals() : old (_mm_getcsr()) { _mm_setcsr (old | 0x8040); }
    ~ScopedFlushDenormals() { _mm_setcsr (old); }
#elif defined(__aarch64__)
    unsigned long old;
    ScopedFlushDenormals() { asm volatile ("mrs %0, fpcr" : "=r"(old)); unsigned long v = old | (1ul << 24); asm volatile ("msr fpcr, %0" : : "r"(v)); }
    ~ScopedFlushDenormals() { asm volatile ("msr fpcr, %0" : : "r"(old)); }
#endif
};

// ------------------------------------------------------------------ FFT complessa radix-2
class Fft
{
public:
    void prepare (int n)
    {
        size = n;
        rev.resize ((size_t) n);
        int bits = 0;
        while ((1 << bits) < n) ++bits;
        for (int i = 0; i < n; ++i)
        {
            int r = 0;
            for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            rev[(size_t) i] = r;
        }
        tw.resize ((size_t) n / 2);
        for (int i = 0; i < n / 2; ++i) tw[(size_t) i] = std::polar (1.0, -kTwoPi * i / n);
    }
    int getSize() const { return size; }

    // trasformata in place; inverse = true calcola la IFFT già normalizzata
    void perform (std::complex<double>* d, bool inverse) const
    {
        const int n = size;
        for (int i = 0; i < n; ++i)
            if (i < rev[(size_t) i]) std::swap (d[i], d[rev[(size_t) i]]);
        for (int len = 2; len <= n; len <<= 1)
        {
            const int half = len >> 1, step = n / len;
            for (int i = 0; i < n; i += len)
                for (int k = 0; k < half; ++k)
                {
                    auto w = tw[(size_t) (k * step)];
                    if (inverse) w = std::conj (w);
                    auto v = d[i + k + half] * w;
                    d[i + k + half] = d[i + k] - v;
                    d[i + k] += v;
                }
        }
        if (inverse)
        {
            const double s = 1.0 / n;
            for (int i = 0; i < n; ++i) d[i] *= s;
        }
    }

private:
    int size = 0;
    std::vector<int> rev;
    std::vector<std::complex<double>> tw;
};

// ------------------------------------------------------------------ biquad (forma diretta trasposta II)
struct BiquadCoefs
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

    static BiquadCoefs identity() { return {}; }

    // tipi: lp, hp, bp (guadagno di picco 0 dB), ap, peak, lowshelf, highshelf, lp1, hp1, ap1
    enum Type { lp, hp, bp, ap, peak, lowshelf, highshelf, lp1, hp1, ap1 };

    static BiquadCoefs make (Type t, double fs, double f, double q = 0.7071067811865476, double gainDb = 0.0)
    {
        BiquadCoefs c;
        const double w = kTwoPi * clampd (f, 5.0, fs * 0.49) / fs;
        if (t == lp1 || t == hp1 || t == ap1)
        {
            const double k = std::tan (w * 0.5);
            if (t == lp1) { c.b0 = k / (1 + k); c.b1 = c.b0; c.b2 = 0; c.a1 = (k - 1) / (k + 1); c.a2 = 0; }
            else if (t == hp1) { c.b0 = 1 / (1 + k); c.b1 = -c.b0; c.b2 = 0; c.a1 = (k - 1) / (k + 1); c.a2 = 0; }
            else { c.b0 = (k - 1) / (k + 1); c.b1 = 1; c.b2 = 0; c.a1 = (k - 1) / (k + 1); c.a2 = 0; }
            return c;
        }
        const double cw = std::cos (w), sw = std::sin (w);
        const double al = sw / (2.0 * q);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double sa = 2.0 * std::sqrt (A) * sw * 0.7071067811865476;
        double b0, b1, b2, a0, a1, a2;
        switch (t)
        {
            case lp:  b0 = (1 - cw) * 0.5; b1 = 1 - cw; b2 = b0; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;
            case hp:  b0 = (1 + cw) * 0.5; b1 = -(1 + cw); b2 = b0; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;
            case bp:  b0 = al; b1 = 0; b2 = -al; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;
            case ap:  b0 = 1 - al; b1 = -2 * cw; b2 = 1 + al; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;
            case peak:
                b0 = 1 + al * A; b1 = -2 * cw; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cw; a2 = 1 - al / A; break;
            case lowshelf:
                b0 = A * ((A + 1) - (A - 1) * cw + sa); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - sa);
                a0 = (A + 1) + (A - 1) * cw + sa; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - sa; break;
            case highshelf:
                b0 = A * ((A + 1) + (A - 1) * cw + sa); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - sa);
                a0 = (A + 1) - (A - 1) * cw + sa; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - sa; break;
            case lp1: case hp1: case ap1: default: return c;
        }
        c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0;
        return c;
    }
};

struct BiquadState
{
    double s1 = 0, s2 = 0;
    inline double process (const BiquadCoefs& c, double x)
    {
        const double y = c.b0 * x + s1;
        s1 = c.b1 * x - c.a1 * y + s2;
        s2 = c.b2 * x - c.a2 * y;
        return y;
    }
    void reset() { s1 = s2 = 0; }
};

// ------------------------------------------------------------------ linea di ritardo intera
class DelayLine
{
public:
    void prepare (int maxDelay) { buf.assign ((size_t) std::max (1, maxDelay + 1), 0.0); pos = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0); pos = 0; }
    inline double process (double x, int d)
    {
        buf[(size_t) pos] = x;
        int r = pos - d;
        if (r < 0) r += (int) buf.size();
        const double y = buf[(size_t) r];
        if (++pos >= (int) buf.size()) pos = 0;
        return y;
    }
private:
    std::vector<double> buf;
    int pos = 0;
};
} // namespace sdsp
