// SATURA - disegno dell'interfaccia (porting di ui/gfx.js)
#include "View.h"
#include "PluginProcessor.h"

using namespace spec;
using juce::String;
using juce::Colour;

namespace
{
const Colour C_TXT = Colour::fromFloatRGBA (0.86f, 0.82f, 0.83f, 1);
const Colour C_DIM = Colour::fromFloatRGBA (0.55f, 0.5f, 0.51f, 1);
const Colour C_RED = Colour::fromFloatRGBA (0.85f, 0.14f, 0.18f, 1);
constexpr float PI_F = juce::MathConstants<float>::pi;
constexpr float HDR = 34, DY = 34, DH = 366, PANW = 548, PANH = 92, PANY = 300, FLOWY = 400, MODY = 426, BOTY = 572;
const double FREQ_GRID[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
const char* const MIDI_INPUTS[] = { "Velocity", "Note", "Pitch Bend", "Mod Wheel", "Aftertouch", "Controller" };
const char* const MIDI_CURVES[] = { "Linear", "Exponential", "Logarithmic", "Square", "Square Root", "Sine" };
const double SYNC_OFS[] = { 1.0, 1.5, 0.6666667 };
float clampf (float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
double curveFn (int c, double x)
{
    switch (c)
    {
        case 1: return (std::exp (x * 3) - 1) / (std::exp (3.0) - 1);
        case 2: return std::log (1 + x * 19) / std::log (20.0);
        case 3: return x * x;
        case 4: return std::sqrt (x);
        case 5: return 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * x);
        default: return x;
    }
}
}

// =================================================================== mappature
float View::fx (double f) const { return (float) (std::log (f / 10.0) / std::log (3000.0) * BW); }
double View::xf (float x) const { return 10.0 * std::pow (3000.0, x / BW); }
float View::levY (double db) const { return DY + 34 + (float) ((36 - juce::jlimit (-36.0, 36.0, db)) / 72.0 * 210.0); }
float View::specY (double db) const { return DY + 16 + (float) (juce::jlimit (0.0, 100.0, -db) / 100.0 * (DH - 16)); }

double View::effNorm (int p) const
{
    const auto& I = params()[(size_t) p];
    if (! engineAlive || I.target < 0 || m.slotsForTarget (I.target).empty()) return m.getNorm (p);
    return juce::jlimit (0.0, 1.0, (double) VIEWD[(size_t) I.target]);
}
double View::effVal (int p) const { return Model::denorm (p, effNorm (p)); }

std::pair<double, double> View::modRange (int t, double base)
{
    if (! engineAlive) return { base, base };
    const double v = VIEWD[(size_t) t];
    auto it = modTrack.find (t);
    if (it == modTrack.end()) it = modTrack.insert ({ t, { v, v } }).first;
    return { std::min ({ it->second.first, base, v }), std::max ({ it->second.second, base, v }) };
}

juce::Colour View::srcColor (const String& type) const
{
    if (type == "xy") return Colour::fromFloatRGBA (0.13f, 0.72f, 0.38f, 1);
    if (type == "xlfo") return Colour::fromFloatRGBA (0.13f, 0.83f, 0.78f, 1);
    if (type == "eg") return Colour::fromFloatRGBA (0.12f, 0.56f, 0.90f, 1);
    if (type == "ef") return Colour::fromFloatRGBA (0.42f, 0.38f, 1.0f, 1);
    if (type == "midi") return Colour::fromFloatRGBA (0.62f, 0.27f, 0.90f, 1);
    return Colour::fromFloatRGBA (0.98f, 0.84f, 0.18f, 1);   // più tipi: giallo
}
juce::Colour View::modColor (int t) const
{
    auto ty = m.targetSourceTypes (t);
    if (ty.isEmpty()) return juce::Colours::transparentBlack;
    if (ty.size() > 1) return srcColor ("multi");
    return srcColor (ty[0]);
}
int View::paramOf (bool band, int idOrOff) const { return band ? Model::bp (m.sel.empty() ? 0 : m.sel[0], idOrOff) : idOrOff; }

View::Reg& View::hit (float x, float y, float w, float h, Reg r)
{
    r.r = { x, y, w, h };
    REG.push_back (std::move (r));
    return REG.back();
}

// =================================================================== primitive
void View::iconPower (float x, float y, bool on)
{
    auto& c = *cv;
    c.lineWidth (1.6f);
    if (on) c.rgb (0.95f, 0.95f, 0.95f); else c.rgb (0.5f, 0.45f, 0.46f);
    c.arc (x, y, 4.5f, -0.35f * PI_F, 1.35f * PI_F); c.doStroke();
    c.line (x, y - 6, x, y - 1);
}
void View::iconSave (float x, float y)
{
    auto& c = *cv;
    c.rgb (0.8f, 0.78f, 0.78f); c.lineWidth (1.2f);
    c.rect (x - 4.5f, y - 4.5f, 9, 9); c.doStroke();
    c.rect (x - 2.5f, y - 4.5f, 5, 3); c.doFill();
}
void View::iconX (float x, float y, float s)
{
    cv->lineWidth (1.6f);
    cv->line (x - s, y - s, x + s, y + s);
    cv->line (x - s, y + s, x + s, y - s);
}
void View::triDown (float x, float y, float s)
{
    cv->moveTo (x - s, y - s / 2); cv->lineTo (x + s, y - s / 2); cv->lineTo (x, y + s / 2); cv->closePath(); cv->doFill();
}

float View::modDots (float x, float y, int t)
{
    auto ty = m.targetSourceTypes (t);
    for (int i = 0; i < ty.size(); ++i) { cv->col (srcColor (ty[i])); cv->circle (x + i * 4, y, 2.4f); cv->doFill(); }
    if (! ty.isEmpty())
    {
        Reg r; r.tgt = t; r.kind = "moddot";
        r.down = [this, t] (Reg& s, const M&) { openSlotPanelTarget (t, s.r.getX(), s.r.getY()); };
        hit (x - 4, y - 5, ty.size() * 4.0f + 6, 10, r);
    }
    return ty.size() * 4.0f;
}

void View::showTip (int pn, float x, float y)
{
    const auto& p = params()[(size_t) pn];
    const String g = p.group;
    String s = (g.startsWith ("xlfo") || g.startsWith ("eg") || g.startsWith ("ef") ? m.srcName (g) + " " + p.ui : String (p.ui)) + ": " + Model::fmtVal (pn, m.pv (pn));
    if (p.target >= 0 && ! m.slotsForTarget (p.target).empty() && engineAlive) s += "  (" + Model::fmtVal (pn, effVal (pn)) + ")";
    tip = { s, x, y, true };
}

void View::learnBalloon (int pn, float x, float y)
{
    int cc = -1;
    for (auto& kv : m.learn) if (kv.second == pn) cc = kv.first;
    const bool armed = m.learnArm == pn;
    if (cc < 0 && ! armed) return;
    const String s = cc < 0 ? "?" : "CC " + String (cc);
    cv->font (8, true);
    const float w = cv->textWidth (s) + 8;
    cv->rgb (armed ? 0.85f : 0.95f, armed ? 0.15f : 0.9f, armed ? 0.15f : 0.6f, 0.95f);
    cv->rrect (x - w / 2, y - 7, w, 12, 3); cv->doFill();
    cv->text (s, x, y + 2, 8, Colour::fromFloatRGBA (0.1f, 0.1f, 0.1f, 1), 'c', true);
}

// ---------------------------------------------------------------- manopola
View::Reg& View::knob (float cx, float cy, float r, int p, KOpt o)
{
    auto& c = *cv;
    const int pn = paramOf (o.band, p);
    const auto& P = params()[(size_t) pn];
    const double n = m.getNorm (pn);
    const float a0 = 0.75f * PI_F, a1 = 2.25f * PI_F;
    const bool hover = hoverReg && hoverReg->pname == pn && hoverReg->kind == "knob";
    if (o.ring)
    {
        c.lineWidth (r * 0.32f);
        c.rgb (0.35f, 0.07f, 0.09f); c.arc (cx, cy, r * 0.84f, a0, a1); c.doStroke();
        c.col (C_RED); c.arc (cx, cy, r * 0.84f, a0, a0 + (a1 - a0) * (float) n); c.doStroke();
    }
    else
    {
        c.lineWidth (2.2f);
        c.rgb (0.2f, 0.17f, 0.18f); c.arc (cx, cy, r + 2.5f, a0, a1); c.doStroke();
        c.col (o.color);
        if (o.bipolar)
        {
            const float am = 1.5f * PI_F, d = ((float) n - 0.5f) * 1.5f * PI_F;
            c.arc (cx, cy, r + 2.5f, std::min (am, am + d), std::max (am, am + d) + 0.001f);
        }
        else c.arc (cx, cy, r + 2.5f, a0, a0 + (a1 - a0) * (float) n + 0.001f);
        c.doStroke();
    }
    if (P.target >= 0)
    {
        auto mc = modColor (P.target);
        if (! mc.isTransparent())
        {
            auto rg = modRange (P.target, n);
            c.col (mc, 0.95f); c.lineWidth (2.5f);
            c.arc (cx, cy, r + (o.ring ? 1.0f : 6.0f), a0 + (a1 - a0) * (float) rg.first, a0 + (a1 - a0) * (float) std::max (rg.second, rg.first + 0.004));
            c.doStroke();
        }
    }
    const float body = o.ring ? r * 0.66f : r;
    auto gd = c.linear (cx, cy - body, cx, cy + body);
    gd.stop (0, hover ? 0.52f : 0.45f, hover ? 0.49f : 0.42f, hover ? 0.5f : 0.43f);
    gd.stop (1, 0.24f, 0.21f, 0.22f);
    c.source (gd); c.circle (cx, cy, body); c.doFill();
    c.rgb (0, 0, 0, 0.5f); c.lineWidth (1); c.circle (cx, cy, body); c.doStroke();
    const float ang = a0 + (a1 - a0) * (float) effNorm (pn);
    c.lineWidth (std::max (2.0f, r * 0.12f)); c.roundCap (true);
    c.rgb (1, 1, 1, 0.95f);
    c.line (cx + std::cos (ang) * body * 0.35f, cy + std::sin (ang) * body * 0.35f, cx + std::cos (ang) * body * 0.92f, cy + std::sin (ang) * body * 0.92f);
    c.roundCap (false);
    if (o.pan >= 0)
    {
        const int ppn = paramOf (o.band, o.pan);
        const float pnv = (float) m.getNorm (ppn), pr = r + 5;
        c.lineWidth (2); c.rgb (0.25f, 0.22f, 0.23f); c.arc (cx, cy, pr, 1.25f * PI_F, 1.75f * PI_F); c.doStroke();
        c.rgb (0.95f, 0.95f, 0.95f);
        const float pa = 1.5f * PI_F + (pnv - 0.5f) * 0.5f * PI_F;
        c.arc (cx, cy, pr, std::min (pa, 1.5f * PI_F) - 0.02f, std::max (pa, 1.5f * PI_F) + 0.02f); c.doStroke();
        c.text ("L", cx - r - 2, cy + r + 2, 7, C_DIM, 'c');
        c.text ("R", cx + r + 2, cy + r + 2, 7, C_DIM, 'c');
        Reg rr; rr.pname = ppn; rr.tgt = params()[(size_t) ppn].target; rr.kind = "pan"; rr.isBand = o.band; rr.off = o.pan;
        rr.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.n0 = m.getNorm (s.pname); s.mx0 = mm.x; };
        rr.drag = [this] (Reg& s, const M& mm)
        {
            const double v = Model::denorm (s.pname, s.n0 + (mm.x - s.mx0) / (mm.shift ? 600.0 : 120.0));
            if (s.isBand) m.setBandParam (s.off, v); else m.setParam (s.pname, v);
            showTip (s.pname, s.r.getCentreX(), s.r.getY());
        };
        rr.up = [this] (Reg&, const M&) { m.endEdit(); };
        rr.dbl = [this] (Reg& s, const M&) { startEntry (s.pname, s.r.getX(), s.r.getY(), s.isBand, s.off); };
        rr.reset = [this] (Reg& s, const M&) { m.beginEdit(); const double d = params()[(size_t) s.pname].def; if (s.isBand) m.setBandParam (s.off, d); else m.setParam (s.pname, d); m.endEdit(); };
        rr.wheel = [this] (Reg& s, const M& mm, float d) { m.beginEdit(); const double v = Model::denorm (s.pname, m.getNorm (s.pname) + d * (mm.shift ? 0.002 : 0.02)); if (s.isBand) m.setBandParam (s.off, v); else m.setParam (s.pname, v); m.endEdit(); };
        rr.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
        hit (cx - pr - 4, cy - pr - 5, (pr + 4) * 2, 9, rr);
    }
    if (o.label.isNotEmpty())
    {
        const float lw = c.text (o.label, cx, cy + r + (o.ring ? 12 : 14), 8.5f, C_TXT, 'c', true);
        if (P.target >= 0) modDots (cx + lw / 2 + 4, cy + r + (o.ring ? 9 : 11), P.target);
    }
    if (m.learnMode) learnBalloon (pn, cx, cy - r - 8);
    Reg k; k.pname = pn; k.tgt = P.target; k.kind = "knob"; k.cx = cx; k.cy = cy; k.rad = r; k.isBand = o.band; k.off = p;
    k.down = [this] (Reg& s, const M& mm)
    {
        m.beginEdit();
        s.n0 = m.getNorm (s.pname); s.my0 = mm.y;
        const float dx = mm.x - s.cx, dy = mm.y - s.cy;
        s.rot = std::sqrt (dx * dx + dy * dy) > s.rad * 0.72f;
    };
    k.drag = [this] (Reg& s, const M& mm)
    {
        double nn;
        if (s.rot)
        {
            float a = std::atan2 (mm.y - s.cy, mm.x - s.cx);
            if (a < 0.5f * PI_F) a += 2 * PI_F;
            nn = (a - 0.75 * PI_F) / (1.5 * PI_F);
            const float d = std::sqrt ((mm.x - s.cx) * (mm.x - s.cx) + (mm.y - s.cy) * (mm.y - s.cy));
            if (d > s.rad * 3) nn = s.n0 + (nn - s.n0) * s.rad * 3 / d;
        }
        else nn = s.n0 + (s.my0 - mm.y) / (mm.shift ? 1000.0 : 180.0);
        const double v = Model::denorm (s.pname, juce::jlimit (0.0, 1.0, nn));
        if (s.isBand) m.setBandParam (s.off, v); else m.setParam (s.pname, v);
        // Alt: ingresso e uscita collegati in senso opposto (come in Saturn 2)
        if (mm.alt && (s.pname == P::gain_in || s.pname == P::gain_out))
            m.setParam (s.pname == P::gain_in ? P::gain_out : P::gain_in, -v);
        showTip (s.pname, s.cx, s.cy - s.rad);
    };
    k.up = [this] (Reg&, const M&) { m.endEdit(); };
    k.dbl = [this] (Reg& s, const M&) { startEntry (s.pname, s.cx - 40, s.cy - 8, s.isBand, s.off); };
    k.reset = [this] (Reg& s, const M&) { m.beginEdit(); const double d = params()[(size_t) s.pname].def; if (s.isBand) m.setBandParam (s.off, d); else m.setParam (s.pname, d); m.endEdit(); };
    k.wheel = [this] (Reg& s, const M& mm, float d)
    {
        m.beginEdit();
        const double v = Model::denorm (s.pname, m.getNorm (s.pname) + d * (mm.shift ? 0.002 : 0.02));
        if (s.isBand) m.setBandParam (s.off, v); else m.setParam (s.pname, v);
        m.endEdit();
        showTip (s.pname, s.cx, s.cy - s.rad);
    };
    k.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.cx, s.cy - s.rad); };
    return hit (cx - r - 3, cy - r - 3, (r + 3) * 2, (r + 3) * 2, k);
}

