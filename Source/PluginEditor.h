#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "View.h"

class SaturaProcessor;

// Contenitore: la vista lavora sempre a 900x600 e viene scalata (dimensioni Small..Extra Large,
// Scaling, ridimensionamento libero e schermo intero).
class SaturaEditor : public juce::AudioProcessorEditor
{
public:
    explicit SaturaEditor (SaturaProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff0d0a0a)); }
    View& getView() { return view; }
    static constexpr double sizes[4] = { 0.8, 1.0, 1.25, 1.5 };

private:
    void applySize();
    void toggleFullscreen();
    SaturaProcessor& proc;
    View view;
    juce::Rectangle<int> before;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturaEditor)
};
