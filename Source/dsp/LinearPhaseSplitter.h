// SATURA - divisione in bande a fase lineare (convoluzione FFT a partizioni uniformi, overlap-save)
//  Ogni passa-basso è progettato a fase zero con modulo 1/(1+(f/fc)^p) (p = 1, 2, 4, 8 -> 6..48 dB/oct);
//  le bande sono differenze di passa-basso consecutivi, quindi la loro somma è un impulso ritardato:
//  ricostruzione perfetta qualunque siano frequenze e pendenze. I canali L/R viaggiano insieme nella
//  parte reale e immaginaria della stessa FFT.
#pragma once
#include "Basics.h"
#include <array>

namespace sdsp
{
class LinearPhaseSplitter
{
public:
    static constexpr int maxBands = 6;
    using cplx = std::complex<double>;

    // halfLen K: kernel di 2K+1 tap, latenza = K + P
    void prepare (double sampleRate, int halfLen, int partition)
    {
        fs = sampleRate;
        K = halfLen;
        P = partition;
        M = 2 * P;
        T = 2 * K + 1;
        Np = (T + P - 1) / P;
        fft.prepare (M);
        D = 1;
        while (D < 4 * K) D <<= 1;
        dfft.prepare (D);
        design.assign ((size_t) D, {});
        window.resize ((size_t) T);
        for (int n = 0; n < T; ++n)   // Blackman-Harris a 4 termini
        {
            const double x = kTwoPi * n / (T - 1);
            window[(size_t) n] = 0.35875 - 0.48829 * std::cos (x) + 0.14128 * std::cos (2 * x) - 0.01168 * std::cos (3 * x);
        }
        for (auto& set : kern) for (auto& b : set) b.assign ((size_t) (Np * M), {});
        fdl.assign ((size_t) (Np * M), {});
        inBuf.assign ((size_t) M, {});
        for (auto& o : outBlk) o.assign ((size_t) P, {});
        for (auto& o : outOld) o.assign ((size_t) P, {});
        work.assign ((size_t) M, {});
        acc.assign ((size_t) M, {});
        lpTime.resize ((size_t) maxBands);
        for (auto& v : lpTime) v.assign ((size_t) T, 0.0);
        bandTime.assign ((size_t) T, 0.0);
        cur = 0;
        fdlHead = 0;
        fill = 0;
        nBands = 0;
        for (auto& f : freqs) f = -1;
        for (auto& s : slopes) s = -1;
        fading = false;
        dirty = true;
    }
    int latency() const { return K + P; }
    void reset()
    {
        std::fill (fdl.begin(), fdl.end(), cplx {});
        std::fill (inBuf.begin(), inBuf.end(), cplx {});
        for (auto& o : outBlk) std::fill (o.begin(), o.end(), cplx {});
        fill = 0;
    }

    // imposta i crossover (frequenze crescenti) e le pendenze (0..3); ricalcola solo se cambiano davvero
    void setCrossovers (int bands, const double* f, const int* slope)
    {
        bool changed = bands != nBands;
        for (int i = 0; i < bands - 1 && ! changed; ++i)
            if (std::abs (f[i] - freqs[(size_t) i]) > freqs[(size_t) i] * 0.003 || slope[i] != slopes[(size_t) i]) changed = true;
        if (! changed) return;
        pendBands = bands;
        for (int i = 0; i < bands - 1; ++i) { pendFreq[(size_t) i] = f[i]; pendSlope[(size_t) i] = slope[i]; }
        dirty = true;
    }