// ---------------------------------------------------------------- cursore verticale
View::Reg& View::vslider (float x, float y, float h, int p, bool band)
{
    auto& c = *cv;
    const int pn = paramOf (band, p);
    const auto& P = params()[(size_t) pn];
    const double n = m.getNorm (pn), ne = effNorm (pn);
    c.rgb (0.3f, 0.27f, 0.28f); c.rrect (x - 1.5f, y + 2, 3, h - 4, 1.5f); c.doFill();
    c.rgb (1, 1, 1, 0.25f); c.rect (x - 5, y + h / 2 - 0.5f, 10, 1); c.doFill();
    const float hy = y + (float) (1 - ne) * (h - 8);
    if (P.target >= 0)
    {
        auto mc = modColor (P.target);
        if (! mc.isTransparent())
        {
            auto rg = modRange (P.target, n);
            c.col (mc); c.rect (x + 4, y + (float) (1 - rg.second) * (h - 8) + 4, 2, (float) (rg.second - rg.first) * (h - 8) + 1); c.doFill();
        }
    }
    auto gd = c.linear (x, hy, x, hy + 8);
    gd.stop (0, 0.75f, 0.72f, 0.73f); gd.stop (1, 0.45f, 0.42f, 0.43f);
    c.source (gd); c.rrect (x - 8, hy, 16, 8, 2); c.doFill();
    c.rgb (0.2f, 0.2f, 0.2f); c.lineWidth (1); c.line (x - 5, hy + 4, x + 5, hy + 4);
    if (m.learnMode) learnBalloon (pn, x, y - 4);
    Reg r; r.pname = pn; r.tgt = P.target; r.kind = "slider"; r.cx = x; r.cy = hy; r.isBand = band; r.off = p; r.rad = h;
    r.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.n0 = m.getNorm (s.pname); s.my0 = mm.y; };
    r.drag = [this] (Reg& s, const M& mm)
    {
        const double v = Model::denorm (s.pname, s.n0 + (s.my0 - mm.y) / (mm.shift ? 400.0 : (s.rad - 8)));
        if (s.isBand) m.setBandParam (s.off, v); else m.setParam (s.pname, v);
        showTip (s.pname, s.cx, s.r.getY() - 4);
    };
    r.up = [this] (Reg&, const M&) { m.endEdit(); };
    r.dbl = [this] (Reg& s, const M&) { startEntry (s.pname, s.cx - 40, s.r.getY(), s.isBand, s.off); };
    r.reset = [this] (Reg& s, const M&) { m.beginEdit(); const double d = params()[(size_t) s.pname].def; if (s.isBand) m.setBandParam (s.off, d); else m.setParam (s.pname, d); m.endEdit(); };
    r.wheel = [this] (Reg& s, const M&, float d) { m.beginEdit(); const double v = Model::denorm (s.pname, m.getNorm (s.pname) + d * 0.02); if (s.isBand) m.setBandParam (s.off, v); else m.setParam (s.pname, v); m.endEdit(); };
    r.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.cx, s.r.getY() - 4); };
    return hit (x - 9, y - 2, 18, h + 4, r);
}

View::Reg& View::button (float x, float y, float w, float h, const String& label, bool on, std::function<void()> fn, bool led, bool hasLed, float size)
{
    auto& c = *cv;
    const String bid = label + String (x);
    const bool hv = hoverReg && hoverReg->bid == bid;
    if (hv) c.rgb (on ? 0.5f : 0.36f, on ? 0.46f : 0.33f, on ? 0.47f : 0.34f);
    else c.rgb (on ? 0.42f : 0.28f, on ? 0.38f : 0.25f, on ? 0.39f : 0.26f);
    c.rrect (x, y, w, h, 3); c.doFill();
    if (hasLed)
    {
        if (led) c.rgb (0.2f, 0.85f, 0.35f); else c.rgb (0.2f, 0.2f, 0.2f);
        c.rect (x + w / 2 - 12, y - 1, 24, 2); c.doFill();
    }
    if (label.isNotEmpty()) c.text (label, x + w / 2, y + h / 2 + 3.5f, size, C_TXT, 'c', true);
    Reg r; r.bid = bid; r.kind = "button";
    r.down = [fn] (Reg&, const M&) { fn(); };
    return hit (x, y, w, h, r);
}

View::Reg& View::smallBtn (float x, float y, float w, const String& label, std::function<void()> fn, bool on)
{
    cv->rgb (on ? 0.42f : 0.26f, on ? 0.38f : 0.23f, on ? 0.39f : 0.24f);
    cv->rrect (x, y, w, 14, 3); cv->doFill();
    if (label.isNotEmpty()) cv->text (label, x + w / 2, y + 10, 8, C_TXT, 'c', true);
    Reg r; r.kind = "button"; r.bid = "sb" + label + String (x) + String (y);
    r.down = [fn] (Reg&, const M&) { fn(); };
    return hit (x, y, w, 14, r);
}

// =================================================================== intestazione
void View::drawHeader()
{
    auto& c = *cv;
    auto gd = c.linear (0, 0, 0, HDR);
    gd.stop (0, 0.2f, 0.13f, 0.14f); gd.stop (1, 0.12f, 0.08f, 0.09f);
    c.source (gd); c.rect (0, 0, BW, HDR); c.doFill();
    c.rgb (0.18f, 0.12f, 0.13f); c.rrect (8, 4, 170, 26, 13); c.doFill();
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
        {
            c.rgb (0.85f, 0.8f, 0.8f, (i + j) % 2 ? 0.9f : 0.35f);
            c.circle (20.0f + i * 6, 11.0f + j * 6, 1.8f); c.doFill();
        }
    c.text ("SATURA", 44, 22, 15, Colour::fromFloatRGBA (0.95f, 0.92f, 0.92f, 1), 'l', true);
    c.text ("multiband saturation", 112, 21, 7.5f, Colour::fromFloatRGBA (0.78f, 0.55f, 0.45f, 1));
    const Colour off = Colour::fromFloatRGBA (0.35f, 0.32f, 0.33f, 1);
    // undo / redo
    c.col (m.canUndo() ? C_TXT : off); c.lineWidth (1.5f);
    c.arc (238, 18, 5, 1.1f * PI_F, 2.6f * PI_F); c.doStroke();
    c.moveTo (231, 13); c.lineTo (234, 19); c.lineTo (237, 13); c.closePath(); c.doFill();
    { Reg r; r.tipText = "Undo"; r.kind = "button"; r.bid = "undo"; r.down = [this] (Reg&, const M&) { m.doUndo(); }; hit (228, 8, 20, 20, r); }
    c.col (m.canRedo() ? C_TXT : off);
    c.arc (266, 18, 5, 0.4f * PI_F, 1.9f * PI_F); c.doStroke();
    c.moveTo (273, 13); c.lineTo (270, 19); c.lineTo (267, 13); c.closePath(); c.doFill();
    { Reg r; r.tipText = "Redo"; r.kind = "button"; r.bid = "redo"; r.down = [this] (Reg&, const M&) { m.doRedo(); }; hit (256, 8, 20, 20, r); }
    // A/B Copy
    c.text ("A", 296, 22, 11, m.abCur == 0 ? juce::Colours::white : C_DIM, 'l', true);
    c.text ("/", 305, 22, 11, C_DIM);
    c.text ("B", 310, 22, 11, m.abCur == 1 ? juce::Colours::white : C_DIM, 'l', true);
    { Reg r; r.tipText = "A/B"; r.kind = "button"; r.bid = "ab"; r.down = [this] (Reg&, const M&) { m.abSwitch(); }; hit (292, 8, 30, 20, r); }
    c.text ("Copy", 330, 22, 10, (m.abDirty || ! m.abCopied) ? C_TXT : off);
    { Reg r; r.tipText = "Copy"; r.kind = "button"; r.bid = "copy"; r.down = [this] (Reg&, const M&) { m.abCopy(); }; hit (326, 8, 30, 20, r); }
    // preset
    c.rgb (0.75f, 0.72f, 0.72f);
    c.moveTo (392, 18); c.lineTo (398, 13); c.lineTo (398, 23); c.closePath(); c.doFill();
    { Reg r; r.kind = "button"; r.bid = "prev"; r.down = [this] (Reg&, const M&) { m.stepPreset (-1); }; hit (386, 8, 16, 20, r); }
    c.moveTo (668, 18); c.lineTo (662, 13); c.lineTo (662, 23); c.closePath(); c.doFill();
    { Reg r; r.kind = "button"; r.bid = "next"; r.down = [this] (Reg&, const M&) { m.stepPreset (1); }; hit (658, 8, 16, 20, r); }
    const bool fl = flashUntil > juce::Time::getMillisecondCounterHiRes();
    c.text (fl ? flashText : m.str.presetName, 530, 22, 11, fl ? Colour::fromFloatRGBA (1, 0.85f, 0.5f, 1) : (m.presetDirty ? C_DIM : C_TXT), 'c');
    {
        Reg r; r.kind = "button"; r.bid = "preset";
        r.down = [this] (Reg&, const M&) { openPresetMenu (410, HDR); };
        r.rdown = [this] (Reg&, const M&) { openPresetShortMenu (410, HDR); };
        hit (410, 6, 240, 22, r);
    }
    c.text ("Help", 822, 22, 10, C_TXT);
    { Reg r; r.kind = "button"; r.bid = "help"; r.down = [this] (Reg&, const M&) { openHelpMenu (780, HDR); }; hit (816, 8, 34, 20, r); }
    c.rgb (0.8f, 0.78f, 0.78f); c.lineWidth (1.2f);
    const float x0 = 868, y0 = 11;
    c.line (x0, y0, x0 + 4, y0); c.line (x0, y0, x0, y0 + 4);
    c.line (x0 + 12, y0, x0 + 8, y0); c.line (x0 + 12, y0, x0 + 12, y0 + 4);
    c.line (x0, y0 + 12, x0 + 4, y0 + 12); c.line (x0, y0 + 12, x0, y0 + 8);
    c.line (x0 + 12, y0 + 12, x0 + 8, y0 + 12); c.line (x0 + 12, y0 + 12, x0 + 12, y0 + 8);
    { Reg r; r.tipText = "Full Screen"; r.kind = "button"; r.bid = "fs"; r.down = [this] (Reg&, const M&) { if (onToggleFullscreen) onToggleFullscreen(); }; hit (862, 6, 26, 22, r); }
}

// =================================================================== display multibanda
void View::drawDisplay()
{
    auto& c = *cv;
    const float y0 = DY, h = DH;
    const int n = m.nb();
    auto gd = c.linear (0, y0, 0, y0 + h);
    gd.stop (0, 0.16f, 0.07f, 0.08f); gd.stop (0.6, 0.11f, 0.05f, 0.06f); gd.stop (1, 0.07f, 0.04f, 0.05f);
    c.source (gd); c.rect (0, y0, BW, h); c.doFill();
    {
        Reg r; r.kind = "dispbg";
        r.down = [this] (Reg& s, const M& mm) { s.mx0 = mm.x; s.my0 = mm.y; selRectOn = false; };
        r.drag = [this] (Reg& s, const M& mm)
        {
            selRect = { (float) std::min (s.mx0, (double) mm.x), (float) std::min (s.my0, (double) mm.y), (float) std::abs (mm.x - s.mx0), (float) std::abs (mm.y - s.my0) };
            selRectOn = true;
            selectInRect();
        };
        r.up = [this] (Reg&, const M& mm)
        {
            if (! selRectOn || selRect.getWidth() < 4) m.sel = { m.bandAtFreq (xf (mm.x)) };
            selRectOn = false; slotp.on = false; repaint();
        };
        r.wheel = [this] (Reg&, const M& mm, float d)
        {
            const int bb = m.bandAtFreq (xf (mm.x));
            if (std::find (m.sel.begin(), m.sel.end(), bb) == m.sel.end()) m.sel = { bb };
            m.beginEdit();
            const int off = mm.alt ? BP::drive : BP::level;
            m.setBandParam (off, m.pv (Model::bp (bb, off)) + d * (mm.shift ? 0.1 : 1.0));
            m.endEdit();
        };
        hit (0, y0, BW, h, r);
    }
    c.lineWidth (1);
    for (double f : FREQ_GRID)
    {
        const float gx = fx (f);
        c.rgb (1, 1, 1, 0.045f); c.line (gx, y0, gx, y0 + h);
        c.text (f >= 1000 ? String ((int) (f / 1000)) + "k" : String ((int) f), gx + 3, y0 + h - 5, 7.5f, Colour::fromFloatRGBA (1, 1, 1, 0.22f));
    }
    // spettro dell'uscita
    if (! specDb.empty() && m.pv (P::bypass) < 0.5)
    {
        c.moveTo (0, y0 + h);
        for (size_t i = 0; i < specDb.size(); ++i) c.lineTo ((float) i * BW / (float) (specDb.size() - 1), specY (specDb[i]));
        c.lineTo (BW, y0 + h); c.closePath();
        c.rgb (0.95f, 0.55f, 0.5f, 0.07f); c.fillKeep();
        c.rgb (0.95f, 0.65f, 0.6f, 0.35f); c.lineWidth (1); c.doStroke();
    }
    for (int b = 0; b < n; ++b)
    {
        double lo, hi; m.bandEdges (b, lo, hi);
        const float x0 = b == 0 ? 0 : fx (lo), x1 = b == n - 1 ? (float) BW : fx (hi);
        const bool sel = std::find (m.sel.begin(), m.sel.end(), b) != m.sel.end();
        const double lv = effVal (Model::bp (b, BP::level));
        const float ly = levY (lv);
        const bool en = m.pv (Model::bp (b, BP::en)) > 0.5;
        float a = sel ? 0.42f : 0.18f;
        if (! en) a *= 0.4f;
        auto gb = c.linear (0, ly, 0, y0 + h);
        gb.stop (0, 0.75f, 0.12f, 0.16f, a); gb.stop (1, 0.4f, 0.05f, 0.08f, a * 0.25f);
        c.source (gb); c.rect (x0, ly, x1 - x0, y0 + h - ly); c.doFill();
        c.rgb (0.95f, 0.5f, 0.52f, sel ? 0.75f : 0.35f); c.lineWidth (1); c.line (x0, ly, x1, ly);
        const float cx = (x0 + x1) / 2;
        drawLevelHandle (b, cx, ly, sel);
        if (n > 1 && ((hoverY >= y0 && hoverY < y0 + 60) || m.pv (Model::bp (b, BP::solo)) > 0.5 || m.pv (Model::bp (b, BP::mute)) > 0.5))
            drawSoloMute (b, x0 + 6, y0 + 8);
        juce::StringArray types;
        for (int j = 0; j < NBP; ++j)
        {
            const int t = params()[(size_t) Model::bp (b, j)].target;
            if (t >= 0) for (auto& ty : m.targetSourceTypes (t)) types.addIfNotAlreadyThere (ty);
        }
        for (int i = 0; i < types.size(); ++i) { c.col (srcColor (types[i])); c.circle (cx - (types.size() - 1) * 2.5f + i * 5, y0 + 10, 2.5f); c.doFill(); }
        if (! types.isEmpty())
        {
            Reg r; r.band = b; r.kind = "bandmod";
            r.down = [this] (Reg& s, const M&) { openSlotPanelBand (s.band, s.r.getX(), s.r.getY() + 12); };
            hit (cx - 12, y0 + 4, 24, 12, r);
        }
    }
    for (int i = 0; i < n - 1; ++i) drawCrossover (i);
    // pulsante "+"
    if (hoverY >= y0 && hoverY < y0 + 26 && hoverX > 0 && n < MAXBANDS && ! drag.on)
    {
        bool near = false;
        for (int i = 0; i < n - 1; ++i) if (std::abs (fx (m.pv (P::xf1 + i)) - hoverX) < 14) near = true;
        if (! near)
        {
            const float px = hoverX;
            c.rgb (0.9f, 0.88f, 0.88f, 0.9f); c.rrect (px - 8, y0 + 5, 16, 16, 3); c.doFill();
            c.rgb (0.15f, 0.1f, 0.1f); c.lineWidth (2);
            c.line (px - 4, y0 + 13, px + 4, y0 + 13); c.line (px, y0 + 9, px, y0 + 17);
            Reg r; r.kind = "addband"; r.cx = px;
            r.down = [this] (Reg& s, const M&) { m.addBandAt (xf (s.cx)); };
            hit (px - 9, y0 + 4, 18, 18, r);
        }
    }
    if (selRectOn)
    {
        c.rgb (1, 1, 1, 0.08f); c.rect (selRect.getX(), selRect.getY(), selRect.getWidth(), selRect.getHeight()); c.doFill();
        c.rgb (1, 1, 1, 0.35f); c.lineWidth (1); c.rect (selRect.getX(), selRect.getY(), selRect.getWidth(), selRect.getHeight()); c.doStroke();
    }
}

