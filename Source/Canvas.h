// SATURA - adattatore con la stessa semantica di mgraphics/canvas (percorso corrente, fill/stroke),
// usato per portare il disegno dell'interfaccia della versione Max quasi riga per riga.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class Canvas
{
public:
    explicit Canvas (juce::Graphics& gr) : g (gr) {}

    struct Grad { juce::ColourGradient cg; void stop (double t, float r, float gg, float b, float a = 1.0f) { cg.addColour (juce::jlimit (0.0, 1.0, t), juce::Colour::fromFloatRGBA (r, gg, b, a)); } };

    void rgb (float r, float gg, float b, float a = 1.0f) { fill = juce::FillType (juce::Colour::fromFloatRGBA (r, gg, b, a)); g.setColour (juce::Colour::fromFloatRGBA (r, gg, b, a)); }
    void col (const juce::Colour& c) { fill = juce::FillType (c); g.setColour (c); }
    void col (const juce::Colour& c, float a) { col (c.withMultipliedAlpha (a)); }
    Grad linear (float x1, float y1, float x2, float y2) { Grad gd; gd.cg = juce::ColourGradient (juce::Colours::transparentBlack, x1, y1, juce::Colours::transparentBlack, x2, y2, false); gd.cg.clearColours(); return gd; }
    Grad radial (float cx, float cy, float r) { Grad gd; gd.cg = juce::ColourGradient (juce::Colours::transparentBlack, cx, cy, juce::Colours::transparentBlack, cx + r, cy, true); gd.cg.clearColours(); return gd; }
    void source (const Grad& gd) { fill = juce::FillType (gd.cg); g.setGradientFill (gd.cg); }

    void lineWidth (float w) { lw = w; }
    void roundCap (bool r) { capRound = r; }

    void moveTo (float x, float y) { path.startNewSubPath (x, y); hasPt = true; }
    void lineTo (float x, float y) { if (! hasPt) moveTo (x, y); else path.lineTo (x, y); }
    void curveTo (float a, float b, float c, float d, float e, float f) { if (! hasPt) moveTo (a, b); path.cubicTo (a, b, c, d, e, f); }
    void closePath() { path.closeSubPath(); }
    void rect (float x, float y, float w, float h) { path.addRectangle (x, y, w, h); hasPt = false; }
    void rrect (float x, float y, float w, float h, float r) { path.addRoundedRectangle (x, y, w, h, juce::jmin (r, w * 0.5f, h * 0.5f)); hasPt = false; }
    void ellipse (float x, float y, float w, float h) { path.addEllipse (x, y, w, h); hasPt = false; }
    void circle (float x, float y, float r) { ellipse (x - r, y - r, 2 * r, 2 * r); }
    // angoli come in canvas: 0 = ore 3, senso orario
    void arc (float cx, float cy, float r, float a0, float a1)
    {
        constexpr float h = juce::MathConstants<float>::halfPi;
        path.addCentredArc (cx, cy, std::abs (r), std::abs (r), 0.0f, a0 + h, a1 + h, ! hasPt);
        hasPt = true;
    }
    void doFill() { g.fillPath (path); clear(); }
    void fillKeep() { g.fillPath (path); }
    void doStroke()
    {
        g.strokePath (path, juce::PathStrokeType (lw, juce::PathStrokeType::mitered, capRound ? juce::PathStrokeType::rounded : juce::PathStrokeType::butt));
        clear();
    }
    void line (float x1, float y1, float x2, float y2) { moveTo (x1, y1); lineTo (x2, y2); doStroke(); }
    void clear() { path.clear(); hasPt = false; }

    // testo: y = linea di base; align 'l', 'c', 'r'; ritorna la larghezza
    void font (float size, bool bold) { fnt = juce::Font (juce::FontOptions (size * 1.08f).withStyle (bold ? "Bold" : "Regular")); g.setFont (fnt); }
    float textWidth (const juce::String& s) const { return juce::GlyphArrangement::getStringWidth (fnt, s); }
    float text (const juce::String& s, float x, float y, float size, juce::Colour c, char align = 'l', bool bold = false)
    {
        font (size, bold);
        const float w = textWidth (s);
        const float xx = align == 'c' ? x - w * 0.5f : (align == 'r' ? x - w : x);
        g.setColour (c);
        g.drawSingleLineText (s, juce::roundToInt (xx), juce::roundToInt (y));
        return w;
    }

    juce::Graphics& g;

private:
    juce::Path path;
    juce::FillType fill;
    juce::Font fnt { juce::FontOptions (10.0f) };
    float lw = 1.0f;
    bool capRound = false, hasPt = false;
};
