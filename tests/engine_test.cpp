// Test del motore SATURA (porting di build/test/run_tests.js della versione Max, più i test nuovi)
#include "dsp/Engine.h"
#include <cstdio>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
using namespace sdsp;
using namespace spec;
static int fails = 0;
#define CHECK(c, ...) do { printf ((c) ? "PASS " : "FAIL "); printf (__VA_ARGS__); printf ("\n"); if (!(c)) ++fails; } while (0)
static const double SR = 48000;

struct Rig
{
    std::unique_ptr<Engine> e = std::make_unique<Engine>();
    std::vector<float> raw, cfg;
    Rig() { raw.resize (NP); for (int i = 0; i < NP; ++i) raw[(size_t) i] = params()[(size_t) i].def; cfg.assign (CFG::SIZE, 0.0f); e->setParams (raw.data()); e->setConfig (cfg.data()); e->prepare (SR); }
    void set (int p, float v) { raw[(size_t) p] = v; e->setParams (raw.data()); }
    void apply() { e->setParams (raw.data()); e->setConfig (cfg.data()); }
    // elabora N campioni; gen(n) -> (l, r); ritorna L e R
    void run (int N, std::function<std::pair<double,double>(int)> gen, std::vector<float>& L, std::vector<float>& R,
              std::function<double(int)> sc = nullptr, std::function<void(int)> each = nullptr)
    {
        L.assign ((size_t) N, 0); R.assign ((size_t) N, 0);
        std::vector<float> s ((size_t) N, 0);
        for (int n = 0; n < N; ++n) { auto v = gen (n); L[(size_t) n] = (float) v.first; R[(size_t) n] = (float) v.second; if (sc) s[(size_t) n] = (float) sc (n); }
        Engine::Transport tp; tp.valid = true; tp.playing = true; tp.bpm = 120;
        const int B = 64;
        for (int i = 0; i < N; i += B)
        {
            const int m = std::min (B, N - i);
            tp.ppq = i / SR * 2.0;
            if (each) each (i);
            e->process (L.data() + i, R.data() + i, 2, s.data() + i, s.data() + i, m, nullptr, 0, tp);
        }
    }
};
static double rms (const std::vector<float>& a, size_t s = 0) { double t = 0; for (size_t i = s; i < a.size(); ++i) t += (double) a[i] * a[i]; return std::sqrt (t / (double) (a.size() - s)); }
static auto sine (double f, double a) { return [=] (int n) { double v = a * std::sin (kTwoPi * f * n / SR); return std::make_pair (v, v); }; }

