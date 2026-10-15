// SATURA - motore audio (senza JUCE)
// Porting del motore gen~ della versione Max for Live, con in più:
//  - crossover a fase lineare via FFT (latenza dichiarata all'host)
//  - oversampling polifase IIR (normale) o FIR a fase lineare (Linear Phase)
//  - ingresso side chain e MIDI nativi, sorgenti MIDI calcolate nel motore
#pragma once
#include "Basics.h"
#include "Oversampler.h"
#include "LinearPhaseSplitter.h"
#include "../Spec.h"
#include <atomic>
#include <cstdint>

namespace sdsp
{
class Engine
{
public:
    static constexpr int CR = 8;   // campioni per blocco di controllo
    static constexpr int MB = spec::MAXBANDS;

    struct MidiEvent { int pos; std::uint8_t st, d1, d2; };
    struct Transport { bool valid = false, playing = false; double bpm = 120.0, ppq = 0.0; };

    Engine();
    void prepare (double sampleRate);
    void reset();

    // valori grezzi dei parametri (unità reali), una volta per blocco
    void setParams (const float* raw);
    // configurazione della modulazione (layout spec::CFG)
    void setConfig (const float* cfg);

    // elabora in place; R può coincidere con L per l'uso mono (nch = 1)
    void process (float* L, float* R, int nch, const float* scL, const float* scR, int n,
                  const MidiEvent* ev, int nev, const Transport& tp);

    int latency() const { return curLatency; }
    int latencyFor (bool linPhase, int hq) const;

    // ---- dati verso l'interfaccia
    std::array<std::atomic<float>, spec::VIEW::SIZE> view;
    static constexpr int SPEC_SIZE = 8192;
    std::array<float, SPEC_SIZE> specRing {};
    std::atomic<int> specPos { 0 };
    std::atomic<int> audition { 0 };   // 0 off, 1 main, 2 side chain, 3.. banda

private:
    struct BandCoefs
    {
        BiquadCoefs fb, prehp, preeq, posteq, postlp, cab, tone[4];
        double sht = 0, bias = 0, special = 0, inner = 1, comp[2] { 1, 1 }, dnorm = 0, dyn = 0;
    };
    struct BandCh
    {
        BiquadState fb, prehp, preeq, posteq, postlp, cab, tone[4];
        bool dryActive = false;
        double dcx = 0, dcy = 0, fbLast = 0, env = 0, bdPrev = 0, bdFlip = 1, dCnt = 0, dHold = 0;
        std::vector<double> smudge;
        Oversampler os, osDry;
        DelayLine dryDelay;
    };
    struct Smooth
    {
        double v = 0, t = 0;
        inline double next (double k) { v += (t - v) * k; return v; }
    };
    struct BandSm { Smooth gain[2], mix, lvl[2], aud, fba, en; };

    void controlBlock();
    void designCrossoversIir();
    void updateStructure();
    double sourceMidi (int k) const;
    void handleMidi (const MidiEvent& e);

    double fs = 48000.0;
    std::array<float, spec::NP> praw {};
    std::array<float, spec::CFG::SIZE> cfg {};
    std::array<double, spec::NP> pv {};
    std::array<double, spec::NT> msum {};
    std::array<double, spec::NSRC> srcv {};

    // modulazione
    struct Lfo { double acc = 0, syncOfs = 0, lastTrig = 0; int lastStep = -1; double rnd[16] {}; };
    struct Eg { int stage = 0; double lvl = 0, t = 0, start = 0, gate = 0, lastTrig = 0; };
    struct Ef { double env = 0, fast = 0, slow = 0; bool init = false; };
    std::array<Lfo, spec::NXLFO> lfo;
    std::array<Eg, spec::NEG> eg;
    std::array<Ef, spec::NEF> ef;
    std::array<double, 8> trigPeak {}, trigDb {};
    std::array<double, 4 + MB> meters {};

    // MIDI
    double midiGate = 0, midiTrig = 0, midiLTrig = 0, midiVel = 0, midiNote = 60, midiBend = 0, midiMw = 0, midiAt = 0;
    int heldNotes = 0;
    std::array<double, 128> midiCc {};
    std::uint32_t rng = 12345;
    double noise() { rng = rng * 1664525u + 1013904223u; return (double) (rng >> 8) / (double) (1u << 24) * 2.0 - 1.0; }

    // posizione di battuta
    double beatPos = 0, beatInc = 0;

    // crossover IIR
    struct XoCoefs { BiquadCoefs lp[4], hp[4], ap[2]; int slope = 2; };
    std::array<XoCoefs, MB - 1> xo;
    struct XoState { BiquadState lp[4], hp[4]; };
    std::array<std::array<XoState, MB - 1>, 2> xoSt;
    std::array<std::array<std::array<std::array<BiquadState, 2>, MB - 1>, MB>, 2> apSt;
    LinearPhaseSplitter lps;
    std::array<double, MB - 1> xfreq {};

    // bande
    std::array<BandCoefs, MB> bc;
    std::array<std::array<BandCh, 2>, MB> bch;
    std::array<BandSm, MB> bsm;
    Smooth igain[2], ogain[2], gmix, byp, fbScale;
    double ienv = 0, kFast = 0.01, kSlow = 0.001, kByp = 0.001, kDyn[2] {};

    // stato strutturale
    int nb = 1, curLatency = 0, osLog2 = 0;
    bool linPhase = false, msMode = false, unrestricted = false;
    int pendLin = -1, pendHq = -1;
    double structFade = 1.0, structTarget = 1.0;
    int ctl = 0, smudgePos = 0;
    DelayLine bypassDelayL, bypassDelayR;
    double osBuf[64] {}, osDryBuf[64] {};
    std::array<int, MB> dryHold {};
};
} // namespace sdsp
