// SATURA - interfaccia principale (porting di ui/gfx.js e ui/events.js della versione Max for Live)
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Canvas.h"
#include "Model.h"

class SaturaProcessor;

class View : public juce::Component, private juce::Timer
{
public:
    static constexpr int BW = 900, BH = 600;
    explicit View (SaturaProcessor& p);
    ~View() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

    std::function<void()> onToggleFullscreen, onSizeChanged;
    bool fullscreen = false;
    // per i test: aggiorna i dati dal motore e ridisegna
    void tick();

    struct M { float x = 0, y = 0; bool cmd = false, shift = false, alt = false, ctrl = false, right = false; };
    struct Reg;
    using Fn = std::function<void (Reg&, const M&)>;
    struct Reg
    {
        juce::Rectangle<float> r;
        juce::String kind, srckey, bid, tipText;
        int pname = -1, tgt = -1, band = -1, idx = -1, si = -1, off = -1;
        bool isBand = false;
        float cx = 0, cy = 0, rad = 0;
        double n0 = 0, my0 = 0, mx0 = 0, v0 = 0;
        bool rot = false, moved = false, all = false;
        std::vector<double> vs;
        int lock = 0;
        Fn down, drag, up, dbl, reset, hover, rdown;
        std::function<void (Reg&, const M&, float)> wheel;
    };
    // per i test
    std::vector<Reg>& regions() { return REG; }
    void simDown (float x, float y) { simDown (x, y, M()); }
    void simDown (float x, float y, M m);
    void simDrag (float x, float y) { simDrag (x, y, M()); }
    void simDrag (float x, float y, M m);
    void simUp (float x, float y);
    void simDouble (float x, float y);
    void simMove (float x, float y);
    juce::TextEditor& textEntry() { return te; }
    bool menuOpen() const { return lastMenuItems > 0; }
    std::function<void (juce::PopupMenu&, std::function<void (int)>)> menuHook;   // test: intercetta i menu

private:
    SaturaProcessor& proc;
    Model& m;
    Canvas* cv = nullptr;
    std::vector<Reg> REG;
    std::unique_ptr<Reg> active;
    std::array<float, 512> VIEWD {};
    std::map<int, std::pair<double, double>> modTrack;
    std::vector<float> specDb;
    struct Tip { juce::String s; float x = 0, y = 0; bool on = false; } tip;
    juce::String flashText;
    double flashUntil = 0;
    struct SlotPanel { int mode = 0; int t = -1, si = -1, b = -1; float x = 0, y = 0; juce::Rectangle<float> rect; bool on = false; } slotp;
    struct Drag { bool on = false; int si = 0; juce::String key; float x0 = 0, y0 = 0, x = 0, y = 0; bool moved = false; juce::String hr; double ht = 0; } drag;
    bool outp = false, help = false;
    juce::Rectangle<float> selRect;
    bool selRectOn = false;
    float hoverX = -1, hoverY = -1;
    juce::String hoverKey;    // identificatore della regione sotto il mouse
    const Reg* hoverReg = nullptr;
    std::vector<int> selStep;
    int stepOwner = -1;
    std::map<int, std::pair<float, float>> srcPos;
    bool engineAlive = false;
    float lastScope = -1;
    int sameCount = 0;
    int lastMenuItems = 0;

    // testo
    juce::TextEditor te;
    struct Entry { int kind = 0; int pname = -1; bool band = false; int off = -1; juce::String key; int slot = -1; std::function<void (juce::String)> fn; juce::String hint; float x = 0, y = 0; } entry;
    bool entryOn = false;
    void startEntry (int pname, float x, float y, bool band = false, int off = -1);
    void startNameEntry (const juce::String& key, float x, float y);
    void startSlotEntry (int i, float x, float y);
    void startTextEntry (float x, float y, const juce::String& text, std::function<void (juce::String)> fn, const juce::String& hint = {});
    void openEntry (const juce::String& text);
    void commitEntry();
    void cancelEntry();

    void timerCallback() override;
    void computeSpectrum();
    void flash (const juce::String& s) { flashText = s; flashUntil = juce::Time::getMillisecondCounterHiRes() + 2500; repaint(); }
    M mk (const juce::MouseEvent& e) const;
    Reg* findReg (float x, float y, std::function<bool (const Reg&)> pred = nullptr);
    Reg& hit (float x, float y, float w, float h, Reg r);

    // ---- mappature
    float fx (double f) const;
    double xf (float x) const;
    float levY (double db) const;
    float specY (double db) const;
    double effNorm (int p) const;
    double effVal (int p) const;
    std::pair<double, double> modRange (int t, double base);
    juce::Colour modColor (int t) const;
    juce::Colour srcColor (const juce::String& type) const;
    int paramOf (bool band, int idOrOff) const;