void View::selectInRect()
{
    const int n = m.nb();
    std::vector<int> s;
    for (int b = 0; b < n; ++b)
    {
        double lo, hi; m.bandEdges (b, lo, hi);
        const float x0 = b == 0 ? 0 : fx (lo), x1 = b == n - 1 ? (float) BW : fx (hi);
        if (x1 > selRect.getX() && x0 < selRect.getRight()) s.push_back (b);
    }
    if (! s.empty()) m.sel = s;
}

void View::drawLevelHandle (int b, float cx, float ly, bool sel)
{
    auto& c = *cv;
    const float w = 30, h = 12;
    auto gd = c.linear (cx, ly - h / 2, cx, ly + h / 2);
    gd.stop (0, sel ? 0.92f : 0.66f, sel ? 0.9f : 0.63f, sel ? 0.9f : 0.64f);
    gd.stop (1, sel ? 0.62f : 0.42f, sel ? 0.6f : 0.4f, sel ? 0.6f : 0.41f);
    c.source (gd); c.rrect (cx - w / 2, ly - h / 2, w, h, 3); c.doFill();
    c.rgb (0.2f, 0.18f, 0.18f); c.lineWidth (1); c.line (cx - 8, ly, cx + 8, ly);
    const int pn = Model::bp (b, BP::level);
    if (m.learnMode) learnBalloon (pn, cx, ly - 12);
    Reg r; r.kind = "levelhandle"; r.band = b; r.pname = pn; r.tgt = params()[(size_t) pn].target; r.cx = cx; r.cy = ly;
    r.down = [this] (Reg& s, const M& mm)
    {
        if (std::find (m.sel.begin(), m.sel.end(), s.band) == m.sel.end()) m.sel = { s.band };
        m.beginEdit();
        s.mx0 = mm.x; s.my0 = mm.y; s.all = mm.alt;
        s.v0 = m.pv (Model::bp (s.band, s.all ? BP::drive : BP::level));
        double lo, hi; m.bandEdges (s.band, lo, hi);
        s.vs = { lo, hi };
    };
    r.drag = [this] (Reg& s, const M& mm)
    {
        const int off = s.all ? BP::drive : BP::level;
        const double dy = (s.my0 - mm.y) * (mm.shift ? 0.1 : 1.0);
        m.setBandParam (off, s.v0 + dy * (s.all ? 0.25 : 72.0 / 210.0));
        const double dx = mm.x - s.mx0;
        if (std::abs (dx) > 3 && m.nb() > 1)
        {
            const double ratio = std::pow (3000.0, dx / BW);
            if (s.band > 0) m.moveXover (s.band - 1, s.vs[0] * ratio);
            if (s.band < m.nb() - 1) m.moveXover (s.band, s.vs[1] * ratio);
        }
        showTip (Model::bp (s.band, off), s.cx, s.cy - 10);
    };
    r.up = [this] (Reg&, const M&) { m.endEdit(); };
    r.dbl = [this] (Reg& s, const M&) { m.sel = { s.band }; startEntry (s.pname, s.cx - 40, s.cy - 8, true, BP::level); };
    r.reset = [this] (Reg& s, const M&) { m.beginEdit(); m.sel = { s.band }; m.setBandParam (BP::level, 0); m.endEdit(); };
    r.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.cx, s.cy - 10); };
    hit (cx - w / 2 - 2, ly - h / 2 - 2, w + 4, h + 4, r);
}

void View::drawCrossover (int i)
{
    auto& c = *cv;
    const int pn = P::xf1 + i;
    const float x = fx (effVal (pn)), y0 = DY, h = DH;
    c.rgb (1, 1, 1, 0.35f); c.lineWidth (1);
    for (float y = y0 + 50; y < y0 + h; y += 4) c.line (x, y, x, std::min (y + 2, y0 + h));
    auto mc = modColor (params()[(size_t) pn].target);
    if (! mc.isTransparent()) { c.col (mc); c.circle (x, y0 + 30, 2.5f); c.doFill(); }
    {
        Reg r; r.kind = "xover"; r.pname = pn; r.tgt = params()[(size_t) pn].target; r.idx = i; r.cx = x; r.cy = y0 + 60;
        r.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.mx0 = mm.x; s.v0 = m.pv (s.pname); };
        r.drag = [this] (Reg& s, const M& mm) { m.moveXover (s.idx, s.v0 * std::pow (3000.0, (mm.x - s.mx0) / BW * (mm.shift ? 0.1 : 1.0))); showTip (s.pname, s.cx, s.cy); };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.dbl = [this] (Reg& s, const M&) { startEntry (s.pname, s.cx - 40, s.cy); };
        r.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.cx, s.cy); };
        r.wheel = [this] (Reg& s, const M&, float d) { m.beginEdit(); m.moveXover (s.idx, m.pv (s.pname) * std::pow (2.0, d / 24.0)); m.endEdit(); };
        r.reset = [this] (Reg& s, const M&) { m.beginEdit(); m.moveXover (s.idx, params()[(size_t) s.pname].def); m.endEdit(); };
        hit (x - 5, y0 + 50, 10, h - 50, r);
    }
    const int sl = (int) std::lround (m.pv (P::xs1 + i));
    c.rgb (0.3f, 0.24f, 0.25f, 0.95f); c.rrect (x - 8, y0 + 34, 16, 14, 3); c.doFill();
    c.rgb (0.9f, 0.86f, 0.86f); c.lineWidth (1);
    for (int j = 0; j <= sl; ++j)
    {
        const float w = 2 + j * 1.3f;
        c.moveTo (x - w, y0 + 45); c.lineTo (x, y0 + 37 + j * 0.5f); c.lineTo (x + w, y0 + 45); c.doStroke();
    }
    Reg r; r.idx = i; r.kind = "button"; r.bid = "slope" + String (i); r.tipText = "Crossover Slope";
    r.down = [this] (Reg& s, const M&)
    {
        juce::PopupMenu pm;
        const int ii = s.idx;
        const char* labels[] = { "6 dB/oct", "12 dB/oct", "24 dB/oct", "48 dB/oct" };
        for (int j = 0; j < 4; ++j)
            pm.addItem (labels[j], true, (int) std::lround (m.pv (P::xs1 + ii)) == j, [this, ii, j] { m.pushUndo(); m.setParam (P::xs1 + ii, j); });
        showMenu (pm, s.r.getX(), s.r.getBottom() + 2);
    };
    hit (x - 9, y0 + 33, 18, 16, r);
}

void View::drawSoloMute (int b, float x, float y)
{
    auto& c = *cv;
    const bool s = m.pv (Model::bp (b, BP::solo)) > 0.5, mu = m.pv (Model::bp (b, BP::mute)) > 0.5;
    c.rgb (s ? 0.85f : 0.25f, s ? 0.15f : 0.2f, s ? 0.15f : 0.2f, 0.95f); c.rrect (x, y, 13, 12, 2); c.doFill();
    c.text ("S", x + 6.5f, y + 9.5f, 8.5f, s ? juce::Colours::white : C_DIM, 'c', true);
    c.rgb (mu ? 0.3f : 0.25f, mu ? 0.45f : 0.2f, mu ? 0.8f : 0.2f, 0.95f); c.rrect (x, y + 15, 13, 12, 2); c.doFill();
    c.text ("M", x + 6.5f, y + 24.5f, 8.5f, mu ? juce::Colours::white : C_DIM, 'c', true);
    for (int k = 0; k < 2; ++k)
    {
        Reg r; r.band = b; r.kind = k == 0 ? "solo" : "mute"; r.idx = k == 0 ? BP::solo : BP::mute;
        r.down = [this] (Reg& s2, const M& mm) { soloMute (s2.band, s2.idx, mm); };
        r.up = [this] (Reg&, const M&) { soloMuteUp(); };
        hit (x, y + k * 15, 13, 12, r);
    }
}

// =================================================================== controlli di banda
float View::bandPanelX() const
{
    const int n = m.nb();
    float s = 0; int cnt = 0;
    for (int b : m.sel)
    {
        double lo, hi; m.bandEdges (b, lo, hi);
        const float x0 = b == 0 ? 0 : fx (lo), x1 = b == n - 1 ? (float) BW : fx (hi);
        s += (x0 + x1) / 2; ++cnt;
    }
    const float cx = cnt ? s / cnt : BW / 2.0f;
    return clampf (cx - PANW / 2, 12, BW - PANW - 12);
}

void View::drawBandPanel()
{
    auto& c = *cv;
    if (m.sel.empty() || m.sel[0] >= m.nb()) m.sel = { 0 };
    const float x0 = bandPanelX(), y0 = PANY;
    const int b = m.sel[0];
    c.rgb (0.08f, 0.05f, 0.06f, 0.82f); c.rrect (x0, y0, PANW, PANH, 6); c.doFill();
    c.rgb (1, 1, 1, 0.07f); c.lineWidth (1); c.rrect (x0 + 0.5f, y0 + 0.5f, PANW - 1, PANH - 1, 6); c.doStroke();
    { Reg r; r.kind = "panelbg"; r.down = [] (Reg&, const M&) {}; hit (x0, y0, PANW, PANH, r); }
    const bool en = m.pv (Model::bp (b, BP::en)) > 0.5;
    c.rgb (0.28f, 0.24f, 0.25f); c.rrect (x0 + 10, y0 + 7, 22, 15, 3); c.doFill();
    iconPower (x0 + 21, y0 + 15, en);
    { Reg r; r.kind = "button"; r.bid = "bandpower"; r.tipText = "Enabled"; r.pname = Model::bp (b, BP::en);
      r.down = [this] (Reg&, const M&) { m.pushUndo(); m.setBandParam (BP::en, m.pv (Model::bp (m.sel[0], BP::en)) > 0.5 ? 0 : 1); }; hit (x0 + 10, y0 + 7, 22, 15, r); }
    c.rgb (0.28f, 0.24f, 0.25f); c.rrect (x0 + 36, y0 + 7, 22, 15, 3); c.doFill();
    iconSave (x0 + 47, y0 + 14.5f);
    { Reg r; r.kind = "button"; r.bid = "bandpresets"; r.tipText = "Band Presets";
      r.down = [this] (Reg& s, const M&) { openSectionMenu ("band", m.sel[0], s.r.getX(), s.r.getBottom() + 3); }; hit (x0 + 36, y0 + 7, 22, 15, r); }
    const int sty = juce::jlimit (0, NSTYLES - 1, (int) std::lround (m.pv (Model::bp (b, BP::style))));
    auto gd = c.linear (0, y0 + 7, 0, y0 + 22);
    gd.stop (0, 0.5f, 0.46f, 0.47f); gd.stop (1, 0.36f, 0.33f, 0.34f);
    c.source (gd); c.rrect (x0 + 64, y0 + 7, 130, 15, 3); c.doFill();
    c.text (styles()[(size_t) sty].name, x0 + 129, y0 + 18, 9, Colour::fromFloatRGBA (0.98f, 0.96f, 0.96f, 1), 'c', true);
    {
        Reg r; r.kind = "style"; r.pname = Model::bp (b, BP::style);
        r.down = [this] (Reg& s, const M&) { openStyleMenu (s.r.getX(), s.r.getBottom() + 2); };
        r.wheel = [this] (Reg&, const M&, float d)
        {
            m.pushUndo();
            m.setBandParam (BP::style, juce::jlimit (0, NSTYLES - 1, (int) std::lround (m.pv (Model::bp (m.sel[0], BP::style))) - (d > 0 ? 1 : -1)));
        };
        hit (x0 + 64, y0 + 7, 130, 15, r);
    }
    if (m.nb() > 1)
    {
        c.rgb (0.3f, 0.26f, 0.27f); c.rrect (x0 + PANW - 30, y0 + 7, 20, 15, 3); c.doFill();
        c.rgb (0.9f, 0.88f, 0.88f); iconX (x0 + PANW - 20, y0 + 14.5f, 3.5f);
        Reg r; r.kind = "button"; r.bid = "removeband"; r.tipText = "Remove Band";
        r.down = [this] (Reg&, const M&) { m.removeBands (m.sel); };
        hit (x0 + PANW - 30, y0 + 7, 20, 15, r);
    }
    const float ky = y0 + 52;
    KOpt o; o.band = true;
    o.label = "MIX"; knob (x0 + 34, ky, 13, BP::mix, o);
    o.label = "FEEDBACK"; knob (x0 + 88, ky, 13, BP::fba, o);
    o.label = "FREQ"; knob (x0 + 140, ky, 13, BP::fbf, o);
    c.rgb (1, 1, 1, 0.3f); c.lineWidth (1); c.line (x0 + 108, ky + 24, x0 + 124, ky + 24);
    KOpt od = o; od.label = "DYNAMICS"; od.bipolar = true; od.color = Colour::fromFloatRGBA (0.2f, 0.85f, 0.8f, 1);
    knob (x0 + 196, ky, 13, BP::dyn, od);
    KOpt dr; dr.band = true; dr.ring = true; dr.pan = BP::dpan; dr.label = "DRIVE";
    knob (x0 + 268, ky - 4, 30, BP::drive, dr);
    const float tx = x0 + 330;
    vslider (tx, y0 + 12, 64, BP::bass, true);
    vslider (tx + 22, y0 + 12, 64, BP::mid, true);
    vslider (tx + 44, y0 + 12, 64, BP::treb, true);
    vslider (tx + 66, y0 + 12, 64, BP::pres, true);
    const float lw2 = c.text ("TONE", tx + 33, ky + 34, 8.5f, C_TXT, 'c', true);
    float tdx = tx + 33 + lw2 / 2 + 4;
    for (int off : { BP::bass, BP::mid, BP::treb, BP::pres }) tdx += modDots (tdx, ky + 31, params()[(size_t) Model::bp (b, off)].target);
    KOpt lv; lv.band = true; lv.ring = true; lv.pan = BP::lpan; lv.label = "LEVEL";
    knob (x0 + 470, ky - 2, 20, BP::level, lv);
}