    // un campione stereo in ingresso; out[b] = campione (L + jR) della banda b, ritardato di latency()
    inline void process (double l, double r, cplx* out)
    {
        inBuf[(size_t) (P + fill)] = cplx (l, r);
        const int idx = fill;
        if (fading)
        {
            const double t = (idx + 0.5) / P;
            for (int b = 0; b < nBands; ++b) out[b] = outOld[(size_t) b][(size_t) idx] * (1.0 - t) + outBlk[(size_t) b][(size_t) idx] * t;
        }
        else
            for (int b = 0; b < nBands; ++b) out[b] = outBlk[(size_t) b][(size_t) idx];
        if (++fill >= P)
        {
            fill = 0;
            processBlock();
        }
    }
    int bands() const { return nBands; }

private:
    void designKernels (int set)
    {
        // passa-basso per ogni crossover
        for (int i = 0; i < nBands - 1; ++i)
        {
            const double fc = freqs[(size_t) i];
            const double p = (double) (1 << slopes[(size_t) i]);
            for (int k = 0; k <= D / 2; ++k)
            {
                const double f = (double) k * fs / D;
                const double m = 1.0 / (1.0 + std::pow (f / fc, p));
                design[(size_t) k] = m;
                if (k > 0 && k < D / 2) design[(size_t) (D - k)] = m;
            }
            dfft.perform (design.data(), true);
            auto& lp = lpTime[(size_t) i];
            for (int n = -K; n <= K; ++n)
                lp[(size_t) (n + K)] = design[(size_t) ((n + D) % D)].real() * window[(size_t) (n + K)];
        }
        // bande: b0 = lp0, bk = lp_k - lp_(k-1), ultima = delta - lp_(n-2)
        for (int b = 0; b < nBands; ++b)
        {
            for (int n = 0; n < T; ++n)
            {
                double v = (b < nBands - 1) ? lpTime[(size_t) b][(size_t) n] : (n == K ? 1.0 : 0.0);
                if (b > 0) v -= lpTime[(size_t) (b - 1)][(size_t) n];
                bandTime[(size_t) n] = v;
            }
            auto& H = kern[(size_t) set][(size_t) b];
            for (int p = 0; p < Np; ++p)
            {
                for (int i = 0; i < M; ++i) work[(size_t) i] = 0;
                for (int i = 0; i < P; ++i)
                {
                    const int n = p * P + i;
                    work[(size_t) i] = n < T ? bandTime[(size_t) n] : 0.0;
                }
                fft.perform (work.data(), false);
                std::copy (work.begin(), work.end(), H.begin() + p * M);
            }
        }
    }

    void processBlock()
    {
        // spettro del blocco [precedente | nuovo] nella linea di ritardo in frequenza
        fdlHead = (fdlHead + Np - 1) % Np;
        cplx* X = fdl.data() + fdlHead * M;
        std::copy (inBuf.begin(), inBuf.end(), X);
        fft.perform (X, false);
        std::copy (inBuf.begin() + P, inBuf.end(), inBuf.begin());

        fading = false;
        if (dirty)
        {
            const int nb = pendBands > 0 ? pendBands : 1;
            const bool sameCount = nb == nBands;
            // uscite col vecchio kernel per la dissolvenza
            if (sameCount && nBands > 0)
            {
                for (int b = 0; b < nBands; ++b) convolve (cur, b, outOld[(size_t) b]);
                fading = true;
            }
            nBands = nb;
            for (int i = 0; i < nBands - 1; ++i) { freqs[(size_t) i] = pendFreq[(size_t) i]; slopes[(size_t) i] = pendSlope[(size_t) i]; }
            cur ^= 1;
            designKernels (cur);
            dirty = false;
        }
        for (int b = 0; b < nBands; ++b) convolve (cur, b, outBlk[(size_t) b]);
    }

    void convolve (int set, int b, std::vector<cplx>& out)
    {
        std::fill (acc.begin(), acc.end(), cplx {});
        const auto& H = kern[(size_t) set][(size_t) b];
        for (int p = 0; p < Np; ++p)
        {
            const cplx* X = fdl.data() + ((fdlHead + p) % Np) * M;
            const cplx* h = H.data() + p * M;
            for (int i = 0; i < M; ++i) acc[(size_t) i] += X[i] * h[i];
        }
        fft.perform (acc.data(), true);
        for (int i = 0; i < P; ++i) out[(size_t) i] = acc[(size_t) (P + i)];
    }

    double fs = 48000;
    int K = 2048, P = 256, M = 512, T = 4097, Np = 17, D = 8192;
    Fft fft, dfft;
    std::vector<cplx> design, fdl, inBuf, work, acc;
    std::array<std::array<std::vector<cplx>, maxBands>, 2> kern;
    std::array<std::vector<cplx>, maxBands> outBlk, outOld;
    std::vector<std::vector<double>> lpTime;
    std::vector<double> bandTime, window;
    std::array<double, maxBands> freqs {}, pendFreq {};
    std::array<int, maxBands> slopes {}, pendSlope {};
    int cur = 0, fdlHead = 0, fill = 0, nBands = 0, pendBands = 1;
    bool fading = false, dirty = true;
};
} // namespace sdsp
