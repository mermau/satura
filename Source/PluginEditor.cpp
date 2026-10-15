#include "PluginEditor.h"
#include "PluginProcessor.h"

SaturaEditor::SaturaEditor (SaturaProcessor& p) : AudioProcessorEditor (p), proc (p), view (p)
{
    addAndMakeVisible (view);
    view.onSizeChanged = [this] { view.fullscreen = false; applySize(); };
    view.onToggleFullscreen = [this] { toggleFullscreen(); };
    setResizable (true, false);   // ridimensionamento dai bordi dove l'host lo consente (VST3), senza angolo JUCE
    setResizeLimits (View::BW / 2, View::BH / 2, View::BW * 4, View::BH * 4);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) View::BW / View::BH);
    applySize();
}

void SaturaEditor::applySize()
{
    const double s = sizes[juce::jlimit (0, 3, proc.model.sizeIdx)] * proc.model.scaleMul;
    setSize (juce::roundToInt (View::BW * s), juce::roundToInt (View::BH * s));
}

void SaturaEditor::toggleFullscreen()
{
    if (! view.fullscreen)
    {
        before = getBounds();
        auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds());
        if (d == nullptr) return;
        const auto area = d->userArea;
        const double s = std::min ((double) area.getWidth() / View::BW, (double) (area.getHeight() - 40) / View::BH);
        view.fullscreen = true;
        setSize (juce::roundToInt (View::BW * s), juce::roundToInt (View::BH * s));
        if (auto* top = getTopLevelComponent(); top != nullptr && top != this) top->setTopLeftPosition (area.getX(), area.getY());
    }
    else
    {
        view.fullscreen = false;
        setSize (before.getWidth(), before.getHeight());
    }
}

void SaturaEditor::resized()
{
    const double s = std::min ((double) getWidth() / View::BW, (double) getHeight() / View::BH);
    view.setTransform (juce::AffineTransform::scale ((float) s));
    view.setBounds (0, 0, View::BW, View::BH);
}