// =================================================================== sezione di modulazione
juce::StringArray View::visibleSources() const
{
    juce::StringArray r;
    for (auto& key : m.str.order)
    {
        String t; int k; Model::srcKeyParts (key, t, k);
        if (! m.srcActive (key)) continue;
        if (! m.showMod && t != "xy") continue;
        r.add (key);
    }
    return r;
}
bool View::isExpanded (const String& key) const
{
    if (m.autoCollapse) return m.expanded == key;
    return m.expList.contains (key);
}
void View::expandSource (const String& key)
{
    if (m.autoCollapse) m.expanded = m.expanded == key ? String() : key;
    else
    {
        if (m.expList.contains (key)) { m.expList.removeString (key); if (m.expanded == key) m.expanded = {}; }
        else { m.expList.add (key); m.expanded = key; }
    }
    selStep.clear();
    repaint();
}
void View::collapseSource (const String& key)
{
    m.expList.removeString (key);
    if (m.expanded == key) m.expanded = {};
    repaint();
}

static float srcW (const String& t) { return t == "xlfo" ? 520.0f : t == "eg" ? 390.0f : t == "ef" ? 280.0f : 160.0f; }

void View::drawModSection()
{
    auto& c = *cv;
    const float y0 = MODY;
    auto gd = c.linear (0, FLOWY, 0, BOTY);
    gd.stop (0, 0.09f, 0.07f, 0.08f); gd.stop (1, 0.12f, 0.1f, 0.11f);
    c.source (gd); c.rect (0, FLOWY, BW, BOTY - FLOWY); c.doFill();
    {
        Reg r; r.kind = "modbg";
        r.down = [this] (Reg&, const M&) { if (! drag.on) { m.expanded = {}; m.expList.clear(); slotp.on = false; repaint(); } };
        hit (0, FLOWY, BW, BOTY - FLOWY, r);
    }
    srcPos.clear();
    auto keys = visibleSources();
    std::vector<float> widths;
    float tot = 0; int ncol = 0;
    for (auto& key : keys)
    {
        String t; int k; Model::srcKeyParts (key, t, k);
        const bool ex = isExpanded (key) && t != "xy";
        const float w = t == "xy" ? (m.str.xy[(size_t) k].mode == 1 ? 36.0f : 70.0f) : (ex ? srcW (t) : 64.0f);
        if (! ex && t != "xy") ++ncol;
        widths.push_back (w); tot += w + 18;
    }
    float gap = 18, shrink = 0;
    const float avail = BW - 18 - 70;
    if (tot > avail)
    {
        gap = 10;
        tot -= (18 - gap) * keys.size();
        if (tot > avail && ncol) shrink = std::min (28.0f, (tot - avail) / ncol);
    }
    float x = 18;
    for (int i = 0; i < keys.size(); ++i)
    {
        const String key = keys[i];
        String t; int k; Model::srcKeyParts (key, t, k);
        const bool ex = isExpanded (key) && t != "xy";
        float w = widths[(size_t) i];
        if (! ex && t != "xy") w -= shrink;
        if (ex) drawSourceExpanded (key, x, y0 + 22, w);
        else drawSourceButton (key, x, y0 + 22, w);
        x += w + gap;
    }
    c.rgb (0.25f, 0.22f, 0.23f); c.rrect (x, y0 + 40, 26, 26, 4); c.doFill();
    c.rgb (0.8f, 0.78f, 0.78f); c.lineWidth (2);
    c.line (x + 7, y0 + 53, x + 19, y0 + 53); c.line (x + 13, y0 + 47, x + 13, y0 + 59);
    { Reg r; r.kind = "button"; r.bid = "addsrc"; r.tipText = "Add Modulation Source"; r.down = [this] (Reg& s, const M&) { openAddMenu (s.r.getX(), s.r.getBottom() + 2); }; hit (x, y0 + 40, 26, 26, r); }
    c.rgb (0.25f, 0.22f, 0.23f); c.rrect (x + 32, y0 + 40, 26, 26, 4); c.doFill();
    c.text (m.showMod ? juce::CharPointer_UTF8 ("\xc2\xab") : juce::CharPointer_UTF8 ("\xc2\xbb"), x + 45, y0 + 58, 14, Colour::fromFloatRGBA (0.8f, 0.78f, 0.78f, 1), 'c', true);
    { Reg r; r.kind = "button"; r.bid = "showmod"; r.tipText = "Show/Hide Modulation"; r.down = [this] (Reg&, const M&) { m.showMod = ! m.showMod; repaint(); }; hit (x + 32, y0 + 40, 26, 26, r); }
    if (keys.isEmpty()) c.text ("Click + to add a modulation source", x + 70, y0 + 57, 9, Colour::fromFloatRGBA (1, 1, 1, 0.25f));
    if (m.showFlow) drawFlow();
}

void View::dragDot (const String& key, const String& axis, float x, float y)
{
    auto& c = *cv;
    String t; int k; Model::srcKeyParts (key, t, k);
    const int si = Model::srcIndexOf (key, axis.isEmpty() ? "y" : axis);
    const auto col = srcColor (t);
    c.col (col); c.circle (x, y, 6.5f); c.doFill();
    c.rgb (0.08f, 0.06f, 0.07f); c.circle (x, y, 3.2f); c.doFill();
    bool used = false;
    for (auto& s : m.str.slots) if (s.src == si) used = true;
    if (used) { c.col (col); c.circle (x, y, 1.8f); c.doFill(); }
    srcPos[si] = { x, y };
    Reg r; r.kind = "srcdot"; r.si = si; r.srckey = key;
    r.down = [this] (Reg& s, const M& mm) { drag = {}; drag.on = true; drag.si = s.si; drag.key = s.srckey; drag.x0 = s.r.getCentreX(); drag.y0 = s.r.getCentreY(); drag.x = mm.x; drag.y = mm.y; };
    r.drag = [this] (Reg&, const M& mm)
    {
        drag.x = mm.x; drag.y = mm.y;
        if (std::abs (mm.x - drag.x0) + std::abs (mm.y - drag.y0) > 4) drag.moved = true;
        dragHover (mm);
    };
    r.up = [this] (Reg&, const M& mm) { dropSource (mm); };
    hit (x - 8, y - 8, 16, 16, r);
}

void View::sourceLabel (const String& key, float cx, float y)
{
    auto& c = *cv;
    const float w = c.text (m.srcName (key), cx - 5, y, 8.5f, C_TXT, 'c', true);
    c.rgb (0.6f, 0.56f, 0.57f); triDown (cx + w / 2, y - 3, 2.6f);
    Reg r; r.kind = "srclabel"; r.srckey = key;
    r.down = [this] (Reg& s, const M&) { openSourceMenu (s.srckey, s.r.getX(), s.r.getBottom() + 2); };
    r.dbl = [this] (Reg& s, const M&) { startNameEntry (s.srckey, s.r.getX(), s.r.getY()); };
    hit (cx - w / 2 - 6, y - 10, w + 16, 13, r);
}

void View::drawSourceButton (const String& key, float x, float y, float w)
{
    auto& c = *cv;
    String t; int k; Model::srcKeyParts (key, t, k);
    const float h = 64;
    if (t == "xy") { drawXY (key, x, y, w, h); return; }
    dragDot (key, {}, x + w / 2, y - 12);
    const bool hv = hoverReg && hoverReg->srckey == key && hoverReg->kind == "srcbtn";
    c.rgb (hv ? 0.2f : 0.16f, hv ? 0.17f : 0.13f, hv ? 0.18f : 0.14f); c.rrect (x, y, w, h, 6); c.doFill();
    c.rgb (1, 1, 1, 0.06f); c.lineWidth (1); c.rrect (x + 0.5f, y + 0.5f, w - 1, h - 1, 6); c.doStroke();
    const auto col = srcColor (t);
    c.col (col); c.lineWidth (1.6f);
    auto pts = sourceShape (key, 40);
    for (size_t i = 0; i < pts.size(); ++i)
    {
        const float px = x + 10 + i * (w - 20) / (pts.size() - 1), py = y + h / 2 - (float) pts[i] * (h / 2 - 12);
        if (i == 0) c.moveTo (px, py); else c.lineTo (px, py);
    }
    c.doStroke();
    if (t == "midi") c.text (MIDI_INPUTS[juce::jlimit (0, 5, m.str.midi[(size_t) k].input)], x + w / 2, y + h - 8, 7.5f, col, 'c', true);
    sourceLabel (key, x + w / 2, y + h + 13);
    int pm = -1;
    if (t == "xlfo") pm = m.str.xlfo[(size_t) k].sync ? P::x1_ofs + k * 6 : P::x1_rate + k * 6;
    else if (t == "eg") pm = P::e1_att + k * 7;
    else if (t == "ef") pm = P::f1_rel + k * 2;
    Reg r; r.kind = "srcbtn"; r.srckey = key; r.pname = pm;
    r.down = [this] (Reg& s, const M& mm) { s.my0 = mm.y; s.moved = false; if (s.pname >= 0) { m.beginEdit(); s.n0 = m.getNorm (s.pname); } };
    r.drag = [this] (Reg& s, const M& mm)
    {
        if (std::abs (mm.y - s.my0) > 2) s.moved = true;
        if (s.pname >= 0 && s.moved) { m.setParamNorm (s.pname, s.n0 + (s.my0 - mm.y) / (mm.shift ? 800.0 : 150.0)); showTip (s.pname, s.r.getCentreX(), s.r.getY()); }
    };
    r.up = [this] (Reg& s, const M&) { if (s.pname >= 0) m.endEdit(); if (! s.moved) expandSource (s.srckey); };
    r.hover = [this] (Reg& s, const M&) { if (s.pname >= 0) showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
    hit (x, y, w, h, r);
}

double View::xlfoValueAt (int k, double ph) const
{
    const auto& X = m.str.xlfo[(size_t) k];
    const int ns = (int) X.steps.size();
    const double bal = m.pv (P::x1_bal + k * 6) * 0.01, gl = m.pv (P::x1_glide + k * 6) * 0.01;
    const double h = 0.5 + 0.45 * bal;
    const double wp = ph < h ? ph * 0.5 / h : 0.5 + (ph - h) * 0.5 / (1 - h);
    const double pos = wp * ns;
    const int si = std::min ((int) std::floor (pos), ns - 1);
    const double fr = pos - si;
    const auto& st = X.steps[(size_t) si];
    const auto& nst = X.steps[(size_t) ((si + 1) % ns)];
    double v0 = st.r ? 0 : st.v, v1 = nst.r ? 0 : nst.v;
    if (X.snap) { v0 = std::round (v0 * 12) / 12; v1 = std::round (v1 * 12) / 12; }
    const double g = juce::jlimit (0.0, 1.0, gl + st.g);
    double t = 0;
    if (g > 0.0001) t = juce::jlimit (0.0, 1.0, (fr - (1 - g)) / g);
    double ct = t;
    if (st.c == 1) ct = t * t; else if (st.c == 2) ct = std::sqrt (t); else if (st.c == 3) ct = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * t);
    return v0 + (v1 - v0) * ct;
}

std::vector<juce::Point<float>> View::egPoints (int k, float W, float H) const
{
    auto nrm = [&] (int off) { return (float) m.getNorm (P::e1_thr + k * 7 + off); };
    const float sus = (float) (m.pv (P::e1_sus + k * 7) / 100.0);
    const float wd = nrm (1) * 0.16f, wa = nrm (2) * 0.2f, wdc = nrm (3) * 0.2f, ws = 0.12f, wh = nrm (5) * 0.12f, wr = nrm (6) * 0.2f;
    const float tot = std::max (wd + wa + wdc + ws + wh + wr, 0.0001f);
    const float sc = 1.0f / tot;
    float x = 0;
    std::vector<juce::Point<float>> p { { 0, 0 } };
    x += wd * sc; p.push_back ({ x, 0 });
    x += wa * sc; p.push_back ({ x, 1 });
    x += wdc * sc; p.push_back ({ x, sus });
    x += ws * sc; p.push_back ({ x, sus });
    x += wh * sc; p.push_back ({ x, sus });
    x += wr * sc; p.push_back ({ x, 0 });
    for (auto& q : p) { q.x *= W; q.y *= H; }
    return p;
}

std::vector<double> View::sourceShape (const String& key, int N) const
{
    String t; int k; Model::srcKeyParts (key, t, k);
    std::vector<double> out;
    for (int i = 0; i < N; ++i)
    {
        const double u = (double) i / (N - 1);
        if (t == "xlfo") out.push_back (xlfoValueAt (k, u * 0.999));
        else if (t == "eg")
        {
            auto p = egPoints (k, 1, 1);
            double v = 0;
            for (size_t j = 1; j < p.size(); ++j)
                if (u <= p[j].x || j == p.size() - 1)
                {
                    const double d = p[j].x - p[j - 1].x;
                    const double tt = d > 0 ? juce::jlimit (0.0, 1.0, (u - p[j - 1].x) / d) : 1.0;
                    v = p[j - 1].y + (p[j].y - p[j - 1].y) * tt;
                    break;
                }
            out.push_back (v * 2 - 1);
        }
        else if (t == "ef")
        {
            double v = u < 0.25 ? std::sin (u / 0.25 * juce::MathConstants<double>::halfPi) * 1.6 - 0.6 : 1 - std::pow ((u - 0.25) / 0.75, 0.5) * 1.6;
            out.push_back (juce::jlimit (-0.8, 0.8, v));
        }
        else
        {
            const auto& mi = m.str.midi[(size_t) k];
            if (mi.input == 2) { const double w = u * 2 - 1; out.push_back ((w < 0 ? -1 : 1) * curveFn (mi.curve, std::abs (w)) * 0.8); }
            else out.push_back ((curveFn (mi.curve, u) * 2 - 1) * 0.8);
        }
    }
    return out;
}

