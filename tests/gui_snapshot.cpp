// Salva l'interfaccia in PNG (uso: gui_snapshot out.png [scenario])  -  "icon.png" genera l'icona dell'app
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static void save (juce::Component& c, const juce::String& file, float scale = 1.0f)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (file));
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String outName = argc > 1 ? argv[1] : "satura.png";
    if (outName.endsWith ("icon.png"))
    {
        juce::Image img (juce::Image::ARGB, 1024, 1024, true);
        juce::Graphics g (img);
        g.setColour (juce::Colour (0xff3a1215)); g.fillRoundedRectangle (40, 40, 944, 944, 200);
        juce::ColourGradient gr (juce::Colour (0xff8a1d24), 512, 300, juce::Colour (0xff2a0b0d), 512, 980, false);
        g.setGradientFill (gr); g.fillRoundedRectangle (60, 60, 904, 904, 180);
        juce::Path p; p.startNewSubPath (150, 700);
        for (int x = 150; x <= 874; x += 4) p.lineTo ((float) x, 560.0f - 260.0f * std::tanh (3.0f * std::sin ((x - 150) / 724.0f * 6.2832f)));
        g.setColour (juce::Colour (0xffe8d7d6)); g.strokePath (p, juce::PathStrokeType (34, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (outName)); out.deleteFile();
        juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    SaturaProcessor proc;
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* se = dynamic_cast<SaturaEditor*> (ed.get());
    ed->setSize (View::BW, View::BH);
    se->getView().tick();
    save (*ed, outName);
    return 0;
}
