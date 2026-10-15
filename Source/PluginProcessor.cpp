#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace spec;

juce::AudioProcessorValueTreeState::ParameterLayout SaturaProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    for (int i = 0; i < NP; ++i)
    {
        const auto& p = params()[(size_t) i];
        const juce::ParameterID id { p.id, 1 };
        const bool automatable = i != P::nbands;
        if (p.map == Map::choice)
        {
            juce::StringArray ch = juce::StringArray::fromTokens (p.choices, "|", "");
            v.push_back (std::make_unique<juce::AudioParameterChoice> (id, p.label, ch, (int) p.def,
                juce::AudioParameterChoiceAttributes().withAutomatable (automatable)));
        }
        else if (p.map == Map::integer)
        {
            v.push_back (std::make_unique<juce::AudioParameterInt> (id, p.label, (int) p.min, (int) p.max, (int) p.def,
                juce::AudioParameterIntAttributes().withAutomatable (automatable)));
        }
        else
        {
            juce::NormalisableRange<float> r (p.min, p.max,
                [i] (float, float, float n) { return (float) Model::denorm (i, n); },
                [i] (float, float, float x) { return (float) Model::norm (i, x); });
            auto attrs = juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([i] (float x, int) { return Model::fmtVal (i, x); })
                .withValueFromStringFunction ([i] (const juce::String& s) { double o = 0; return Model::parseEntry (i, s, o) ? (float) o : (float) params()[(size_t) i].def; });
            v.push_back (std::make_unique<juce::AudioParameterFloat> (id, p.label, r, p.def, attrs));
        }
    }
    return { v.begin(), v.end() };
}

SaturaProcessor::SaturaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "SATURA", createLayout()),
      model (*this)
{
    for (int i = 0; i < NP; ++i) prm[(size_t) i] = apvts.getParameter (params()[(size_t) i].id);
    for (auto& c : ccMap) c.store (-1);
    model.loadPrefs();
    model.revertLearn();      // mappa MIDI salvata (globale)
    model.ensureFactoryPresets();
    model.loadDefaultPreset();
    model.writeCfg();
    startTimerHz (30);
}

SaturaProcessor::~SaturaProcessor()
{
    stopTimer();
    model.saveLearn();        // come Saturn 2: la mappatura corrente si salva alla chiusura
}

double SaturaProcessor::rawParam (int i) const
{
    auto* p = prm[(size_t) i];
    return p->convertFrom0to1 (p->getValue());
}

bool SaturaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void SaturaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < NP; ++i) raw[(size_t) i] = (float) rawParam (i);
    engine.setParams (raw.data());
    {
        const juce::SpinLock::ScopedLockType l (cfgLock);
        cfgLocal = cfgPending;
    }
    engine.setConfig (cfgLocal.data());
    engine.prepare (sampleRate);
    events.reserve (2048);
    scratchL.assign ((size_t) std::max (1, samplesPerBlock), 0.0f);
    scratchR.assign ((size_t) std::max (1, samplesPerBlock), 0.0f);
    setLatencySamples (engine.latency());
}

void SaturaProcessor::pushConfig (const std::array<float, CFG::SIZE>& c)
{
    {
        const juce::SpinLock::ScopedLockType l (cfgLock);
        cfgPending = c;
    }
    cfgVersion.fetch_add (1);
}

void SaturaProcessor::post (int type, int a, int b)
{
    const auto w = fifo.write (1);
    if (w.blockSize1 > 0) msgs[(size_t) w.startIndex1] = { type, a, b };
}

void SaturaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    auto main = getBusBuffer (buffer, true, 0);
    const int nch = main.getNumChannels();
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i) buffer.clear (i, 0, n);
    if (nch == 0 || n == 0) { midi.clear(); return; }

    for (int i = 0; i < NP; ++i) raw[(size_t) i] = (float) rawParam (i);
    engine.setParams (raw.data());
    const int ver = cfgVersion.load();
    if (ver != cfgSeen)
    {
        const juce::SpinLock::ScopedTryLockType l (cfgLock);
        if (l.isLocked())
        {
            cfgLocal = cfgPending;
            cfgSeen = ver;
            engine.setConfig (cfgLocal.data());
        }
    }

    sdsp::Engine::Transport tp;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            tp.valid = true;
            tp.playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) tp.bpm = *b;
            if (auto q = pos->getPpqPosition()) tp.ppq = *q;
        }

    events.clear();
    const bool midiOn = midiEnabled.load();
    for (const auto m : midi)
    {
        if (! midiOn) break;
        const auto msg = m.getMessage();
        const auto* d = msg.getRawData();
        if (msg.getRawDataSize() < 1) continue;
        const std::uint8_t st = d[0];
        const std::uint8_t d1 = msg.getRawDataSize() > 1 ? d[1] : 0, d2 = msg.getRawDataSize() > 2 ? d[2] : 0;
        if (msg.isController())
        {
            if (d1 == 0) bank = d2;
            if (learnActive.load()) post (0, d1, d2);
            else if (const int p = ccMap[(size_t) (d1 & 127)].load(); p >= 0) post (1, p, d2);
        }
        else if (msg.isProgramChange()) post (2, bank, d1);
        if (events.size() < events.capacity())
            events.push_back ({ m.samplePosition, st, d1, d2 });
    }
    midi.clear();

    const float* scL = nullptr; const float* scR = nullptr;
    if (getBusCount (true) > 1)
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0 && getBus (true, 1)->isEnabled())
        {
            scL = sc.getReadPointer (0);
            scR = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : scL;
        }
    }
    float* L = main.getWritePointer (0);
    float* R = nch > 1 ? main.getWritePointer (1) : L;
    engine.process (L, R, nch, scL, scR, n, events.data(), (int) events.size(), tp);
    if (engine.latency() != getLatencySamples()) wantLatency.store (engine.latency() + 1);
}

void SaturaProcessor::timerCallback()
{
    if (const int wl = wantLatency.exchange (0); wl > 0) setLatencySamples (wl - 1);
    const auto r = fifo.read (fifo.getNumReady());
    auto handle = [this] (const Msg& m)
    {
        if (m.type == 0)
        {
            lastCcSeen.store (m.a);
            if (model.learnMode && model.learnArm >= 0) model.setLearn (m.a, model.learnArm);
        }
        else if (m.type == 1)
        {
            auto* p = prm[(size_t) m.a];
            p->beginChangeGesture();
            p->setValueNotifyingHost (m.b / 127.0f);
            p->endChangeGesture();
        }
        else if (m.type == 2 && model.pgmChanges) model.programChange (m.a, m.b);
    };
    for (int i = 0; i < r.blockSize1; ++i) handle (msgs[(size_t) (r.startIndex1 + i)]);
    for (int i = 0; i < r.blockSize2; ++i) handle (msgs[(size_t) (r.startIndex2 + i)]);
}

void SaturaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("struct", juce::JSON::toString (model.str.toVar(), true), nullptr);
    state.setProperty ("instance", juce::JSON::toString (model.instanceState(), true), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void SaturaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto vt = juce::ValueTree::fromXml (*xml);
        if (! vt.isValid()) return;
        const auto s = vt.getProperty ("struct").toString();
        const auto inst = vt.getProperty ("instance").toString();
        apvts.replaceState (vt);
        if (s.isNotEmpty()) model.str = Struct::fromVar (juce::JSON::parse (s));
        if (inst.isNotEmpty()) model.setInstanceState (juce::JSON::parse (inst));
        model.presetDirty = false;
        model.writeCfg();
        if (model.onChange) model.onChange();
    }
}

juce::AudioProcessorEditor* SaturaProcessor::createEditor() { return new SaturaEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SaturaProcessor(); }