void View::drawXY (const String& key, float x, float y, float w, float h)
{
    auto& c = *cv;
    String t; int k; Model::srcKeyParts (key, t, k);
    const auto& o = m.str.xy[(size_t) k];
    const auto col = srcColor ("xy");
    const int px = P::xy1_x + k * 2, py = P::xy1_y + k * 2;
    const bool slider = o.mode == 1;
    if (slider) dragDot (key, "y", x + w / 2, y - 12);
    else
    {
        dragDot (key, "x", x + w / 2 - 9, y - 12); dragDot (key, "y", x + w / 2 + 9, y - 12);
        c.text ("X", x + w / 2 - 9, y + 8, 7, C_DIM, 'c'); c.text ("Y", x + w / 2 + 9, y + 8, 7, C_DIM, 'c');
    }
    c.rgb (0.13f, 0.11f, 0.12f); c.rrect (x, y, w, h, 6); c.doFill();
    c.rgb (1, 1, 1, 0.07f); c.lineWidth (1); c.rrect (x + 0.5f, y + 0.5f, w - 1, h - 1, 6); c.doStroke();
    const float nx = (float) (m.pv (px) + 100) / 200, ny = (float) (m.pv (py) + 100) / 200;
    if (slider)
    {
        const float fy = y + (1 - ny) * h, base = o.range == 1 ? y + h : y + h / 2;
        auto gr = c.linear (0, fy, 0, y + h);
        gr.stop (0, col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.85f);
        gr.stop (1, col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.15f);
        c.source (gr); c.rect (x + 3, std::min (fy, base), w - 6, std::abs (base - fy) + 1); c.doFill();
        c.col (col); c.rect (x + 3, fy - 1, w - 6, 2); c.doFill();
    }
    else
    {
        const float cx = x + nx * w, cy = y + (1 - ny) * h;
        auto rg = c.radial (cx, cy, 22);
        rg.stop (0, col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.75f);
        rg.stop (1, col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 0.0f);
        c.source (rg); c.rrect (x, y, w, h, 6); c.doFill();
        c.rgb (1, 1, 1, 0.15f); c.line (x + 4, cy, x + w - 4, cy); c.line (cx, y + 4, cx, y + h - 4);
        c.rgb (1, 1, 1); c.circle (cx, cy, 2.2f); c.doFill();
    }
    sourceLabel (key, x + w / 2, y + h + 13);
    if (m.learnMode) { learnBalloon (py, x + w / 2, y + h / 2); if (! slider) learnBalloon (px, x + w / 2, y + h / 2 + 14); }
    Reg r; r.kind = "xy"; r.srckey = key; r.pname = py; r.idx = px; r.rot = slider;
    auto setFn = [this] (Reg& s, const M& mm)
    {
        double vx = juce::jlimit (0.0, 1.0, (double) (mm.x - s.r.getX()) / s.r.getWidth()) * 200 - 100;
        double vy = juce::jlimit (0.0, 1.0, 1.0 - (mm.y - s.r.getY()) / s.r.getHeight()) * 200 - 100;
        if (mm.shift && ! s.rot)
        {
            if (s.lock == 0) s.lock = std::abs (vx - m.pv (s.idx)) > std::abs (vy - m.pv (s.pname)) ? 1 : 2;
            if (s.lock == 1) vy = m.pv (s.pname); else vx = m.pv (s.idx);
        }
        if (! s.rot) m.setParam (s.idx, vx);
        m.setParam (s.pname, vy);
        tip = { s.rot ? Model::fmtVal (s.pname, m.pv (s.pname)) : "X " + String (juce::roundToInt (m.pv (s.idx))) + "%  Y " + String (juce::roundToInt (m.pv (s.pname))) + "%", s.r.getCentreX(), s.r.getY(), true };
    };
    r.down = [this, setFn] (Reg& s, const M& mm) { m.beginEdit(); s.lock = 0; setFn (s, mm); };
    r.drag = [setFn] (Reg& s, const M& mm) { setFn (s, mm); };
    r.up = [this] (Reg&, const M&) { m.endEdit(); };
    r.reset = [this] (Reg& s, const M&) { m.beginEdit(); m.setParam (s.idx, 0); m.setParam (s.pname, 0); m.endEdit(); };
    r.dbl = [this] (Reg& s, const M&) { if (s.rot) startEntry (s.pname, s.r.getX() - 20, s.r.getY() + 20); };
    r.hover = [this] (Reg& s, const M&) { if (s.rot) showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
    hit (x, y, w, h, r);
}

void View::panelBg (float x, float y, float w, float h, const String& key)
{
    auto& c = *cv;
    c.rgb (0.16f, 0.13f, 0.14f); c.rrect (x, y, w, h, 6); c.doFill();
    c.rgb (1, 1, 1, 0.07f); c.lineWidth (1); c.rrect (x + 0.5f, y + 0.5f, w - 1, h - 1, 6); c.doStroke();
    { Reg r; r.kind = "srcpanel"; r.srckey = key; r.down = [this] (Reg&, const M&) { selStep.clear(); repaint(); }; hit (x, y, w, h, r); }
    c.text (juce::CharPointer_UTF8 ("\xc2\xab"), x + 8, y + 12, 11, C_DIM, 'l', true);
    { Reg r; r.kind = "button"; r.bid = "collapse" + key; r.srckey = key; r.down = [this] (Reg& s, const M&) { collapseSource (s.srckey); }; hit (x + 2, y + 2, 16, 14, r); }
    c.rgb (0.6f, 0.56f, 0.57f); iconX (x + w - 10, y + 9, 3);
    { Reg r; r.kind = "button"; r.bid = "remove" + key; r.tipText = "Remove"; r.srckey = key; r.down = [this] (Reg& s, const M&) { m.pushUndo(); m.removeSource (s.srckey); }; hit (x + w - 18, y + 2, 16, 14, r); }
}

void View::drawSourceExpanded (const String& key, float x, float y, float w)
{
    String t; int k; Model::srcKeyParts (key, t, k);
    const float h = 84;
    dragDot (key, {}, x + 30, y - 12);
    panelBg (x, y, w, h, key);
    if (t == "xlfo") drawXLFOPanel (k, x, y, w, h);
    else if (t == "eg") drawEGPanel (k, x, y, w, h);
    else if (t == "ef") drawEFPanel (k, x, y, w, h);
    else if (t == "midi") drawMIDIPanel (k, x, y, w, h);
    sourceLabel (key, x + w / 2, y + h + 13);
}

void View::drawXLFOPanel (int k, float x, float y, float, float)
{
    auto& c = *cv;
    auto& X = m.str.xlfo[(size_t) k];
    const auto col = srcColor ("xlfo");
    const int pb = P::x1_rate + k * 6;
    const bool sync = X.sync > 0;
    KOpt o; o.label = sync ? "OFFSET" : "RATE"; o.color = col;
    knob (x + 42, y + 44, 17, sync ? pb + 2 : pb, o);
    const float bn = (float) m.getNorm (pb + 3);
    c.lineWidth (2);
    c.rgb (0.3f, 0.27f, 0.28f); c.arc (x + 42, y + 44, 25, 1.15f * PI_F, 1.85f * PI_F); c.doStroke();
    c.col (col);
    const float ba = 1.5f * PI_F + (bn - 0.5f) * 0.7f * PI_F;
    c.arc (x + 42, y + 44, 25, std::min (ba, 1.5f * PI_F) - 0.03f, std::max (ba, 1.5f * PI_F) + 0.03f); c.doStroke();
    {
        Reg r; r.kind = "pan"; r.pname = pb + 3; r.tgt = params()[(size_t) (pb + 3)].target;
        r.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.n0 = m.getNorm (s.pname); s.mx0 = mm.x; };
        r.drag = [this] (Reg& s, const M& mm) { m.setParamNorm (s.pname, s.n0 + (mm.x - s.mx0) / (mm.shift ? 600.0 : 100.0)); showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.reset = [this] (Reg& s, const M&) { m.setParam (s.pname, 0); };
        r.dbl = [this] (Reg& s, const M&) { startEntry (s.pname, s.r.getX(), s.r.getY()); };
        r.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
        hit (x + 18, y + 14, 48, 9, r);
    }
    c.text ("BAL", x + 42, y + 12, 7, C_DIM, 'c', true);
    if (sync)
    {
        const float angs[3] = { 1.5f * PI_F, 0.95f * PI_F, 0.05f * PI_F };
        const char* tips[3] = { "Straight", "Dotted", "Triplet" };
        for (int d = 0; d < 3; ++d)
        {
            const float dx = x + 42 + std::cos (angs[d]) * 31, dy = y + 44 + std::sin (angs[d]) * 31;
            const bool on = std::abs (m.pv (pb + 2) - SYNC_OFS[d]) < 0.01;
            if (on) c.col (col); else c.rgb (0.6f, 0.57f, 0.58f);
            c.circle (dx, dy, 2.4f); c.doFill();
            Reg r; r.kind = "button"; r.bid = "ofs" + String (d); r.tipText = tips[d]; r.v0 = SYNC_OFS[d]; r.pname = pb + 2;
            r.down = [this] (Reg& s, const M&) { m.pushUndo(); m.setParam (s.pname, s.v0); };
            hit (dx - 4, dy - 4, 8, 8, r);
        }
    }
    const String ml = sync ? String (syncDivs[juce::jlimit (0, 10, (int) std::lround (m.pv (pb + 1)))]) : String ("Hz");
    smallBtn (x + 72, y + 8, 40, ml + String (juce::CharPointer_UTF8 (" \xe2\x96\xbe")), [this, k, x, y] { openRateMenu (k, x + 72, y + 24); });
    smallBtn (x + 116, y + 8, 18, "", [this, k, x, y] { openRetrigMenu (k, x + 116, y + 24); }, X.retrig > 0).tipText = "MIDI Trigger";
    for (int kk = 0; kk < 3; ++kk) { c.rgb (0.9f, 0.9f, 0.9f); c.rect (x + 119 + kk * 4, y + 11, 3, 8); c.doFill(); }
    smallBtn (x + 116, y + 26, 18, "S", [this, k] { m.pushUndo(); auto& Xr = m.str.xlfo[(size_t) k]; Xr.snap = Xr.snap ? 0 : 1; m.structChanged(); }, X.snap > 0).tipText = "Snap";
    KOpt og; og.label = "GLIDE"; og.bipolar = true; og.color = col;
    knob (x + 100, y + 54, 12, pb + 4, og);
    // editor dei passi
    const float ex = x + 146, ey = y + 8, ew = 236, eh = 68;
    c.rgb (0.09f, 0.07f, 0.08f); c.rrect (ex, ey, ew, eh, 4); c.doFill();
    c.rgb (1, 1, 1, 0.08f); c.lineWidth (1); c.line (ex, ey + eh / 2, ex + ew, ey + eh / 2);
    c.text ("1", ex - 6, ey + 7, 7, C_DIM, 'r'); c.text ("-1", ex - 3, ey + eh, 7, C_DIM, 'r');
    const int ns = (int) X.steps.size();
    const float sw = ew / ns;
    if (X.snap)
        for (int i = 0; i <= 24; ++i)
        {
            const float ky = ey + eh - (i / 24.0f) * eh;
            const bool black = (i % 12 == 1 || i % 12 == 3 || i % 12 == 6 || i % 12 == 8 || i % 12 == 10);
            c.rgb (1, 1, 1, black ? 0.03f : 0.06f); c.line (ex, ky, ex + ew, ky);
        }
    c.col (col, 0.9f); c.lineWidth (1.6f);
    for (int i = 0; i <= 120; ++i)
    {
        const double ph = i / 120.0 * 0.9999;
        const float px = ex + (float) ph * ew, py = ey + eh / 2 - (float) xlfoValueAt (k, ph) * (eh / 2 - 4);
        if (i == 0) c.moveTo (px, py); else c.lineTo (px, py);
    }
    c.doStroke();
    for (int i = 0; i < ns; ++i)
    {
        const auto& st = X.steps[(size_t) i];
        const float sx = ex + i * sw, sy = ey + eh / 2 - (float) (st.r ? 0 : st.v) * (eh / 2 - 4);
        const bool sel = stepOwner == k && std::find (selStep.begin(), selStep.end(), i) != selStep.end();
        c.rgb (1, 1, 1, sel ? 0.12f : 0.03f); c.rect (sx + 1, ey + 1, sw - 2, eh - 2); c.doFill();
        if (st.r) c.text ("?", sx + sw / 2, ey + eh / 2 + 4, 10, Colour::fromFloatRGBA (1, 1, 1, 0.5f), 'c', true);
        else
        {
            if (sel) c.rgb (1, 1, 1); else c.col (col);
            c.rrect (sx + sw / 2 - 7, sy - 5, 14, 10, 2); c.doFill();
            c.text (String (i + 1), sx + sw / 2, sy + 3, 7, Colour::fromFloatRGBA (0.05f, 0.05f, 0.05f, 1), 'c', true);
        }
        Reg r; r.kind = "step"; r.idx = i; r.band = k; r.cy = eh;
        r.down = [this] (Reg& s, const M& mm)
        {
            if (stepOwner != s.band) { selStep.clear(); stepOwner = s.band; }
            auto& sl = selStep;
            if (mm.cmd || mm.ctrl) { auto it = std::find (sl.begin(), sl.end(), s.idx); if (it != sl.end()) sl.erase (it); else sl.push_back (s.idx); }
            else if (mm.shift && ! sl.empty()) { const int a = sl.back(); for (int z = std::min (a, s.idx); z <= std::max (a, s.idx); ++z) if (std::find (sl.begin(), sl.end(), z) == sl.end()) sl.push_back (z); }
            else if (std::find (sl.begin(), sl.end(), s.idx) == sl.end()) selStep = { s.idx };
            m.beginEdit();
            s.my0 = mm.y; s.vs.clear();
            for (int q : selStep) s.vs.push_back (m.str.xlfo[(size_t) s.band].steps[(size_t) q].v);
            repaint();
        };
        r.drag = [this] (Reg& s, const M& mm)
        {
            const double dv = (s.my0 - mm.y) / (s.cy / 2 - 4) * (mm.shift ? 0.1 : 1.0);
            auto& steps = m.str.xlfo[(size_t) s.band].steps;
            for (size_t q = 0; q < selStep.size() && q < s.vs.size(); ++q)
                if (selStep[q] < (int) steps.size()) steps[(size_t) selStep[q]].v = juce::jlimit (-1.0, 1.0, s.vs[q] + dv);
            if (s.idx < (int) steps.size()) tip = { "Step " + String (s.idx + 1) + ": " + String (steps[(size_t) s.idx].v, 2), s.r.getCentreX(), s.r.getY(), true };
            m.structChanged();
        };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.reset = [this] (Reg& s, const M&) { m.str.xlfo[(size_t) s.band].steps[(size_t) s.idx].v = 0; m.structChanged(); };
        hit (sx, ey, sw, eh, r);
    }
    const float lph = VIEWD[(size_t) (VIEW::LFOPH + k)];
    c.col (col, 0.6f); c.line (ex + lph * ew, ey, ex + lph * ew, ey + eh);
    const float pho = (float) (m.pv (pb + 5) / 100.0), tx = ex + pho * ew;
    c.rgb (0.85f, 0.85f, 0.85f, 0.5f);
    c.moveTo (tx, ey + eh); c.lineTo (tx - 6, ey + eh + 6); c.lineTo (tx + 6, ey + eh + 6); c.closePath(); c.doFill();
    c.rgb (1, 1, 1, 0.25f); c.line (tx, ey, tx, ey + eh);
    {
        Reg r; r.kind = "phase"; r.pname = pb + 5; r.tgt = params()[(size_t) (pb + 5)].target; r.rad = ew;
        r.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.mx0 = mm.x; s.v0 = m.pv (s.pname); };
        r.drag = [this] (Reg& s, const M& mm) { m.setParam (s.pname, juce::jlimit (0.0, 100.0, s.v0 + (mm.x - s.mx0) / s.rad * 100)); showTip (s.pname, s.r.getX(), s.r.getY()); };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.reset = [this] (Reg& s, const M&) { m.setParam (s.pname, 0); };
        hit (tx - 7, ey + eh - 2, 14, 10, r);
    }
    smallBtn (ex + ew + 4, ey, 14, "+", [this, k]
    {
        m.pushUndo(); auto& Xs = m.str.xlfo[(size_t) k];
        if ((int) Xs.steps.size() < NSTEPS)
        {
            int at = (int) Xs.steps.size();
            if (stepOwner == k && ! selStep.empty()) at = *std::max_element (selStep.begin(), selStep.end()) + 1;
            Xs.steps.insert (Xs.steps.begin() + at, Xs.steps[(size_t) std::max (at - 1, 0)]);
        }
        m.structChanged();
    });
    smallBtn (ex + ew + 4, ey + eh - 14, 14, "-", [this, k]
    {
        m.pushUndo(); auto& Xs = m.str.xlfo[(size_t) k];
        if (stepOwner == k && ! selStep.empty())
        {
            std::sort (selStep.begin(), selStep.end(), std::greater<int>());
            for (int z : selStep) if (Xs.steps.size() > 1 && z < (int) Xs.steps.size()) Xs.steps.erase (Xs.steps.begin() + z);
            selStep.clear();
        }
        else if (Xs.steps.size() > 1) Xs.steps.pop_back();
        m.structChanged();
    });
    const float px0 = ex + ew + 24;
    if (! selStep.empty() && stepOwner == k && selStep[0] < ns)
    {
        const auto& s0 = X.steps[(size_t) selStep[0]];
        c.text (selStep.size() > 1 ? String ((int) selStep.size()) + " Steps" : "Step " + String (selStep[0] + 1), px0 + 55, y + 16, 9, C_TXT, 'c', true);
        smallBtn (px0 + 4, y + 22, 50, "Random", [this, k]
        {
            m.pushUndo(); auto& st = m.str.xlfo[(size_t) k].steps;
            const int nv = st[(size_t) selStep[0]].r ? 0 : 1;
            for (int z : selStep) if (z < (int) st.size()) st[(size_t) z].r = nv;
            m.structChanged();
        }, s0.r > 0);
        const char* curves[] = { "Linear", "Sqr", "Sqrt", "Sine" };
        smallBtn (px0 + 58, y + 22, 50, String (curves[juce::jlimit (0, 3, s0.c)]) + String (juce::CharPointer_UTF8 (" \xe2\x96\xbe")), [this, k, px0, y]
        {
            juce::PopupMenu pm;
            const char* cn[] = { "Linear", "Sqr", "Sqrt", "Sine" };
            const int cur = m.str.xlfo[(size_t) k].steps[(size_t) selStep[0]].c;
            for (int ci = 0; ci < 4; ++ci)
                pm.addItem (cn[ci], true, cur == ci, [this, k, ci] { m.pushUndo(); auto& st = m.str.xlfo[(size_t) k].steps; for (int z : selStep) if (z < (int) st.size()) st[(size_t) z].c = ci; m.structChanged(); });
            showMenu (pm, px0 + 58, y + 38);
        });
        stepKnob (px0 + 28, y + 60, k, 0, -1, 1, "VALUE");
        stepKnob (px0 + 82, y + 60, k, 1, 0, 1, "GLIDE");
    }
    else
    {
        c.text ("Click a step", px0 + 55, y + 46, 8, C_DIM, 'c');
        c.text ("to edit it", px0 + 55, y + 57, 8, C_DIM, 'c');
    }
}

