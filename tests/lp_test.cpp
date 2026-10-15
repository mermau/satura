#include "dsp/LinearPhaseSplitter.h"
#include <cstdio>
using namespace sdsp;
int fails = 0;
#define CHECK(c, ...) do { printf ((c) ? "PASS " : "FAIL "); printf (__VA_ARGS__); printf ("\n"); if (!(c)) ++fails; } while (0)
int main()
{
    const double fs = 48000;
    for (int nb : { 2, 4, 6 })
        for (int sl = 0; sl < 4; ++sl)
        {
            LinearPhaseSplitter s; s.prepare (fs, 2048, 256);
            double f[5] = { 120, 800, 2500, 6000, 12000 }; int slope[5] = { sl, sl, sl, sl, sl };
            s.setCrossovers (nb, f, slope);
            const int N = 2048 + 256 + 8192;
            std::vector<std::vector<double>> h ((size_t) nb, std::vector<double> ((size_t) N, 0.0));
            std::vector<double> sum ((size_t) N, 0.0);
            std::complex<double> out[6];
            for (int n = 0; n < N; ++n)
            {
                for (auto& o : out) o = 0;
                s.process (n == 0 ? 1.0 : 0.0, 0.0, out);
                for (int b = 0; b < nb; ++b) { h[(size_t) b][(size_t) n] = out[b].real(); sum[(size_t) n] += out[b].real(); }
            }
            double err = 0; int pk = 0;
            for (int n = 0; n < N; ++n) { double want = n == s.latency() ? 1.0 : 0.0; err = std::max (err, std::abs (sum[(size_t) n] - want)); if (std::abs (sum[(size_t) n]) > std::abs (sum[(size_t) pk])) pk = n; }
            // risposta della banda 0 al crossover e un'ottava sopra
            auto mag = [&] (int b, double fr) { double re = 0, im = 0; for (int n = 0; n < N; ++n) { re += h[(size_t) b][(size_t) n] * std::cos (kTwoPi * fr * n / fs); im += h[(size_t) b][(size_t) n] * std::sin (kTwoPi * fr * n / fs); } return gainToDb (std::hypot (re, im)); };
            const double atFc = mag (0, f[0]), oct = mag (0, f[0] * 2) - atFc, oct2 = mag (0, f[0] * 4) - mag (0, f[0] * 2);
            CHECK (err < 1e-9 && pk == s.latency() && std::abs (atFc + 6.02) < 0.3, "%d bande %d dB/oct: somma = impulso (err %.1e) a %d, -%.2f dB al crossover, pendenza %.1f / %.1f dB/oct",
                   nb, 6 << sl, err, pk, -atFc, oct, oct2);
        }
    // cambio di frequenza durante l'esecuzione: niente scatti né perdita di ricostruzione
    {
        LinearPhaseSplitter s; s.prepare (fs, 2048, 256);
        double f[1] = { 500 }; int sl[1] = { 2 }; s.setCrossovers (2, f, sl);
        std::complex<double> out[6]; double maxJump = 0, prev = 0, worst = 0;
        for (int n = 0; n < 48000; ++n)
        {
            if (n % 256 == 0) { f[0] = 500 * std::pow (8.0, (double) n / 48000); s.setCrossovers (2, f, sl); }
            double x = std::sin (kTwoPi * 220 * n / fs) * 0.5;
            out[0] = out[1] = 0; s.process (x, x, out);
            double y = (out[0] + out[1]).real();
            if (n > 5000) { worst = std::max (worst, std::abs (y - 0.5 * std::sin (kTwoPi * 220 * (n - s.latency()) / fs))); maxJump = std::max (maxJump, std::abs (out[0].real() - prev)); }
            prev = out[0].real();
        }
        CHECK (worst < 1e-6 && maxJump < 0.05, "crossover modulato: somma esatta (err %.1e), banda continua (salto max %.4f)", worst, maxJump);
    }
    printf (fails ? "%d falliti\n" : "tutto ok\n", fails);
    return fails ? 1 : 0;
}
