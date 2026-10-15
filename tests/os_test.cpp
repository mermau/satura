// Verifica dell'oversampler: risposta in banda, reiezione delle immagini, latenza FIR
#include "dsp/Oversampler.h"
#include <cstdio>
using namespace sdsp;
int fails = 0;
#define CHECK(c, ...) do { printf ((c) ? "PASS " : "FAIL "); printf (__VA_ARGS__); printf ("\n"); if (!(c)) ++fails; } while (0)

static double tone (Oversampler& os, double f, double fs, int N, int lat, double& phaseErr)
{
    os.reset();
    double buf[64]; double re = 0, im = 0, re0 = 0, im0 = 0;
    for (int n = 0; n < N; ++n)
    {
        double x = std::sin (kTwoPi * f * n / fs);
        os.up (x, buf);
        double y = os.down (buf);
        if (n > N / 2)
        {
            re += y * std::cos (kTwoPi * f * n / fs); im += y * std::sin (kTwoPi * f * n / fs);
            double xr = std::sin (kTwoPi * f * (n - lat) / fs);
            re0 += xr * std::cos (kTwoPi * f * n / fs); im0 += xr * std::sin (kTwoPi * f * n / fs);
        }
    }
    phaseErr = std::abs (std::arg (std::complex<double> (re, im) / std::complex<double> (re0, im0)));
    return std::hypot (re, im) / std::hypot (re0, im0);
}
int main()
{
    const double fs = 44100;
    for (int mode = 0; mode < 2; ++mode)
        for (int st : { 3, 5 })
        {
            Oversampler os; os.prepare (st, mode ? Oversampler::fir : Oversampler::iir);
            double worst = 0, worstPh = 0;
            for (double f : { 50.0, 1000.0, 5000.0, 10000.0, 18000.0, 20000.0 })
            {
                double ph; double g = tone (os, f, fs, 20000, os.latency(), ph);
                worst = std::max (worst, std::abs (gainToDb (g)));
                if (mode == 1) worstPh = std::max (worstPh, ph);
            }
            CHECK (worst < 0.01 && worstPh < 1e-3, "%s %dx: risposta piatta entro %.4f dB fino a 20 kHz%s (latenza %d)",
                   mode ? "FIR" : "IIR", 1 << st, worst, mode ? ", fase lineare" : "", os.latency());
            // immagini: un tono a 15 kHz sovracampionato non deve avere energia oltre 22.05 kHz
            os.reset(); double buf[64]; const int F = 1 << st; double eIm = 0, eTot = 0;
            std::vector<double> hi;
            for (int n = 0; n < 8192; ++n) { os.up (std::sin (kTwoPi * 15000 * n / fs), buf); for (int i = 0; i < F; ++i) hi.push_back (buf[i]); }
            // DFT su alcuni bin delle immagini (fs - 15k, fs + 15k)
            for (double f : { fs - 15000.0, fs + 15000.0, 2 * fs - 15000.0 })
            {
                double re = 0, im = 0, r0 = 0, i0 = 0;
                for (size_t n = hi.size() / 2; n < hi.size(); ++n)
                {
                    const double wn = 0.5 - 0.5 * std::cos (kTwoPi * (double) (n - hi.size() / 2) / (double) (hi.size() / 2));
                    re += wn * hi[n] * std::cos (kTwoPi * f * n / (fs * F)); im += wn * hi[n] * std::sin (kTwoPi * f * n / (fs * F));
                    r0 += wn * hi[n] * std::cos (kTwoPi * 15000 * n / (fs * F)); i0 += wn * hi[n] * std::sin (kTwoPi * 15000 * n / (fs * F));
                }
                eIm = std::max (eIm, std::hypot (re, im)); eTot = std::hypot (r0, i0);
            }
            CHECK (gainToDb (eIm / eTot) < -90, "%s %dx: immagini a %.1f dB", mode ? "FIR" : "IIR", F, gainToDb (eIm / eTot));
        }
    printf (fails ? "%d falliti\n" : "tutto ok\n", fails);
    return fails ? 1 : 0;
}