void View::stepKnob (float cx, float cy, int k, int field, double mn, double mx, const String& label)
{
    auto& c = *cv;
    const auto& s0 = m.str.xlfo[(size_t) k].steps[(size_t) selStep[0]];
    const double val = field == 0 ? s0.v : s0.g;
    const float n = (float) ((val - mn) / (mx - mn)), r = 11;
    c.lineWidth (2);
    c.rgb (0.2f, 0.17f, 0.18f); c.arc (cx, cy, r + 2.5f, 0.75f * PI_F, 2.25f * PI_F); c.doStroke();
    c.col (srcColor ("xlfo")); c.arc (cx, cy, r + 2.5f, 0.75f * PI_F, 0.75f * PI_F + 1.5f * PI_F * n + 0.001f); c.doStroke();
    c.rgb (0.4f, 0.37f, 0.38f); c.circle (cx, cy, r); c.doFill();
    const float a = 0.75f * PI_F + 1.5f * PI_F * n;
    c.rgb (1, 1, 1); c.lineWidth (2); c.line (cx + std::cos (a) * 4, cy + std::sin (a) * 4, cx + std::cos (a) * 10, cy + std::sin (a) * 10);
    c.text (label, cx, cy + r + 10, 7, C_TXT, 'c', true);
    Reg rg; rg.kind = "stepknob"; rg.band = k; rg.idx = field; rg.v0 = mn; rg.n0 = mx; rg.srckey = label;
    rg.down = [this] (Reg& s, const M& mm)
    {
        m.beginEdit(); s.my0 = mm.y; s.vs.clear();
        for (int z : selStep) { const auto& st = m.str.xlfo[(size_t) s.band].steps[(size_t) z]; s.vs.push_back (s.idx == 0 ? st.v : st.g); }
    };
    rg.drag = [this] (Reg& s, const M& mm)
    {
        if (s.vs.empty()) return;
        const double nv = juce::jlimit (s.v0, s.n0, s.vs[0] + (s.my0 - mm.y) / (mm.shift ? 1000.0 : 150.0) * (s.n0 - s.v0));
        auto& st = m.str.xlfo[(size_t) s.band].steps;
        for (int z : selStep) if (z < (int) st.size()) { if (s.idx == 0) st[(size_t) z].v = nv; else st[(size_t) z].g = nv; }
        tip = { s.srckey + ": " + String (nv, 2), s.r.getCentreX(), s.r.getY(), true };
        m.structChanged();
    };
    rg.up = [this] (Reg&, const M&) { m.endEdit(); };
    hit (cx - r - 3, cy - r - 3, (r + 3) * 2, (r + 3) * 2, rg);
}

void View::trigButton (const String& type, int k, float x, float y)
{
    auto& c = *cv;
    const int trig = type == "eg" ? m.str.eg[(size_t) k].trig : m.str.ef[(size_t) k].trig;
    smallBtn (x, y, 20, "", [this, type, k, x, y] { openTrigMenu (type, k, x, y + 16); }).tipText = "Trigger Input";
    c.rgb (0.9f, 0.9f, 0.9f); c.lineWidth (1.2f);
    c.line (x + 5, y + 7, x + 10, y + 7); c.rect (x + 10, y + 4, 5, 6); c.doStroke();
    if (trig >= 1 && trig <= 7)
    {
        auto& r = smallBtn (x, y + 18, 20, "", [] {});
        r.tipText = "Audition";
        r.down = [this, trig] (Reg&, const M&) { proc.engine.audition.store (trig == 1 ? 2 : trig + 1); };
        r.up = [this] (Reg&, const M&) { proc.engine.audition.store (0); };
        c.rgb (0.9f, 0.9f, 0.9f); c.lineWidth (1.2f);
        c.arc (x + 10, y + 27, 4, PI_F, 2 * PI_F); c.doStroke();
    }
}

void View::drawEGPanel (int k, float x, float y, float w, float h)
{
    auto& c = *cv;
    auto& E = m.str.eg[(size_t) k];
    const auto col = srcColor ("eg");
    const int pb = P::e1_thr + k * 7;
    trigButton ("eg", k, x + 10, y + 18);
    smallBtn (x + 34, y + 18, 20, E.range ? juce::CharPointer_UTF8 ("\xc2\xb1") : juce::CharPointer_UTF8 ("+"), [this, k] { m.pushUndo(); auto& e = m.str.eg[(size_t) k]; e.range = e.range ? 0 : 1; m.structChanged(); }, E.range > 0).tipText = "Range";
    if (E.trig != 8)
    {
        const float thx = x + 36, thy = y + 58;
        const float tl = VIEWD[(size_t) (VIEW::TRIG + juce::jlimit (0, 7, E.trig))];
        const float tn = clampf ((tl + 60) / 60, 0, 1);
        c.lineWidth (3);
        c.rgb (0.2f, 0.17f, 0.18f); c.arc (thx, thy, 18, 0.75f * PI_F, 2.25f * PI_F); c.doStroke();
        c.rgb (0.45f, 0.8f, 0.2f); c.arc (thx, thy, 18, 0.75f * PI_F, 0.75f * PI_F + 1.5f * PI_F * tn + 0.001f); c.doStroke();
        KOpt o; o.color = col;
        knob (thx, thy, 12, pb, o);
        c.text ("THRESHOLD", thx, y + h - 2, 6.5f, C_TXT, 'c', true);
    }
    else c.text ("MIDI", x + 36, y + 62, 9, col, 'c', true);
    const float gx = x + 72, gy = y + 10, gw = w - 86, gh = h - 22;
    c.rgb (0.09f, 0.07f, 0.08f); c.rrect (gx, gy, gw, gh, 4); c.doFill();
    auto pts = egPoints (k, gw - 16, gh - 12);
    const float X0 = gx + 8, Y0 = gy + gh - 6;
    const double segS[6] = { 0, E.as, E.ds, 0, 0, E.rs };
    c.moveTo (X0 + pts[0].x, Y0 - pts[0].y);
    for (size_t i = 1; i < pts.size(); ++i)
        for (int t = 1; t <= 12; ++t)
        {
            const double tt = t / 12.0, s = segS[i - 1];
            double yy;
            if (i == 2 || i == 3 || i == 6) yy = pts[i - 1].y + (pts[i].y - pts[i - 1].y) * std::pow (tt, std::pow (4.0, -s));
            else yy = pts[i - 1].y + (pts[i].y - pts[i - 1].y) * tt;
            c.lineTo (X0 + pts[i - 1].x + (pts[i].x - pts[i - 1].x) * (float) tt, Y0 - (float) yy);
        }
    c.lineTo (X0 + pts.back().x, Y0); c.lineTo (X0 + pts[0].x, Y0); c.closePath();
    c.col (col, 0.18f); c.fillKeep();
    c.col (col, 0.95f); c.lineWidth (1.6f); c.doStroke();
    c.rgb (1, 1, 1, 0.12f); c.line (X0 + pts[4].x, gy + 4, X0 + pts[4].x, gy + gh - 4);
    const std::pair<int, int> handles[5] = { { 1, 1 }, { 2, 2 }, { 3, 3 }, { 5, 5 }, { 6, 6 } };   // punto, offset del parametro
    for (auto& hd : handles)
    {
        const auto hp = pts[(size_t) hd.first];
        c.rgb (0.85f, 0.85f, 0.9f); c.circle (X0 + hp.x, Y0 - hp.y, 3.6f); c.doFill();
        Reg r; r.kind = "egpt"; r.pname = pb + hd.second; r.tgt = params()[(size_t) (pb + hd.second)].target; r.rot = hd.first == 3; r.idx = pb + 4; r.rad = gh;
        r.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.mx0 = mm.x; s.my0 = mm.y; s.n0 = m.getNorm (s.pname); s.v0 = m.pv (s.idx); };
        r.drag = [this] (Reg& s, const M& mm)
        {
            m.setParamNorm (s.pname, s.n0 + (mm.x - s.mx0) / (mm.shift ? 600.0 : 90.0));
            if (s.rot) m.setParam (s.idx, juce::jlimit (0.0, 100.0, s.v0 + (s.my0 - mm.y) / (s.rad - 12) * 100 * (mm.shift ? 0.1 : 1.0)));
            showTip (s.pname, s.r.getCentreX(), s.r.getY());
        };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.dbl = [this] (Reg& s, const M&) { startEntry (s.pname, s.r.getX(), s.r.getY()); };
        r.reset = [this] (Reg& s, const M&) { m.setParam (s.pname, params()[(size_t) s.pname].def); };
        r.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
        hit (X0 + hp.x - 6, Y0 - hp.y - 6, 12, 12, r);
    }
    struct SL { int a, b; double* v; int sign; };
    SL slopes[3] = { { 1, 2, &E.as, 1 }, { 2, 3, &E.ds, -1 }, { 5, 6, &E.rs, -1 } };
    for (int i = 0; i < 3; ++i)
    {
        const auto a = pts[(size_t) slopes[i].a], b = pts[(size_t) slopes[i].b];
        const double sv = *slopes[i].v;
        const float mxp = (a.x + b.x) / 2, myp = a.y + (b.y - a.y) * (float) std::pow (0.5, std::pow (4.0, -sv));
        c.col (col); c.lineWidth (1.2f); c.circle (X0 + mxp, Y0 - myp, 2.6f); c.doStroke();
        Reg r; r.kind = "egslope"; r.band = k; r.idx = i; r.lock = slopes[i].sign;
        r.down = [this] (Reg& s, const M& mm)
        {
            m.beginEdit(); s.my0 = mm.y;
            auto& e = m.str.eg[(size_t) s.band];
            s.v0 = s.idx == 0 ? e.as : s.idx == 1 ? e.ds : e.rs;
        };
        r.drag = [this] (Reg& s, const M& mm)
        {
            auto& e = m.str.eg[(size_t) s.band];
            const double v = juce::jlimit (-1.0, 1.0, s.v0 + (s.my0 - mm.y) / 60.0 * s.lock);
            (s.idx == 0 ? e.as : s.idx == 1 ? e.ds : e.rs) = v;
            tip = { "Slope " + String (v, 2), s.r.getCentreX(), s.r.getY(), true };
            m.structChanged();
        };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.reset = [this] (Reg& s, const M&) { auto& e = m.str.eg[(size_t) s.band]; (s.idx == 0 ? e.as : s.idx == 1 ? e.ds : e.rs) = 0; m.structChanged(); };
        hit (X0 + mxp - 5, Y0 - myp - 5, 10, 10, r);
    }
    if (VIEWD[(size_t) (VIEW::EGSTAGE + k)] > 0) { c.col (col); c.circle (gx + gw - 8, gy + 8, 2.5f); c.doFill(); }
    c.text ("D", X0 + pts[1].x, Y0 + 9, 6, C_DIM, 'c'); c.text ("A", X0 + pts[2].x, gy + 9, 6, C_DIM, 'c');
    c.text ("S", X0 + pts[3].x + 6, Y0 - pts[3].y - 5, 6, C_DIM, 'c'); c.text ("H", X0 + pts[5].x, Y0 - pts[5].y - 6, 6, C_DIM, 'c'); c.text ("R", X0 + pts[6].x, Y0 + 9, 6, C_DIM, 'c');
    const float tdx = gx + gw - 30;
    float dx = tdx;
    for (int off = 0; off < 7; ++off) dx += modDots (dx, gy + 8, params()[(size_t) (pb + off)].target);
}

