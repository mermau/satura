#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Engine.h"
#include "Model.h"

class SaturaProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    SaturaProcessor();
    ~SaturaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return model.str.presetName; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return prm[(size_t) spec::P::bypass]; }

    juce::AudioProcessorValueTreeState apvts;
    std::array<juce::RangedAudioParameter*, spec::NP> prm {};
    sdsp::Engine engine;
    Model model;

    // configurazione della modulazione (scritta dal modello, letta dall'audio)
    void pushConfig (const std::array<float, spec::CFG::SIZE>& c);
    // MIDI Learn: mappa CC -> parametro letta dall'audio
    std::array<std::atomic<int>, 128> ccMap;
    std::atomic<bool> learnActive { false }, midiEnabled { true };
    std::atomic<int> lastCcSeen { -1 };

    double rawParam (int i) const;
    void handleMessages() { timerCallback(); }   // usato dai test
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void timerCallback() override;
    std::array<float, spec::NP> raw {};
    std::array<float, spec::CFG::SIZE> cfgPending {}, cfgLocal {};
    juce::SpinLock cfgLock;
    std::atomic<int> cfgVersion { 0 };
    int cfgSeen = -1;
    std::vector<sdsp::Engine::MidiEvent> events;
    // eventi audio -> messaggi
    struct Msg { int type, a, b; };   // 0 CC appreso, 1 CC mappato (param, valore), 2 program change (banco, programma)
    juce::AbstractFifo fifo { 512 };
    std::array<Msg, 512> msgs {};
    void post (int type, int a, int b);
    int bank = 0;
    std::atomic<int> wantLatency { 0 };
    std::vector<float> scratchL, scratchR;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturaProcessor)
};