int main()
{
    std::vector<float> L, R;
    // T1 default
    { Rig r; r.run ((int) SR, sine (1000, 0.25), L, R); double g = gainToDb (rms (L, 4800) / (0.25 / std::sqrt (2.0)));
      CHECK (std::abs (g) < 3, "T1 impostazioni di default (Warm Tape, drive 6 dB): guadagno %.2f dB", g); }
    // T2 ricostruzione: bande disattivate, impulso
    auto xover = [&] (int nbands, int slope, bool lp, int hq, double& dev, int& pk)
    {
        Rig r; r.set (P::nbands, (float) nbands); r.set (P::linphase, lp ? 1.f : 0.f); r.set (P::hq, (float) hq);
        for (int i = 0; i < 5; ++i) r.set (P::xs1 + i, (float) slope);
        for (int b = 0; b < 6; ++b) r.set (bandParam (b, BP::en), 0);
        r.e->prepare (SR);
        r.run (16384, [] (int) { return std::make_pair (0.0, 0.0); }, L, R);
        const int N = 16384;
        r.run (N, [] (int n) { return std::make_pair (n == 0 ? 1.0 : 0.0, n == 0 ? 1.0 : 0.0); }, L, R);
        // spettro con DFT su griglia logaritmica
        dev = 0;
        for (double f = 20; f < 20000; f *= 1.05)
        {
            double re = 0, im = 0;
            for (int n = 0; n < N; ++n) { re += L[(size_t) n] * std::cos (kTwoPi * f * n / SR); im += L[(size_t) n] * std::sin (kTwoPi * f * n / SR); }
            dev = std::max (dev, std::abs (gainToDb (std::hypot (re, im))));
        }
        pk = 0; for (int n = 0; n < N; ++n) if (std::abs (L[(size_t) n]) > std::abs (L[(size_t) pk])) pk = n;
        return r.e->latency();
    };
    for (int nb : { 2, 6 }) for (int sl = 0; sl < 4; ++sl)
    { double dev; int pk; xover (nb, sl, false, 0, dev, pk); CHECK (dev < 0.01, "T2 IIR %d bande %d dB/oct: piatta entro %.4f dB", nb, 6 << sl, dev); }
    for (int hq : { 0, 1, 2 })
    { double dev; int pk; int lat = xover (6, 2, true, hq, dev, pk); CHECK (dev < 0.01 && pk == lat, "T2 Linear Phase 6 bande, HQ %d: piatta entro %.4f dB, picco a %d = latenza %d", hq, dev, pk, lat); }
    for (int hq : { 1, 2 })
    { double dev; int pk; xover (3, 2, false, hq, dev, pk); CHECK (dev < 0.05, "T2 IIR 3 bande con oversampling %s: piatta entro %.4f dB (bande disattivate, ramo asciutto allineato)", hq == 1 ? "8x" : "32x", dev); }
    // T3 stili
    for (int s = 0; s < NSTYLES; ++s)
    {
        Rig r; r.set (bandParam (0, BP::style), (float) s); r.set (bandParam (0, BP::drive), 24);
        r.run ((int) SR / 2, sine (220, 0.25), L, R);
        double g = gainToDb (rms (L, 4800) / (0.25 / std::sqrt (2.0))); double pk = 0; for (auto v : L) pk = std::max (pk, (double) std::abs (v));
        CHECK (g > -24 && g < 12 && pk < 4, "T3 stile %s: guadagno %.1f dB, picco %.2f", styles()[(size_t) s].name, g, pk);
    }
    // T4 aliasing
    auto alias = [&] (int hq, bool lp)
    {
        Rig r; r.set (bandParam (0, BP::style), 11); r.set (bandParam (0, BP::drive), 36); r.set (P::hq, (float) hq); r.set (P::linphase, lp ? 1.f : 0.f); r.e->prepare (SR);
        const int N = 32768, W = 8192;
        const int bin = 1499; const double f0 = SR * bin / 8192.0;
        r.run (N + W, sine (f0, 0.8), L, R);
        // energia alle armoniche (multipli di f0) contro il resto sotto 20 kHz (finestra di Hann, DFT a frequenze scelte)
        std::vector<double> x ((size_t) N); for (int n = 0; n < N; ++n) x[(size_t) n] = L[(size_t) (n + W)] * (0.5 - 0.5 * std::cos (kTwoPi * n / N));
        double ha = 0, al = 0;
        for (int k = 1; k < N / 2; k += 3)
        {
            const double f = (double) k * SR / N; if (f > 20000) break;
            double re = 0, im = 0; for (int n = 0; n < N; n += 1) { re += x[(size_t) n] * std::cos (kTwoPi * k * n / N); im += x[(size_t) n] * std::sin (kTwoPi * k * n / N); }
            const double m2 = re * re + im * im;
            const double h = f / f0; const bool isH = std::abs (h - std::round (h)) * f0 < 25;
            (isH ? ha : al) += m2;
        }
        return 10 * std::log10 (al / ha);
    };
    const double a0 = alias (0, false), a1 = alias (1, false), a2 = alias (2, false), a2l = alias (2, true);
    CHECK (a1 < a0 - 20 && a2 < a1 - 3 && a2l < a0 - 20, "T4 aliasing: Off %.1f dB, Good %.1f dB, Superb %.1f dB, Superb Linear Phase %.1f dB", a0, a1, a2, a2l);
    // T5 XLFO -> drive
    {
        Rig r; auto& c = r.cfg; const int XB = CFG::XLFO_BASE;
        c[XB] = 1; c[XB + 1] = 2; c[XB + 8] = 1; c[XB + 10] = 3; c[XB + 11] = 1; c[XB + 12] = -1; c[XB + 14] = 3; c[XB + 15] = 1;
        const int sb = CFG::SLOT_BASE; c[sb] = 1; c[sb + 1] = SRC::xlfo1; c[sb + 2] = (float) params()[P::b1_drive].target; c[sb + 3] = 0.5f;
        r.set (P::x1_rate, 4); r.apply();
        double mn = 1, mx = 0, smn = 1, smx = -1;
        r.run ((int) SR / 2, [] (int n) { double v = 0.1 * std::sin (n * 0.05); return std::make_pair (v, v); }, L, R, nullptr, [&] (int) {
            const double v = r.e->view[(size_t) params()[P::b1_drive].target].load(); mn = std::min (mn, v); mx = std::max (mx, v);
            const double s = r.e->view[(size_t) (VIEW::SRC + SRC::xlfo1)].load(); smn = std::min (smn, s); smx = std::max (smx, s); });
        CHECK (smn < -0.95 && smx > 0.95 && mn < 0.01 && mx > 0.6, "T5 XLFO %.2f..%.2f, drive normalizzato %.2f..%.2f", smn, smx, mn, mx);
    }
    // T6 EG ed EF
    {
        Rig r; auto& c = r.cfg; c[CFG::EG_BASE] = 1; c[CFG::EF_BASE] = 1;
        r.set (P::e1_att, 10); r.set (P::e1_dec, 50); r.set (P::e1_sus, 50); r.set (P::e1_rel, 100); r.set (P::e1_thr, -30); r.apply();
        double egmax = 0, egEnd = 1, efmax = 0, efEnd = 1;
        r.run ((int) SR, [] (int n) { double v = n < SR / 4 ? 0.3 * std::sin (n * 0.03) : 0; return std::make_pair (v, v); }, L, R, nullptr, [&] (int i) {
            const double v = r.e->view[(size_t) (VIEW::SRC + SRC::eg1)].load(), f = r.e->view[(size_t) (VIEW::SRC + SRC::ef1)].load();
            if (i < SR / 2) { egmax = std::max (egmax, v); efmax = std::max (efmax, f); } else { egEnd = v; efEnd = f; } });
        CHECK (egmax > 0.99 && egEnd < 0.01, "T6 EG: picco %.3f, fine %.3f", egmax, egEnd);
        CHECK (efmax > 0.7 && efEnd < 0.05, "T6 EF: picco %.3f, fine %.3f", efmax, efEnd);
    }
    // T6b EG sul side chain esterno
    {
        Rig r; auto& c = r.cfg; c[CFG::EG_BASE] = 1; c[CFG::EG_BASE + 1] = 1; r.set (P::e1_thr, -30); r.apply();
        double egmax = 0;
        r.run ((int) SR / 2, [] (int) { return std::make_pair (0.0, 0.0); }, L, R, [] (int n) { return n < SR / 8 ? 0.5 * std::sin (n * 0.05) : 0.0; },
               [&] (int) { egmax = std::max (egmax, (double) r.e->view[(size_t) (VIEW::SRC + SRC::eg1)].load()); });
        CHECK (egmax > 0.99, "T6b EG attivato dal side chain esterno (ingresso principale muto): picco %.3f", egmax);
    }
    // T6c MIDI: EG su nota, sorgente MIDI (mod wheel)
    {
        Rig r; auto& c = r.cfg; c[CFG::EG_BASE] = 1; c[CFG::EG_BASE + 1] = 9; c[CFG::MIDI_BASE] = 1; c[CFG::MIDI_BASE + 1] = 3; r.apply();
        std::vector<float> l (4800, 0.f), rr (4800, 0.f);
        Engine::MidiEvent ev[2] = { { 10, 0x90, 60, 100 }, { 20, 0xB0, 1, 64 } };
        Engine::Transport tp;
        for (int i = 0; i < 4800; i += 480) r.e->process (l.data() + i, rr.data() + i, 2, nullptr, nullptr, 480, i == 0 ? ev : nullptr, i == 0 ? 2 : 0, tp);
        const double egv = r.e->view[(size_t) (VIEW::SRC + SRC::eg1)].load(), mw = r.e->view[(size_t) (VIEW::SRC + SRC::midi1)].load();
        CHECK (egv > 0.4 && std::abs (mw - 64 / 127.0) < 1e-3, "T6c MIDI: EG su nota %.2f, mod wheel %.3f", egv, mw);
    }
    // T7 feedback
    for (int unr = 0; unr < 2; ++unr)
    {
        Rig r; r.set (bandParam (0, BP::fba), 100); r.set (bandParam (0, BP::fbf), 440); r.set (bandParam (0, BP::drive), 24); r.set (P::unrestricted, (float) unr);
        r.run ((int) SR * 3, [] (int n) { double v = n < SR / 10 ? 0.3 * std::sin (n * 0.06) : 0; return std::make_pair (v, v); }, L, R);
        const double t = rms (L, (size_t) (SR * 2.5));
        CHECK (unr ? t > 0.01 : t < 0.001, "T7 feedback, Unrestricted %s: coda %.1f dBFS", unr ? "On" : "Off", gainToDb (t));
    }
    // T8 bypass, solo, M/S
    for (int lp = 0; lp < 2; ++lp)
    {
        Rig r; r.set (P::linphase, (float) lp); r.e->prepare (SR); r.set (P::bypass, 1);
        r.run (12000, sine (500, 0.5), L, R);
        const int lat = r.e->latency(); double err = 0;
        for (int n = 8000; n < 12000; ++n) err = std::max (err, std::abs (L[(size_t) n] - 0.5 * std::sin (kTwoPi * 500 * (n - lat) / SR)));
        CHECK (err < 1e-5, "T8 bypass %s trasparente e allineato alla latenza %d (errore %.1e)", lp ? "Linear Phase" : "normale", lat, err);
    }
    { Rig r; r.set (P::nbands, 2); r.set (P::xf1, 1000); r.set (bandParam (1, BP::solo), 1); r.run ((int) SR / 2, sine (100, 0.5), L, R);
      CHECK (rms (L, 9600) < 0.01, "T8 solo della banda alta: 100 Hz a %.1f dBFS", gainToDb (rms (L, 9600))); }
    { Rig r; r.set (P::msmode, 1); r.set (bandParam (0, BP::en), 0);
      r.run (4800, [] (int n) { return std::make_pair (0.5 * std::sin (n * 0.01), 0.2 * std::cos (n * 0.013)); }, L, R);
      double err = 0; for (int n = 2400; n < 4800; ++n) err = std::max (err, std::abs (L[(size_t) n] - 0.5 * std::sin (n * 0.01)) + std::abs (R[(size_t) n] - 0.2 * std::cos (n * 0.013)));
      CHECK (err < 1e-5, "T8 M/S trasparente (errore %.1e)", err); }
    // T8b mix globale e di banda allineati in fase con oversampling (niente filtro a pettine)
    for (int hq : { 1, 2 })
    {
        Rig r; r.set (P::hq, (float) hq); r.set (bandParam (0, BP::drive), 0); r.set (bandParam (0, BP::style), 9); r.set (bandParam (0, BP::mix), 50); r.set (P::gmix, 50); r.e->prepare (SR);
        double worst = 0;
        for (double f : { 2000.0, 8000.0, 15000.0 })
        { r.e->reset(); r.run (9600, sine (f, 0.01), L, R); worst = std::max (worst, std::abs (gainToDb (rms (L, 4800) / (0.01 / std::sqrt (2.0))))); }
        CHECK (worst < 0.5, "T8b mix al 50%% con oversampling %s: nessun filtro a pettine (scarto max %.2f dB)", hq == 1 ? "8x" : "32x", worst);
    }
    // T9 dinamica
    {
        auto lev = [&] (double dyn) { Rig r; r.set (bandParam (0, BP::style), 1); r.set (bandParam (0, BP::drive), 0); r.set (bandParam (0, BP::dyn), (float) dyn); r.run ((int) SR / 2, sine (300, 0.05), L, R); return gainToDb (rms (L, 12000)); };
        const double g0 = lev (0), gc = lev (100), gg = lev (-100);
        CHECK (gg < g0 - 6 && std::abs (gc - g0) < 12, "T9 dinamica: neutro %.1f, compressione %.1f, gate %.1f dBFS", g0, gc, gg);
    }
    // T10 mono
    { Rig r; std::vector<float> m (4800); for (int n = 0; n < 4800; ++n) m[(size_t) n] = (float) (0.25 * std::sin (kTwoPi * 1000 * n / SR));
      Engine::Transport tp; r.e->process (m.data(), m.data(), 1, nullptr, nullptr, 4800, nullptr, 0, tp);
      double t = 0; for (int n = 2400; n < 4800; ++n) t += m[(size_t) n] * m[(size_t) n]; t = std::sqrt (t / 2400);
      CHECK (std::isfinite (t) && t > 0.1, "T10 uso mono: %.3f rms", t); }
    // T11 carico CPU (indicativo)
    {
        Rig r; r.set (P::nbands, 6); r.set (P::hq, 2); r.set (P::linphase, 1); r.e->prepare (SR);
        auto t0 = std::chrono::steady_clock::now(); r.run ((int) SR * 2, sine (440, 0.3), L, R);
        double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        Rig r2; r2.set (P::nbands, 1); r2.e->prepare (SR);
        auto t1 = std::chrono::steady_clock::now(); r2.run ((int) SR * 2, sine (440, 0.3), L, R);
        double sec2 = std::chrono::duration<double> (std::chrono::steady_clock::now() - t1).count();
        printf ("INFO CPU: 1 banda %.1f%% di un core, 6 bande Superb Linear Phase %.1f%% (su questa macchina)\n", sec2 / 2 * 100, sec / 2 * 100);
    }
    printf (fails ? "%d test falliti\n" : "Tutti i test superati\n", fails);
    return fails ? 1 : 0;
}