void View::drawEFPanel (int k, float x, float y, float w, float h)
{
    auto& c = *cv;
    auto& F = m.str.ef[(size_t) k];
    const auto col = srcColor ("ef");
    const int pb = P::f1_att + k * 2;
    trigButton ("ef", k, x + 10, y + 18);
    smallBtn (x + 34, y + 18, 20, F.mode ? "T" : "E", [this, k] { m.pushUndo(); auto& f = m.str.ef[(size_t) k]; f.mode = f.mode ? 0 : 1; m.structChanged(); }, F.mode > 0).tipText = "Mode";
    c.text (F.mode ? "Transient" : "Envelope", x + 34, y + 70, 7.5f, C_DIM, 'c');
    const float gx = x + 64, gy = y + 10, gw = w - 78, gh = h - 22;
    c.rgb (0.09f, 0.07f, 0.08f); c.rrect (gx, gy, gw, gh, 4); c.doFill();
    const float an = (float) m.getNorm (pb), rn = (float) m.getNorm (pb + 1);
    const float ax = gx + 10 + an * (gw * 0.35f), rx = ax + 14 + rn * (gw * 0.55f);
    const float my = gy + gh / 2, amp = gh / 2 - 6;
    c.moveTo (gx + 10, my);
    c.curveTo (gx + 10, my - amp * 0.6f, ax - 8, my - amp, ax, my - amp);
    c.lineTo (ax + 10, my - amp);
    c.curveTo (rx - (rx - ax) * 0.4f, my - amp * 0.7f, rx - 10, my - 2, rx, my);
    c.curveTo (rx - 10, my + 2, rx - (rx - ax) * 0.4f, my + amp * 0.7f, ax + 10, my + amp);
    c.lineTo (ax, my + amp);
    c.curveTo (ax - 8, my + amp, gx + 10, my + amp * 0.6f, gx + 10, my);
    c.col (col, 0.2f); c.fillKeep(); c.col (col); c.lineWidth (1.5f); c.doStroke();
    const float lvl = VIEWD[(size_t) (VIEW::SRC + SRC::ef1 + k)];
    c.col (col, 0.5f); c.rect (gx + gw - 8, gy + gh - 4 - lvl * (gh - 8), 4, lvl * (gh - 8)); c.doFill();
    const float hx[2] = { ax, rx }, hy[2] = { my - amp, my };
    for (int i = 0; i < 2; ++i)
    {
        c.rgb (0.85f, 0.85f, 0.9f); c.circle (hx[i], hy[i], 3.6f); c.doFill();
        Reg r; r.kind = "efpt"; r.pname = pb + i; r.tgt = params()[(size_t) (pb + i)].target;
        r.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.mx0 = mm.x; s.n0 = m.getNorm (s.pname); };
        r.drag = [this] (Reg& s, const M& mm) { m.setParamNorm (s.pname, s.n0 + (mm.x - s.mx0) / (mm.shift ? 600.0 : 90.0)); showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.dbl = [this] (Reg& s, const M&) { startEntry (s.pname, s.r.getX(), s.r.getY()); };
        r.reset = [this] (Reg& s, const M&) { m.setParam (s.pname, params()[(size_t) s.pname].def); };
        r.hover = [this] (Reg& s, const M&) { showTip (s.pname, s.r.getCentreX(), s.r.getY()); };
        hit (hx[i] - 6, hy[i] - 6, 12, 12, r);
    }
}

void View::drawMIDIPanel (int k, float x, float y, float w, float)
{
    auto& c = *cv;
    auto& Mi = m.str.midi[(size_t) k];
    auto row = [&] (float yy, const String& label) -> Reg&
    {
        c.rgb (0.24f, 0.21f, 0.22f); c.rrect (x + 18, yy, w - 30, 16, 3); c.doFill();
        c.text (label, x + 18 + (w - 30) / 2, yy + 11.5f, 8.5f, C_TXT, 'c', true);
        Reg r; r.kind = "button"; r.bid = "midirow" + String (yy);
        return hit (x + 18, yy, w - 30, 16, r);
    };
    auto& r1 = row (y + 10, String (MIDI_INPUTS[juce::jlimit (0, 5, Mi.input)]) + String (juce::CharPointer_UTF8 (" \xe2\x96\xbe")));
    r1.down = [this, k] (Reg& s, const M&)
    {
        juce::PopupMenu pm;
        for (int i = 0; i < 6; ++i) pm.addItem (MIDI_INPUTS[i], true, m.str.midi[(size_t) k].input == i, [this, k, i] { m.pushUndo(); m.str.midi[(size_t) k].input = i; m.structChanged(); });
        showMenu (pm, s.r.getX(), s.r.getBottom() + 2);
    };
    if (Mi.input == 5)
    {
        auto& r2 = row (y + 32, "CC " + String (Mi.cc));
        r2.band = k;
        r2.down = [this] (Reg& s, const M& mm) { s.my0 = mm.y; s.v0 = m.str.midi[(size_t) s.band].cc; m.beginEdit(); };
        r2.drag = [this] (Reg& s, const M& mm) { m.str.midi[(size_t) s.band].cc = juce::jlimit (0, 127, (int) std::lround (s.v0 + (s.my0 - mm.y) / 4)); m.structChanged(); };
        r2.up = [this] (Reg&, const M&) { m.endEdit(); };
        r2.wheel = [this] (Reg& s, const M&, float d) { auto& cc = m.str.midi[(size_t) s.band].cc; cc = juce::jlimit (0, 127, cc + (d > 0 ? 1 : -1)); m.structChanged(); };
        r2.dbl = [this] (Reg& s, const M&)
        {
            const int kk = s.band;
            startTextEntry (s.r.getX(), s.r.getY(), String (m.str.midi[(size_t) kk].cc), [this, kk] (String t) { m.pushUndo(); m.str.midi[(size_t) kk].cc = juce::jlimit (0, 127, t.getIntValue()); m.structChanged(); }, "0..127");
        };
    }
    else c.text (juce::CharPointer_UTF8 ("\xe2\x80\x94"), x + w / 2 + 6, y + 44, 9, C_DIM, 'c');
    auto& r3 = row (y + 54, String (MIDI_CURVES[juce::jlimit (0, 5, Mi.curve)]) + String (juce::CharPointer_UTF8 (" \xe2\x96\xbe")));
    r3.down = [this, k] (Reg& s, const M&)
    {
        juce::PopupMenu pm;
        for (int i = 0; i < 6; ++i) pm.addItem (MIDI_CURVES[i], true, m.str.midi[(size_t) k].curve == i, [this, k, i] { m.pushUndo(); m.str.midi[(size_t) k].curve = i; m.structChanged(); });
        showMenu (pm, s.r.getX(), s.r.getBottom() + 2);
    };
}

// ---------------------------------------------------------------- flusso delle sorgenti
void View::drawFlow()
{
    auto& c = *cv;
    std::map<int, const Reg*> regs;
    for (auto& r : REG) if (r.tgt >= 0 && (r.kind == "knob" || r.kind == "slider" || r.kind == "xover")) regs[r.tgt] = &r;
    c.roundCap (true);
    for (auto& s : m.str.slots)
    {
        if (! s.on || s.tgt >= SLOT0) continue;
        auto sp = srcPos.find (s.src);
        if (sp == srcPos.end()) continue;
        const float v = clampf (std::abs (VIEWD[(size_t) (VIEW::SRC + s.src)] * (float) s.amt), 0, 1);
        if (v < 0.01f) continue;
        float tx;
        auto it = regs.find (s.tgt);
        if (it != regs.end()) tx = it->second->r.getCentreX();
        else
        {
            const int b = Model::bandOfTarget (s.tgt);
            if (b >= 0) { double lo, hi; m.bandEdges (b, lo, hi); tx = fx (std::sqrt (lo * hi)); }
            else tx = sp->second.first;
        }
        tx = clampf (tx, 10, BW - 10);
        const auto col = srcColor (Model::srcTypeOfIndex (s.src));
        const float y1 = sp->second.second - 8, y2 = FLOWY + 2;
        const float widths[3] = { 16, 9, 4 }, alph[3] = { 0.05f, 0.08f, 0.12f };
        for (int k = 0; k < 3; ++k)
        {
            c.col (col, alph[k] * (0.3f + v)); c.lineWidth (widths[k]);
            c.moveTo (sp->second.first, y1);
            c.curveTo (sp->second.first, y1 - 14, tx, y2 + 10, tx, y2);
            c.doStroke();
        }
    }
    c.roundCap (false);
}

// =================================================================== barra inferiore
void View::drawBottom()
{
    auto& c = *cv;
    const float y = BOTY;
    c.rgb (0.08f, 0.06f, 0.07f); c.rect (0, y, BW, BH - y); c.doFill();
    button (14, y + 6, 80, 16, "MIDI Learn", m.learnMode, [this] { m.learnMode = ! m.learnMode; m.learnArm = -1; proc.learnActive.store (m.learnMode); repaint(); });
    smallBtn (96, y + 7, 14, juce::CharPointer_UTF8 ("\xe2\x96\xbe"), [this, y] { openLearnMenu (96, y - 2); });
    const bool lp = m.pv (P::linphase) > 0.5;
    button (300, y + 6, 100, 16, "Linear Phase", lp, [this] { m.pushUndo(); m.setParam (P::linphase, m.pv (P::linphase) > 0.5 ? 0 : 1); }, lp, true);
    const int hq = (int) std::lround (m.pv (P::hq));
    button (470, y + 6, 100, 16, hq == 2 ? "Superb Quality" : "High Quality", hq > 0, [this, y]
    {
        juce::PopupMenu pm;
        const char* l[] = { "Off", "Good (8x)", "Superb (32x)" };
        const int cur = (int) std::lround (m.pv (P::hq));
        for (int i = 0; i < 3; ++i) pm.addItem (l[i], true, cur == i, [this, i] { m.pushUndo(); m.setParam (P::hq, i); });
        showMenu (pm, 470, y - 2);
    }, hq > 0, true);
    const bool byp = m.pv (P::bypass) > 0.5;
    c.rgb (byp ? 0.85f : 0.3f, byp ? 0.15f : 0.27f, byp ? 0.15f : 0.28f); c.rrect (660, y + 6, 18, 16, 3); c.doFill();
    iconPower (669, y + 14.5f, ! byp);
    { Reg r; r.kind = "button"; r.bid = "bypass"; r.tipText = "Global Bypass"; r.pname = P::bypass; r.down = [this] (Reg&, const M&) { m.setParam (P::bypass, m.pv (P::bypass) > 0.5 ? 0 : 1); }; hit (660, y + 6, 18, 16, r); }
    const String ostr = String (juce::roundToInt (m.pv (P::gmix))) + "%   " + (m.pv (P::gain_in) > 0 ? "+" : "") + String (m.pv (P::gain_in), 1) + " dB   " + (m.pv (P::gain_out) > 0 ? "+" : "") + String (m.pv (P::gain_out), 1) + " dB";
    c.rgb (outp ? 0.3f : 0.18f, outp ? 0.26f : 0.15f, outp ? 0.27f : 0.16f); c.rrect (684, y + 6, 170, 16, 3); c.doFill();
    c.text (ostr, 769, y + 17.5f, 9, C_TXT, 'c');
    if (m.pv (P::unrestricted) > 0.5 || m.pv (P::msmode) > 0.5) { c.rgb (0.95f, 0.6f, 0.2f); c.rect (690, y + 6, 20, 2); c.doFill(); }
    {
        Reg r; r.kind = "outbtn";
        r.hover = [this] (Reg&, const M&) { outp = true; };
        r.down = [this] (Reg& s, const M& mm) { outp = true; m.beginEdit(); s.my0 = mm.y; s.v0 = m.pv (P::gain_in); };
        r.drag = [this] (Reg& s, const M& mm) { m.setParam (P::gain_in, s.v0 + (s.my0 - mm.y) * (mm.shift ? 0.02 : 0.2)); };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        hit (684, y + 6, 170, 16, r);
    }
    if (m.pv (P::bypass) > 0.5) { c.rgb (0.9f, 0.15f, 0.15f); c.rect (660, y + 2, 18, 2); c.doFill(); }
    c.rgb (0.7f, 0.68f, 0.68f); c.lineWidth (1.2f);
    c.line (866, y + 8, 880, y + 22); c.line (880, y + 22, 880, y + 15); c.line (880, y + 22, 873, y + 22);
    c.line (866, y + 8, 866, y + 15); c.line (866, y + 8, 873, y + 8);
    { Reg r; r.kind = "button"; r.bid = "resize"; r.tipText = "Resize"; r.down = [this, y] (Reg&, const M&) { openResizeMenu (760, y - 2); }; hit (860, y + 4, 28, 22, r); }
}

void View::drawOutputPanel()
{
    auto& c = *cv;
    const float w = 220, h = 112, x = BW - w - 34, y = BOTY - h - 2;
    c.rgb (0.13f, 0.1f, 0.11f, 0.97f); c.rrect (x, y, w, h, 6); c.doFill();
    c.rgb (1, 1, 1, 0.08f); c.lineWidth (1); c.rrect (x + 0.5f, y + 0.5f, w - 1, h - 1, 6); c.doStroke();
    { Reg r; r.kind = "outpanel"; r.down = [] (Reg&, const M&) {}; hit (x, y, w, h + 24, r); }
    KOpt o; o.ring = true;
    o.pan = P::pan_in; o.label = "INPUT"; knob (x + 38, y + 46, 18, P::gain_in, o);
    o.pan = P::pan_out; o.label = "OUTPUT"; knob (x + 104, y + 46, 18, P::gain_out, o);
    smallBtn (x + 150, y + 14, 28, "FB", [this] { m.pushUndo(); m.setParam (P::unrestricted, m.pv (P::unrestricted) > 0.5 ? 0 : 1); }, m.pv (P::unrestricted) > 0.5).tipText = "Unrestricted Feedback";
    smallBtn (x + 182, y + 14, 28, "M/S", [this] { m.pushUndo(); m.setParam (P::msmode, m.pv (P::msmode) > 0.5 ? 0 : 1); }, m.pv (P::msmode) > 0.5).tipText = String ("Channel Mode: ") + (m.pv (P::msmode) > 0.5 ? "Mid/Side" : "Stereo");
    const float mx = x + 150, my = y + 52, mw = 60;
    c.rgb (0.07f, 0.05f, 0.06f); c.rrect (mx, my, mw, 8, 3); c.doFill();
    const float mn = (float) (m.pv (P::gmix) / 100.0);
    c.rgb (0.75f, 0.72f, 0.72f); c.rrect (mx, my, std::max (mw * mn, 1.0f), 8, 3); c.doFill();
    c.text ("MIX " + String (juce::roundToInt (m.pv (P::gmix))) + "%", mx + mw / 2, my + 20, 7.5f, C_TXT, 'c', true);
    const int mt = params()[P::gmix].target;
    modDots (mx + mw / 2 + 24, my + 17, mt);
    {
        Reg r; r.kind = "knob"; r.pname = P::gmix; r.tgt = mt; r.rad = mw; r.cx = mx + mw / 2; r.cy = my;
        r.down = [this] (Reg& s, const M& mm) { m.beginEdit(); s.mx0 = mm.x; s.v0 = m.pv (P::gmix); };
        r.drag = [this] (Reg& s, const M& mm) { m.setParam (P::gmix, s.v0 + (mm.x - s.mx0) / s.rad * 100 * (mm.shift ? 0.1 : 1.0)); showTip (P::gmix, s.cx, s.cy - 6); };
        r.up = [this] (Reg&, const M&) { m.endEdit(); };
        r.reset = [this] (Reg&, const M&) { m.setParam (P::gmix, 100); };
        r.dbl = [this] (Reg& s, const M&) { startEntry (P::gmix, s.r.getX(), s.r.getY() - 20); };
        r.hover = [this] (Reg& s, const M&) { showTip (P::gmix, s.cx, s.cy - 6); };
        hit (mx, my - 4, mw, 16, r);
    }
    for (int i = 0; i < 4; ++i)
    {
        const float lv = clampf ((VIEWD[(size_t) (VIEW::METER + i)] + 60) / 66, 0, 1);
        c.rgb (0.07f, 0.05f, 0.06f); c.rect (x + 150 + i * 7, y + 76, 5, 28); c.doFill();
        c.rgb (lv > 0.9f ? 0.95f : 0.3f, lv > 0.9f ? 0.2f : 0.8f, 0.3f); c.rect (x + 150 + i * 7, y + 104 - lv * 28, 5, lv * 28); c.doFill();
    }
    c.text ("in", x + 157, y + 110, 6, C_DIM, 'c'); c.text ("out", x + 171, y + 110, 6, C_DIM, 'c');
}