    // ---- disegno
    struct KOpt { bool band = false, ring = false, bipolar = false; juce::String label; int pan = -1; juce::Colour color { juce::Colour::fromFloatRGBA (0.92f, 0.9f, 0.9f, 1) }; bool hasColor = false; };
    float modDots (float x, float y, int t);
    Reg& knob (float cx, float cy, float r, int p, KOpt o);
    void showTip (int pn, float x, float y);
    void learnBalloon (int pn, float x, float y);
    Reg& vslider (float x, float y, float h, int p, bool band);
    Reg& button (float x, float y, float w, float h, const juce::String& label, bool on, std::function<void()> fn, bool led = false, bool hasLed = false, float size = 9);
    Reg& smallBtn (float x, float y, float w, const juce::String& label, std::function<void()> fn, bool on = false);
    void iconPower (float x, float y, bool on);
    void iconSave (float x, float y);
    void iconX (float x, float y, float s);
    void triDown (float x, float y, float s);
    void drawHeader();
    void drawDisplay();
    void drawLevelHandle (int b, float cx, float ly, bool sel);
    void drawCrossover (int i);
    void drawSoloMute (int b, float x, float y);
    void selectInRect();
    float bandPanelX() const;
    void drawBandPanel();
    juce::StringArray visibleSources() const;
    bool isExpanded (const juce::String& key) const;
    void expandSource (const juce::String& key);
    void collapseSource (const juce::String& key);
    void drawModSection();
    void dragDot (const juce::String& key, const juce::String& axis, float x, float y);
    void sourceLabel (const juce::String& key, float cx, float y);
    void drawSourceButton (const juce::String& key, float x, float y, float w);
    std::vector<double> sourceShape (const juce::String& key, int N) const;
    double xlfoValueAt (int k, double ph) const;
    std::vector<juce::Point<float>> egPoints (int k, float W, float H) const;
    void drawXY (const juce::String& key, float x, float y, float w, float h);
    void panelBg (float x, float y, float w, float h, const juce::String& key);
    void drawSourceExpanded (const juce::String& key, float x, float y, float w);
    void drawXLFOPanel (int k, float x, float y, float w, float h);
    void stepKnob (float cx, float cy, int k, int field, double mn, double mx, const juce::String& label);
    void trigButton (const juce::String& type, int k, float x, float y);
    void drawEGPanel (int k, float x, float y, float w, float h);
    void drawEFPanel (int k, float x, float y, float w, float h);
    void drawMIDIPanel (int k, float x, float y, float w, float h);
    void drawFlow();
    void drawBottom();
    void drawOutputPanel();
    void drawSlotPanel();
    std::vector<std::pair<int, int>> slotRows() const;
    void drawTip();
    void drawLearnBar();
    void drawDragOverlay();
    void drawHelp();

    // ---- trascinamento delle sorgenti
    void dragHover (const M& mm);
    void dropSource (const M& mm);

    // ---- pannelli e menu
    void openSlotPanelTarget (int t, float x, float y) { slotp = {}; slotp.mode = 0; slotp.t = t; slotp.x = x; slotp.y = y; slotp.on = true; repaint(); }
    void openSlotPanelSource (int si, float x, float y) { slotp = {}; slotp.mode = 1; slotp.si = si; slotp.x = x; slotp.y = y; slotp.on = true; repaint(); }
    void openSlotPanelBand (int b, float x, float y) { slotp = {}; slotp.mode = 2; slotp.b = b; slotp.x = x; slotp.y = y; slotp.on = true; repaint(); }
    void showMenu (juce::PopupMenu& menu, float x, float y);
    void openPresetMenu (float x, float y);
    void openPresetShortMenu (float x, float y);
    void openHelpMenu (float x, float y);
    void openStyleMenu (float x, float y);
    void openSectionMenu (const juce::String& type, int k, float x, float y);
    void openAddMenu (float x, float y);
    void openSourceMenu (const juce::String& key, float x, float y);
    void openRateMenu (int k, float x, float y);
    void openRetrigMenu (int k, float x, float y);
    void openTrigMenu (const juce::String& type, int k, float x, float y);
    void openLearnMenu (float x, float y);
    void openResizeMenu (float x, float y);
    void openSlotChangeMenu (int i, float x, float y);
    void savePresetAsDialog();
    void openOtherPresetDialog();
    void changePresetFolderDialog();
    std::unique_ptr<juce::FileChooser> chooser;
    juce::PopupMenu slotTargetMenu (std::function<void (int)> fn);

    void soloMute (int b, int what, const M& mm);
    void soloMuteUp();
    struct SM { std::vector<std::pair<int, double>> snap; double t = 0; bool on = false; } smTmp;

    juce::CriticalSection dummy;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (View)
};