// =================================================================== pannello degli slot
std::vector<std::pair<int, int>> View::slotRows() const
{
    std::vector<std::pair<int, int>> rows;
    std::function<void (int, int)> add = [&] (int idx, int depth)
    {
        rows.push_back ({ idx, depth });
        for (int q : m.slotsForTarget (SLOT0 + idx)) if (depth < 3) add (q, depth + 1);
    };
    for (int i = 0; i < (int) m.str.slots.size(); ++i)
    {
        const auto& s = m.str.slots[(size_t) i];
        bool ok;
        if (slotp.mode == 0) ok = s.tgt == slotp.t;
        else if (slotp.mode == 1) ok = s.src == slotp.si && s.tgt < SLOT0;
        else ok = Model::bandOfTarget (s.tgt) == slotp.b;
        if (ok) add (i, 0);
    }
    return rows;
}

void View::drawSlotPanel()
{
    auto& c = *cv;
    auto rows = slotRows();
    if (rows.empty() && slotp.mode != 0) { slotp.on = false; return; }
    const float w = 236, rh = 19, h = 24 + rows.size() * rh + 6;
    const float x = clampf (slotp.x + 14, 4, BW - w - 4), y = clampf (slotp.y - 10, HDR + 4, BH - h - 4);
    c.rgb (0.12f, 0.1f, 0.11f, 0.97f); c.rrect (x, y, w, h, 6); c.doFill();
    c.rgb (1, 1, 1, 0.1f); c.lineWidth (1); c.rrect (x + 0.5f, y + 0.5f, w - 1, h - 1, 6); c.doStroke();
    slotp.rect = { x, y, w, h };
    { Reg r; r.kind = "slotpanel"; r.down = [] (Reg&, const M&) {}; hit (x, y, w, h, r); }
    const String title = slotp.mode == 0 ? m.targetName (slotp.t) : slotp.mode == 1 ? m.srcLabelForSlot (slotp.si) : "Band " + String (slotp.b + 1);
    c.text (title, x + 10, y + 15, 9, C_TXT, 'l', true);
    c.rgb (0.6f, 0.56f, 0.57f); triDown (x + w - 14, y + 11, 3);
    {
        Reg r; r.kind = "button"; r.bid = "slotall";
        r.down = [this] (Reg& s, const M&)
        {
            auto rr = slotRows();
            std::vector<int> ids; for (auto& q : rr) ids.push_back (q.first);
            juce::PopupMenu pm;
            pm.addItem ("Delete All", [this, ids] { m.pushUndo(); auto v = ids; std::sort (v.begin(), v.end(), std::greater<int>()); for (int i : v) if (i < (int) m.str.slots.size()) m.deleteSlot (i, true); m.structChanged(); });
            pm.addItem ("Bypass All", [this, ids] { m.pushUndo(); for (int i : ids) m.str.slots[(size_t) i].on = 0; m.structChanged(); });
            pm.addItem ("Enable All", [this, ids] { m.pushUndo(); for (int i : ids) m.str.slots[(size_t) i].on = 1; m.structChanged(); });
            pm.addItem ("Invert All", [this, ids] { m.pushUndo(); for (int i : ids) m.str.slots[(size_t) i].inv = m.str.slots[(size_t) i].inv ? 0 : 1; m.structChanged(); });
            showMenu (pm, s.r.getX() - 100, s.r.getBottom() + 2);
        };
        hit (x + w - 24, y + 4, 20, 14, r);
    }
    for (size_t ri = 0; ri < rows.size(); ++ri)
    {
        const int si = rows[ri].first, depth = rows[ri].second;
        const auto& s = m.str.slots[(size_t) si];
        const float ry = y + 24 + ri * rh, ind = depth * 14.0f;
        const float sx = x + 30 + ind, sw = w - 58 - ind;
        const bool hover = hoverX >= x && hoverX < x + w && hoverY >= ry && hoverY < ry + rh;
        if (hover || ! s.on)
        {
            iconPower (x + 10 + ind, ry + 9, s.on);
            Reg r; r.kind = "button"; r.bid = "slot_on" + String (si); r.tipText = "On/Off"; r.idx = si;
            r.down = [this] (Reg& q, const M&) { m.pushUndo(); auto& sl = m.str.slots[(size_t) q.idx]; sl.on = sl.on ? 0 : 1; m.structChanged(); };
            hit (x + 3 + ind, ry + 2, 14, 14, r);
        }
        c.text (s.inv ? juce::CharPointer_UTF8 ("\xe2\x88\x92") : juce::CharPointer_UTF8 ("+"), x + 22 + ind, ry + 13, 11, C_TXT, 'c', true);
        { Reg r; r.kind = "button"; r.bid = "slot_inv" + String (si); r.tipText = "Invert"; r.idx = si; r.down = [this] (Reg& q, const M&) { m.pushUndo(); auto& sl = m.str.slots[(size_t) q.idx]; sl.inv = sl.inv ? 0 : 1; m.structChanged(); }; hit (x + 16 + ind, ry + 2, 12, 14, r); }
        c.rgb (0.07f, 0.05f, 0.06f); c.rrect (sx, ry + 3, sw, 13, 3); c.doFill();
        const auto col = srcColor (Model::srcTypeOfIndex (s.src));
        c.col (col, s.on ? 0.75f : 0.25f); c.rrect (sx, ry + 3, std::max (sw * (float) std::abs (s.amt), 2.0f), 13, 3); c.doFill();
        const float ev = VIEWD[(size_t) (SLOT0 + si)];
        if (! m.slotsForTarget (SLOT0 + si).empty()) { c.rgb (1, 1, 1, 0.8f); c.rect (sx + sw * std::min (std::abs (ev), 1.0f) - 1, ry + 3, 2, 13); c.doFill(); }
        String lab = (slotp.mode == 0 || depth > 0) ? m.srcLabelForSlot (s.src) : m.targetName (s.tgt);
        if (slotp.mode == 2 && depth == 0)
        {
            int off = 0; Model::bandOfTarget (s.tgt, &off);
            lab = m.srcLabelForSlot (s.src) + " > " + params()[(size_t) Model::bp (0, off)].ui;
        }
        c.font (8, true);
        while (lab.length() > 4 && c.textWidth (lab) > sw - 34) lab = lab.dropLastCharacters (2);
        c.text (lab, sx + 5, ry + 12.5f, 8, Colour::fromFloatRGBA (1, 1, 1, 0.92f), 'l', true);
        c.text (String (juce::roundToInt (std::abs (s.amt) * 100)) + "%", sx + sw - 4, ry + 12.5f, 8, Colour::fromFloatRGBA (1, 1, 1, 0.75f), 'r');
        {
            Reg r; r.kind = "slotlevel"; r.idx = si; r.rad = sw; r.tgt = SLOT0 + si;
            r.down = [this] (Reg& q, const M& mm)
            {
                q.all = mm.alt; m.beginEdit(); q.mx0 = mm.x; q.v0 = m.str.slots[(size_t) q.idx].amt;
                q.vs.clear(); for (auto& sl : m.str.slots) q.vs.push_back (sl.amt);
            };
            r.drag = [this] (Reg& q, const M& mm)
            {
                const double d = (mm.x - q.mx0) / q.rad * (mm.shift ? 0.1 : 1.0);
                if (q.all || mm.alt)
                {
                    const int src = m.str.slots[(size_t) q.idx].src;
                    for (size_t z = 0; z < m.str.slots.size() && z < q.vs.size(); ++z) if (m.str.slots[z].src == src) m.str.slots[z].amt = juce::jlimit (0.0, 1.0, q.vs[z] + d);
                }
                else m.str.slots[(size_t) q.idx].amt = juce::jlimit (0.0, 1.0, q.v0 + d);
                m.structChanged();
            };
            r.up = [this] (Reg& q, const M&) { m.endEdit(); q.all = false; };
            r.reset = [this] (Reg& q, const M&) { m.str.slots[(size_t) q.idx].amt = 0.5; m.structChanged(); };
            r.dbl = [this] (Reg& q, const M&) { startSlotEntry (q.idx, q.r.getX(), q.r.getY()); };
            r.rdown = [this] (Reg& q, const M&) { openSlotChangeMenu (q.idx, q.r.getX(), q.r.getBottom()); };
            hit (sx, ry + 3, sw, 13, r);
        }
        c.rgb (0.55f, 0.5f, 0.52f); triDown (x + w - 24, ry + 9, 2.6f);
        { Reg r; r.kind = "button"; r.bid = "slot_ch" + String (si); r.tipText = "Change Source/Target"; r.idx = si; r.down = [this] (Reg& q, const M&) { openSlotChangeMenu (q.idx, q.r.getX() - 120, q.r.getBottom()); }; hit (x + w - 30, ry + 2, 12, 14, r); }
        c.rgb (0.7f, 0.66f, 0.67f); iconX (x + w - 11, ry + 9, 3);
        { Reg r; r.kind = "button"; r.bid = "slot_del" + String (si); r.tipText = "Delete Slot"; r.idx = si; r.down = [this] (Reg& q, const M&) { m.pushUndo(); m.deleteSlot (q.idx); }; hit (x + w - 18, ry + 2, 14, 14, r); }
    }
    if (rows.empty()) c.text ("No modulation slots", x + 10, y + 36, 8, C_DIM);
}

// =================================================================== sovrimpressioni
void View::drawTip()
{
    if (! tip.on || drag.on) return;
    auto& c = *cv;
    c.font (9, true);
    const float w = c.textWidth (tip.s) + 14;
    const float x = clampf (tip.x - w / 2, 2, BW - w - 2), y = clampf (tip.y - 24, 2, BH - 20);
    c.rgb (0.95f, 0.94f, 0.94f, 0.95f); c.rrect (x, y, w, 17, 4); c.doFill();
    c.text (tip.s, x + 7, y + 12, 9, Colour::fromFloatRGBA (0.1f, 0.1f, 0.1f, 1), 'l', true);
}

void View::drawLearnBar()
{
    auto& c = *cv;
    c.rgb (0, 0, 0, 0.35f); c.rect (0, HDR, BW, BH - HDR); c.doFill();
    c.rgb (0.75f, 0.15f, 0.18f); c.rect (0, 0, BW, HDR); c.doFill();
    c.text ("MIDI LEARN", 14, 21, 11, juce::Colours::white, 'l', true);
    const String s = m.learnArm >= 0 ? "Move a slider/knob on your MIDI controller to associate it to: " + String (params()[(size_t) m.learnArm].label)
                                    : "Click a control in the interface, then move a slider/knob on your MIDI controller";
    c.text (s, 450, 21, 9.5f, juce::Colours::white, 'c');
    button (BW - 70, 8, 56, 18, "Close", false, [this] { m.learnMode = false; m.learnArm = -1; proc.learnActive.store (false); repaint(); });
}

void View::drawDragOverlay()
{
    auto& c = *cv;
    String t; int k; Model::srcKeyParts (drag.key, t, k);
    const auto col = srcColor (t);
    c.rgb (0, 0, 0, 0.42f); c.rect (0, 0, BW, BH); c.doFill();
    for (auto& r : REG)
    {
        if (r.tgt < 0 || r.kind == "moddot") continue;
        const bool over = r.r.contains (drag.x, drag.y);
        c.col (col, over ? 0.9f : 0.45f); c.lineWidth (over ? 2.5f : 1.5f);
        if (r.kind == "knob") { c.circle (r.r.getCentreX(), r.r.getCentreY(), std::max (r.r.getWidth(), r.r.getHeight()) / 2 + 2); c.doStroke(); }
        else { c.rrect (r.r.getX() - 1, r.r.getY() - 1, r.r.getWidth() + 2, r.r.getHeight() + 2, 3); c.doStroke(); }
    }
    c.col (col); c.lineWidth (2); c.line (drag.x0, drag.y0, drag.x, drag.y);
    c.circle (drag.x, drag.y, 4); c.doFill();
}

void View::drawHelp()
{
    auto& c = *cv;
    c.rgb (0, 0, 0, 0.78f); c.rect (0, 0, BW, BH); c.doFill();
    const String mod = juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX ? "Cmd" : "Ctrl";
    const char* lines[] = {
        "SATURA - quick help",
        "",
        "Display: click '+' at the top to split a band; drag the dotted splits to move crossovers;",
        "  drag level buttons vertically (Alt = Drive), horizontally to move both crossovers of the band.",
        "  Drag on the background to select several bands. S/M: solo and mute (hold = temporary).",
        "Knobs: drag vertically or grab the arrow to rotate; Shift = fine tuning; mouse wheel;",
        "  %MOD%+click = default value; double-click = type a value (1k, A4, C#3+13, 2x, 50%).",
        "Modulation: '+' adds a source; drag its coloured dot onto any control to modulate it.",
        "  Click the coloured indicators next to controls to open the modulation slot panel.",
        "MIDI: route a MIDI track to SATURA (in Logic: AU MIDI-controlled Effects).",
        "Side chain: route a track to SATURA's side chain input and pick External Side Chain",
        "  as trigger input of an EG or EF.",
        "",
        "Click to close" };
    int i = 0;
    for (auto* l : lines)
    {
        c.text (String (l).replace ("%MOD%", mod), 60, 120.0f + i * 20, i == 0 ? 14.0f : 10.0f, i == 0 ? juce::Colours::white : C_TXT, 'l', i == 0);
        ++i;
    }
}

// =================================================================== paint
void View::paint (juce::Graphics& g)
{
    Canvas c (g);
    cv = &c;
    REG.clear();
    tip.on = false;
    c.rgb (0.05f, 0.04f, 0.04f); c.rect (0, 0, BW, BH); c.doFill();
    drawDisplay();
    drawBandPanel();
    drawModSection();
    drawHeader();
    drawBottom();
    if (outp) drawOutputPanel();
    if (slotp.on) drawSlotPanel();
    // regione sotto il mouse (ricalcolata sul nuovo elenco)
    hoverReg = nullptr;
    if (hoverX >= 0)
        for (auto it = REG.rbegin(); it != REG.rend(); ++it)
            if (it->r.contains (hoverX, hoverY) && it->kind != "dispbg" && it->kind != "modbg" && it->kind != "panelbg" && it->kind != "srcpanel" && it->kind != "slotpanel")
            { hoverReg = &*it; break; }
    if (! drag.on && ! active)
    {
        if (hoverReg && hoverReg->hover) { auto copy = *hoverReg; copy.hover (copy, {}); }
        if (hoverReg && hoverReg->tipText.isNotEmpty()) tip = { hoverReg->tipText, hoverReg->r.getCentreX(), hoverReg->r.getY(), true };
    }
    if (active && active->pname >= 0 && active->hover && ! drag.on && active->kind != "outbtn")
    {
        auto copy = *active;
        for (auto& r : REG) if (r.pname == active->pname && r.kind == active->kind) { copy.r = r.r; copy.cx = r.cx; copy.cy = r.cy; break; }
        copy.hover (copy, {});
    }
    if (drag.on && drag.moved) drawDragOverlay();
    if (m.learnMode) drawLearnBar();
    drawTip();
    if (entryOn && entry.hint.isNotEmpty())
        c.text (entry.hint + "  (Enter = OK, Esc = cancel)", juce::jlimit (2.0f, (float) BW - 122, entry.x), juce::jlimit (2.0f, (float) BH - 22, entry.y) + 34, 8, Colour::fromFloatRGBA (1, 1, 1, 0.75f));
    if (help) drawHelp();
    cv = nullptr;
}
