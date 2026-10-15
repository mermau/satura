// SATURA - eventi, menu, campi di testo e aggiornamento (porting di ui/events.js)
#include "View.h"
#include "PluginProcessor.h"

using namespace spec;
using juce::String;

#ifdef JucePlugin_VersionString
 #define SATURA_VERSION JucePlugin_VersionString
#else
 #define SATURA_VERSION "1.0.0"
#endif

namespace
{
constexpr float HDR = 34;
const char* const SIZE_NAMES[] = { "Small", "Medium", "Large", "Extra Large" };
const double SCALES[] = { 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0 };
}

View::View (SaturaProcessor& p) : proc (p), m (p.model)
{
    setSize (BW, BH);
    setWantsKeyboardFocus (true);
    addChildComponent (te);
    te.setMultiLine (false);
    te.setSelectAllWhenFocused (true);
    te.setJustification (juce::Justification::centredLeft);
    te.setFont (juce::Font (juce::FontOptions (12.0f)));
    te.setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
    te.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    te.setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    te.setColour (juce::TextEditor::highlightColourId, juce::Colour (0xff3a72e0));
    te.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff3a72e0));
    te.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xff3a72e0));
    te.onReturnKey = [this] { commitEntry(); };
    te.onEscapeKey = [this] { cancelEntry(); };
    te.onFocusLost = [this] { if (entryOn) commitEntry(); };
    specDb.assign (300, -120.0f);
    m.onChange = [this] { repaint(); };
    startTimerHz (30);
}

View::~View()
{
    m.onChange = nullptr;
    stopTimer();
}

// =================================================================== aggiornamento
void View::timerCallback() { tick(); }

void View::tick()
{
    for (size_t i = 0; i < VIEWD.size() && i < proc.engine.view.size(); ++i) VIEWD[i] = proc.engine.view[i].load (std::memory_order_relaxed);
    const float sp = (float) proc.engine.specPos.load();
    if (sp != lastScope) { engineAlive = true; sameCount = 0; lastScope = sp; }
    else if (++sameCount > 12) engineAlive = false;
    for (auto& kv : modTrack)
    {
        const double v = VIEWD[(size_t) kv.first];
        kv.second.first = std::min (v, kv.second.first + (v - kv.second.first) * 0.08);
        kv.second.second = std::max (v, kv.second.second + (v - kv.second.second) * 0.08);
    }
    if (engineAlive) computeSpectrum();
    repaint();
}

void View::computeSpectrum()
{
    constexpr int N = 4096;
    static thread_local std::vector<std::complex<double>> buf;
    static thread_local sdsp::Fft fft;
    if (fft.getSize() != N) fft.prepare (N);
    buf.resize (N);
    const int pos = proc.engine.specPos.load (std::memory_order_acquire);
    const double sr = std::max (8000.0f, VIEWD[(size_t) VIEW::LATENCY + 1]);
    for (int i = 0; i < N; ++i)
    {
        const int j = (pos - N + i + sdsp::Engine::SPEC_SIZE) & (sdsp::Engine::SPEC_SIZE - 1);
        const double w = 0.5 - 0.5 * std::cos (2 * juce::MathConstants<double>::pi * i / (N - 1));
        buf[(size_t) i] = proc.engine.specRing[(size_t) j] * w;
    }
    fft.perform (buf.data(), false);
    const int NP2 = (int) specDb.size();
    for (int i = 0; i < NP2; ++i)
    {
        const double f0 = 10 * std::pow (3000.0, (i - 0.5) / (NP2 - 1)), f1 = 10 * std::pow (3000.0, (i + 0.5) / (NP2 - 1));
        const int b0 = std::max (1, (int) (f0 / sr * N)), b1 = std::min (N / 2 - 1, std::max (b0, (int) (f1 / sr * N)));
        double mx = 0;
        for (int j = b0; j <= b1; ++j) mx = std::max (mx, std::norm (buf[(size_t) j]));
        const double fc = 10 * std::pow (3000.0, (double) i / (NP2 - 1));
        const double db = 10 * std::log10 (std::max (mx, 1e-20)) - 20 * std::log10 (N / 4.0) + 4.5 * std::log2 (fc / 1000);
        specDb[(size_t) i] = db > specDb[(size_t) i] ? (float) db : specDb[(size_t) i] - 1.2f;
    }
}

// =================================================================== eventi del mouse
View::M View::mk (const juce::MouseEvent& e) const
{
    M mm;
    mm.x = e.position.x; mm.y = e.position.y;
    const auto mods = e.mods;
    mm.shift = mods.isShiftDown(); mm.alt = mods.isAltDown(); mm.cmd = mods.isCommandDown(); mm.ctrl = mods.isCtrlDown();
    mm.right = mods.isPopupMenu();
    return mm;
}

View::Reg* View::findReg (float x, float y, std::function<bool (const Reg&)> pred)
{
    for (auto it = REG.rbegin(); it != REG.rend(); ++it)
        if (it->r.contains (x, y) && (! pred || pred (*it))) return &*it;
    return nullptr;
}

static bool isReset (const View::M& m)
{
#if JUCE_MAC
    return m.cmd;
#else
    return m.ctrl && ! m.alt;
#endif
}

void View::simDown (float x, float y, M mm)
{
    mm.x = x; mm.y = y;
    if (help) { help = false; repaint(); return; }
    if (entryOn) commitEntry();
    Reg* r = findReg (x, y);
    if (slotp.on && ! slotp.rect.contains (x, y) && (r == nullptr || (r->kind != "moddot" && r->kind != "srcdot"))) slotp.on = false;
    if (r == nullptr) { repaint(); return; }
    if (m.learnMode && r->pname >= 0 && r->kind != "button") { m.learnArm = r->pname; repaint(); return; }
    if (mm.right && r->rdown) { auto c = *r; c.rdown (c, mm); repaint(); return; }
    if (isReset (mm) && r->reset) { auto c = *r; c.reset (c, mm); repaint(); return; }
    active = std::make_unique<Reg> (*r);
    if (active->down) active->down (*active, mm);
    repaint();
}
void View::simDrag (float x, float y, M mm)
{
    mm.x = x; mm.y = y;
    hoverX = x; hoverY = y;
    if (active && active->drag) active->drag (*active, mm);
    repaint();
}
void View::simUp (float x, float y)
{
    M mm; mm.x = x; mm.y = y;
    if (active)
    {
        auto a = std::move (active);
        if (a->up) a->up (*a, mm);
    }
    repaint();
}
void View::simDouble (float x, float y)
{
    if (m.learnMode) return;
    Reg* r = findReg (x, y);
    if (r && r->dbl) { auto c = *r; M mm; mm.x = x; mm.y = y; c.dbl (c, mm); repaint(); }
}
void View::simMove (float x, float y)
{
    hoverX = x; hoverY = y;
    const Reg* r = findReg (x, y, [] (const Reg& q) { return q.kind == "outbtn" || q.kind == "outpanel"; });
    if (outp && r == nullptr) outp = false;
    else if (! outp && r && r->kind == "outbtn") outp = true;
    repaint();
}

void View::mouseDown (const juce::MouseEvent& e) { simDown (e.position.x, e.position.y, mk (e)); }
void View::mouseDrag (const juce::MouseEvent& e) { simDrag (e.position.x, e.position.y, mk (e)); }
void View::mouseUp (const juce::MouseEvent& e) { simUp (e.position.x, e.position.y); }
void View::mouseDoubleClick (const juce::MouseEvent& e) { simDouble (e.position.x, e.position.y); }
void View::mouseMove (const juce::MouseEvent& e) { simMove (e.position.x, e.position.y); }
void View::mouseExit (const juce::MouseEvent&) { hoverX = hoverY = -1; outp = false; repaint(); }
void View::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float d = w.deltaY > 0 ? 1.0f : (w.deltaY < 0 ? -1.0f : 0.0f);
    if (d == 0) return;
    if (Reg* r = findReg (e.position.x, e.position.y, [] (const Reg& q) { return (bool) q.wheel; }))
    {
        auto c = *r;
        c.wheel (c, mk (e), d);
        repaint();
    }
}

bool View::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey)
    {
        if (entryOn) { cancelEntry(); return true; }
        if (help) { help = false; repaint(); return true; }
        if (m.learnMode) { m.learnMode = false; m.learnArm = -1; proc.learnActive.store (false); repaint(); return true; }
        if (fullscreen && onToggleFullscreen) { onToggleFullscreen(); return true; }
    }
    if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'Z')
    {
        if (k.getModifiers().isShiftDown()) m.doRedo(); else m.doUndo();
        return true;
    }
    return false;
}

// =================================================================== trascinamento delle sorgenti
void View::dragHover (const M& mm)
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const Reg* r = findReg (mm.x, mm.y, [] (const Reg& q) { return q.kind == "srcbtn" || q.kind == "dispbg" || q.kind == "levelhandle"; });
    if (r == nullptr) { drag.hr = {}; return; }
    const String id = r->kind + r->srckey;
    if (drag.hr != id) { drag.hr = id; drag.ht = now; return; }
    if (now - drag.ht < 350) return;
    if (r->kind == "srcbtn" && ! isExpanded (r->srckey)) expandSource (r->srckey);
    if (r->kind == "dispbg" || r->kind == "levelhandle")
    {
        const int b = m.bandAtFreq (xf (mm.x));
        if (std::find (m.sel.begin(), m.sel.end(), b) == m.sel.end()) m.sel = { b };
    }
}

void View::dropSource (const M& mm)
{
    auto d = drag;
    drag = {};
    if (! d.on) return;
    if (! d.moved) { openSlotPanelSource (d.si, d.x0, d.y0); return; }
    const Reg* r = findReg (mm.x, mm.y, [] (const Reg& q) { return q.tgt >= 0 && q.kind != "moddot"; });
    if (r == nullptr) { repaint(); return; }
    const int tgt = r->tgt;
    const auto rr = r->r;
    m.pushUndo();
    std::vector<int> targets { tgt };
    int off = 0;
    const int b = Model::bandOfTarget (tgt, &off);
    if (b >= 0 && std::find (m.sel.begin(), m.sel.end(), b) != m.sel.end())
    {
        targets.clear();
        for (int s : m.sel) targets.push_back (params()[(size_t) Model::bp (s, off)].target);
    }
    if (tgt >= SLOT0 && tgt - SLOT0 < (int) m.str.slots.size() && m.str.slots[(size_t) (tgt - SLOT0)].src == d.si)
    {
        flash ("A source cannot modulate its own slot");
        return;
    }
    bool full = false;
    for (int t : targets) if (m.addSlot (d.si, t, 0.5) < 0) full = true;
    if (full) flash ("Maximum number of modulation slots reached (" + String (NSLOTS) + ")");
    openSlotPanelTarget (tgt, rr.getRight(), rr.getY());
}

// =================================================================== solo e mute
void View::soloMute (int b, int what, const M& mm)
{
    smTmp = {};
    smTmp.on = true;
    smTmp.t = juce::Time::getMillisecondCounterHiRes();
    for (int i = 0; i < m.nb(); ++i) smTmp.snap.push_back ({ Model::bp (i, what), m.pv (Model::bp (i, what)) });
    const int name = Model::bp (b, what);
    if (isReset (mm))
    {
        bool others = false;
        for (int j = 0; j < m.nb(); ++j) if (j != b && m.pv (Model::bp (j, what)) > 0.5) others = true;
        const bool onlyThis = m.pv (name) > 0.5 && ! others;
        for (int i = 0; i < m.nb(); ++i) m.setParam (Model::bp (i, what), (i == b && ! onlyThis) ? 1 : 0);
    }
    else m.setParam (name, m.pv (name) > 0.5 ? 0 : 1);
}
void View::soloMuteUp()
{
    if (smTmp.on && juce::Time::getMillisecondCounterHiRes() - smTmp.t > 450)
        for (auto& s : smTmp.snap) m.setParam (s.first, s.second);   // tenuto premuto: solo/mute temporaneo
    smTmp = {};
}

// =================================================================== campi di testo
void View::openEntry (const String& text)
{
    const float x = juce::jlimit (2.0f, (float) BW - 122, entry.x), y = juce::jlimit (2.0f, (float) BH - 22, entry.y);
    te.setBounds (juce::roundToInt (x), juce::roundToInt (y), 120, 20);
    te.setText (text, false);
    te.setVisible (true);
    te.toFront (true);
    te.grabKeyboardFocus();
    te.selectAll();
    entryOn = true;
    repaint();
}
void View::startEntry (int pname, float x, float y, bool band, int off)
{
    entry = {};
    entry.kind = 0; entry.pname = pname; entry.band = band; entry.off = off; entry.x = x; entry.y = y;
    const auto& p = params()[(size_t) pname];
    entry.hint = p.unit == 3 ? "e.g. 1k, A4, C#3+13" : (p.unit == 4 ? "e.g. -3, 2x" : (p.unit == 2 ? "e.g. 250ms, 1.5s" : ""));
    String t = Model::fmtVal (pname, m.pv (pname));
    if (p.unit == 3) t = String (juce::roundToInt (m.pv (pname)));
    else if (p.unit == 4 || p.unit == 5) t = t.upToFirstOccurrenceOf (" ", false, false).trimCharactersAtEnd ("%");
    openEntry (t);
}
void View::startNameEntry (const String& key, float x, float y)
{
    entry = {}; entry.kind = 1; entry.key = key; entry.x = x; entry.y = y;
    openEntry (m.srcName (key));
}
void View::startSlotEntry (int i, float x, float y)
{
    entry = {}; entry.kind = 2; entry.slot = i; entry.x = x; entry.y = y; entry.hint = "0..100 %";
    openEntry (String (juce::roundToInt (m.str.slots[(size_t) i].amt * 100)));
}
void View::startTextEntry (float x, float y, const String& text, std::function<void (String)> fn, const String& hint)
{
    entry = {}; entry.kind = 3; entry.fn = std::move (fn); entry.x = x; entry.y = y; entry.hint = hint;
    openEntry (text);
}
void View::cancelEntry()
{
    entryOn = false;
    te.setVisible (false);
    grabKeyboardFocus();
    repaint();
}
void View::commitEntry()
{
    if (! entryOn) return;
    entryOn = false;
    const String t = te.getText().trim();
    te.setVisible (false);
    auto e = entry;
    if (e.kind == 0)
    {
        double v;
        if (Model::parseEntry (e.pname, t, v))
        {
            m.pushUndo();
            if (e.band && e.off >= 0) m.setBandParam (e.off, v);
            else m.setParam (e.pname, v);
        }
    }
    else if (e.kind == 1)
    {
        m.pushUndo();
        if (t.isNotEmpty()) m.str.names[e.key] = t.substring (0, 24); else m.str.names.erase (e.key);
        m.structChanged();
    }
    else if (e.kind == 2)
    {
        if (t.containsAnyOf ("0123456789") && e.slot < (int) m.str.slots.size())
        {
            m.pushUndo();
            m.str.slots[(size_t) e.slot].amt = juce::jlimit (0.0, 1.0, t.getDoubleValue() / 100.0);
            m.structChanged();
        }
    }
    else if (e.kind == 3 && t.isNotEmpty() && e.fn) e.fn (t);
    repaint();
}

// =================================================================== menu
void View::showMenu (juce::PopupMenu& menu, float x, float y)
{
    lastMenuItems = menu.getNumItems();
    if (menuHook) { menuHook (menu, nullptr); return; }
    const auto area = localAreaToGlobal (juce::Rectangle<int> (juce::roundToInt (x), juce::roundToInt (y), 1, 1));
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (area).withParentComponent (nullptr),
                        [this] (int) { lastMenuItems = 0; repaint(); });
}

void View::openPresetMenu (float x, float y)
{
    juce::PopupMenu menu;
    auto list = m.allPresets();
    std::map<String, juce::PopupMenu> cats;
    juce::StringArray order;
    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto& e = list[i];
        String label = e.name;
        if (m.pgmChanges) label = "(" + String ((int) i / 128) + "/" + String ((int) i % 128) + ") " + label;
        auto f = e.file;
        if (e.cat.isEmpty()) menu.addItem (label, true, m.str.presetName == e.name, [this, f] { m.loadPresetFile (f); });
        else
        {
            if (! order.contains (e.cat)) order.add (e.cat);
            cats[e.cat].addItem (label, true, m.str.presetName == e.name, [this, f] { m.loadPresetFile (f); });
        }
    }
    if (! list.empty()) menu.addSeparator();
    for (auto& c : order) menu.addSubMenu (c.replace ("__", ""), cats[c], true, nullptr, false);
    menu.addSeparator();
    menu.addItem ("Save As...", [this] { savePresetAsDialog(); });
    juce::PopupMenu opt;
    opt.addItem ("Save As Default", [this] { m.saveAsDefault(); flash ("Default setting saved"); });
    opt.addItem ("Open Other Preset...", [this] { openOtherPresetDialog(); });
    opt.addItem ("Change Preset Folder...", [this] { changePresetFolderDialog(); });
    opt.addItem ("Restore Factory Presets", [this] { m.ensureFactoryPresets (true); flash ("Factory presets restored"); });
    opt.addSeparator();
    opt.addItem ("Enable MIDI Program Changes", true, m.pgmChanges, [this] { m.pgmChanges = ! m.pgmChanges; });
    menu.addSubMenu ("Options", opt);
    showMenu (menu, x, y);
}

void View::openPresetShortMenu (float x, float y)
{
    juce::PopupMenu menu;
    menu.addItem ("Save", [this]
    {
        auto list = m.allPresets();
        juce::File target;
        for (auto& e : list) if (e.name == m.str.presetName) target = e.file;
        if (target == juce::File()) { savePresetAsDialog(); return; }
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Save preset", "Overwrite '" + m.str.presetName + "'?", "Save", "Cancel", this,
            juce::ModalCallbackFunction::create ([this, target] (int r) { if (r == 1) { m.savePresetFile (target.withFileExtension (Model::presetExt())); flash ("Preset saved"); } }));
    });
    menu.addItem ("Save As...", [this] { savePresetAsDialog(); });
    showMenu (menu, x, y);
}

void View::savePresetAsDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Save preset", m.presetFolder().getChildFile (m.str.presetName + Model::presetExt()), "*" + Model::presetExt());
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f != juce::File()) { if (m.savePresetFile (f)) flash ("Preset saved: " + f.getFileNameWithoutExtension()); else flash ("Could not save the preset"); }
        });
}
void View::openOtherPresetDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Open preset", m.presetFolder(), "*" + Model::presetExt() + ";*.json");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.existsAsFile() && ! m.loadPresetFile (f)) flash ("Not a valid preset"); });
}
void View::changePresetFolderDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Preset folder", m.presetFolder());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.isDirectory()) { m.setPresetFolder (f); flash ("Preset folder: " + f.getFullPathName()); } });
}

void View::openHelpMenu (float x, float y)
{
    juce::PopupMenu menu;
    menu.addItem ("Quick Help", [this] { help = true; repaint(); });
    menu.addItem ("About SATURA", [this] { flash (String ("SATURA ") + SATURA_VERSION + " - Collateral Effects"); });
    showMenu (menu, x, y);
}

void View::openStyleMenu (float x, float y)
{
    juce::PopupMenu menu;
    const int cur = juce::jlimit (0, NSTYLES - 1, (int) std::lround (m.pv (Model::bp (m.sel[0], BP::style))));
    juce::StringArray cats;
    for (auto& s : styles()) cats.addIfNotAlreadyThere (s.cat);
    for (auto& c : cats)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < NSTYLES; ++i)
            if (c == styles()[(size_t) i].cat)
                sub.addItem (styles()[(size_t) i].name, true, cur == i, [this, i] { m.pushUndo(); m.setBandParam (BP::style, i); });
        menu.addSubMenu (c, sub, true, nullptr, c == String (styles()[(size_t) cur].cat));
    }
    showMenu (menu, x, y);
}

void View::openSectionMenu (const String& type, int k, float x, float y)
{
    juce::PopupMenu menu;
    bool sepDone = false;
    for (auto& sp : m.sectionPresets (type))
    {
        if (sp.user && ! sepDone) { menu.addSeparator(); sepDone = true; }
        auto d = sp.d; auto name = sp.name;
        menu.addItem (name, [this, type, k, d, name]
        {
            m.pushUndo();
            if (type == "band") { for (int b : m.sel) m.applySectionPreset ("band", b, d); }
            else
            {
                m.applySectionPreset (type, k, d);
                const String key = type + String (k + 1);
                if (name != "Default") m.str.names[key] = name; else m.str.names.erase (key);
                m.structChanged();
            }
        });
    }
    menu.addSeparator();
    menu.addItem ("Save As...", [this, type, k, x, y]
    {
        startTextEntry (x, y - 20, type == "band" ? String ("My Band") : m.srcName (type + String (k + 1)),
                        [this, type, k] (String n) { m.saveSectionPreset (type, k, n); flash ("Section preset saved"); }, "preset name");
    });
    menu.addItem ("Save As Default", [this, type, k] { m.saveSectionDefault (type, k); flash ("Default section preset saved"); });
    showMenu (menu, x, y);
}

void View::openAddMenu (float x, float y)
{
    juce::PopupMenu menu;
    auto add = [this] (String type, bool slider)
    {
        return [this, type, slider]
        {
            m.pushUndo();
            const String key = m.addSource (type, slider);
            if (key.isEmpty()) flash ("No more sources of this type available");
            else if (type != "xy") { if (m.autoCollapse) m.expanded = key; else { m.expList.add (key); m.expanded = key; } }
            m.showMod = true;
            repaint();
        };
    };
    menu.addItem ("New XLFO", add ("xlfo", false));
    menu.addItem ("New Envelope Generator", add ("eg", false));
    menu.addItem ("New Envelope Follower", add ("ef", false));
    menu.addItem ("New MIDI Source", add ("midi", false));
    menu.addItem ("New XY Controller", add ("xy", false));
    menu.addItem ("New Slider", add ("xy", true));
    menu.addSeparator();
    menu.addItem ("Auto-Collapse Sources", true, m.autoCollapse, [this] { m.autoCollapse = ! m.autoCollapse; repaint(); });
    menu.addItem ("Show Source Flow", true, m.showFlow, [this] { m.showFlow = ! m.showFlow; repaint(); });
    showMenu (menu, x, y);
}

void View::openSourceMenu (const String& key, float x, float y)
{
    juce::PopupMenu menu;
    String t; int k; Model::srcKeyParts (key, t, k);
    if (t != "xy")
    {
        bool sepDone = false;
        for (auto& sp : m.sectionPresets (t))
        {
            if (sp.user && ! sepDone) { menu.addSeparator(); sepDone = true; }
            auto d = sp.d; auto name = sp.name;
            menu.addItem (name, [this, t, k, key, d, name] { m.pushUndo(); m.applySectionPreset (t, k, d); if (name != "Default") m.str.names[key] = name; else m.str.names.erase (key); m.structChanged(); });
        }
        menu.addSeparator();
        menu.addItem ("Save As...", [this, t, k, key, x, y] { startTextEntry (x, y - 20, m.srcName (key), [this, t, k] (String n) { m.saveSectionPreset (t, k, n); flash ("Section preset saved"); }, "preset name"); });
        menu.addItem ("Save As Default", [this, t, k] { m.saveSectionDefault (t, k); flash ("Default saved"); });
    }
    else
    {
        auto& o = m.str.xy[(size_t) k];
        menu.addItem ("XY Mode", true, o.mode == 0, [this, k] { m.pushUndo(); m.str.xy[(size_t) k].mode = 0; m.structChanged(); });
        menu.addItem ("Slider Mode", true, o.mode == 1, [this, k, key]
        {
            m.pushUndo(); m.str.xy[(size_t) k].mode = 1;
            const int sx = Model::srcIndexOf (key, "x");
            for (int z = (int) m.str.slots.size() - 1; z >= 0; --z) if (m.str.slots[(size_t) z].src == sx) m.deleteSlot (z, true);
            m.structChanged();
        });
        menu.addSeparator();
        menu.addItem ("Bipolar (-1 to 1)", true, o.range == 0, [this, k] { m.pushUndo(); m.str.xy[(size_t) k].range = 0; m.structChanged(); });
        menu.addItem ("Unipolar (0 to 1)", true, o.range == 1, [this, k] { m.pushUndo(); m.str.xy[(size_t) k].range = 1; m.structChanged(); });
    }
    menu.addSeparator();
    menu.addItem ("Rename...", [this, key, x, y] { startNameEntry (key, x, y - 20); });
    menu.addItem ("Remove", [this, key] { m.pushUndo(); m.removeSource (key); });
    showMenu (menu, x, y);
}

void View::openRateMenu (int k, float x, float y)
{
    juce::PopupMenu menu;
    auto& X = m.str.xlfo[(size_t) k];
    const int pdiv = P::x1_div + k * 6;
    menu.addItem ("Free (Hz)", true, ! X.sync, [this, k] { m.pushUndo(); m.str.xlfo[(size_t) k].sync = 0; m.structChanged(); });
    menu.addSeparator();
    for (int d = 0; d < 11; ++d)
        menu.addItem (String (syncDivs[d]) + (d <= 4 ? (d == 4 ? " bar" : " bars") : ""), true, X.sync && (int) std::lround (m.pv (pdiv)) == d,
                      [this, k, d, pdiv] { m.pushUndo(); m.str.xlfo[(size_t) k].sync = 1; m.setParam (pdiv, d); m.structChanged(); });
    showMenu (menu, x, y);
}

void View::openRetrigMenu (int k, float x, float y)
{
    juce::PopupMenu menu;
    const char* l[] = { "Free Running", "MIDI Retrigger", "MIDI Legato" };
    for (int i = 0; i < 3; ++i) menu.addItem (l[i], true, m.str.xlfo[(size_t) k].retrig == i, [this, k, i] { m.pushUndo(); m.str.xlfo[(size_t) k].retrig = i; m.structChanged(); });
    showMenu (menu, x, y);
}

void View::openTrigMenu (const String& type, int k, float x, float y)
{
    juce::PopupMenu menu;
    const int cur = type == "eg" ? m.str.eg[(size_t) k].trig : m.str.ef[(size_t) k].trig;
    juce::StringArray names { "Main Input", "External Side Chain" };
    for (int b = 0; b < m.nb(); ++b) names.add ("Band " + String (b + 1) + " Input");
    auto setT = [this, type, k] (int t) { m.pushUndo(); if (type == "eg") m.str.eg[(size_t) k].trig = t; else m.str.ef[(size_t) k].trig = t; m.structChanged(); };
    for (int i = 0; i < names.size(); ++i) menu.addItem (names[i], true, cur == i, [setT, i] { setT (i); });
    if (type == "eg") menu.addItem ("MIDI", true, cur == 8, [setT] { setT (8); });
    showMenu (menu, x, y);
}

void View::openLearnMenu (float x, float y)
{
    juce::PopupMenu menu;
    menu.addItem ("Enable MIDI", true, m.midiEnable, [this] { m.midiEnable = ! m.midiEnable; m.pushLearnToProcessor(); });
    juce::PopupMenu clear;
    for (auto& kv : m.learn)
    {
        const int cc = kv.first;
        clear.addItem ("CC " + String (cc) + String (juce::CharPointer_UTF8 (" \xe2\x86\x92 ")) + params()[(size_t) kv.second].label, [this, cc] { m.clearLearn (cc); });
    }
    if (m.learn.empty()) clear.addItem ("(no associations)", false, false, nullptr);
    else { clear.addSeparator(); clear.addItem ("Clear All", [this] { m.clearLearn (-1); }); }
    menu.addSubMenu ("Clear", clear);
    menu.addItem ("Revert", [this] { m.revertLearn(); });
    menu.addItem ("Save", [this] { m.saveLearn(); flash ("MIDI mapping saved"); });
    showMenu (menu, x, y);
}

void View::openResizeMenu (float x, float y)
{
    juce::PopupMenu menu;
    for (int i = 0; i < 4; ++i)
        menu.addItem (SIZE_NAMES[i], true, m.sizeIdx == i && ! fullscreen, [this, i] { m.sizeIdx = i; m.savePrefs(); if (onSizeChanged) onSizeChanged(); });
    menu.addSeparator();
    juce::PopupMenu sc;
    for (double s : SCALES)
        sc.addItem (String (juce::roundToInt (s * 100)) + "%" + (s == 1.0 ? " (Default)" : ""), true, std::abs (m.scaleMul - s) < 1e-6,
                    [this, s] { m.scaleMul = s; m.savePrefs(); if (onSizeChanged) onSizeChanged(); });
    menu.addSubMenu ("Scaling", sc);
    showMenu (menu, x, y);
}

juce::PopupMenu View::slotTargetMenu (std::function<void (int)> fn)
{
    juce::PopupMenu menu;
    auto addGroup = [&] (const String& name, const std::vector<int>& ps)
    {
        juce::PopupMenu sub;
        for (int p : ps) { const int t = params()[(size_t) p].target; if (t >= 0) sub.addItem (m.targetName (t), [fn, t] { fn (t); }); }
        if (sub.getNumItems() > 0) menu.addSubMenu (name, sub);
    };
    addGroup ("Global", { P::gain_in, P::pan_in, P::gain_out, P::pan_out, P::gmix });
    std::vector<int> xs;
    for (int i = 0; i < m.nb() - 1; ++i) xs.push_back (P::xf1 + i);
    addGroup ("Crossovers", xs);
    for (int b = 0; b < m.nb(); ++b)
    {
        std::vector<int> ps;
        for (int j = 0; j < NBP; ++j) ps.push_back (Model::bp (b, j));
        addGroup ("Band " + String (b + 1), ps);
    }
    for (auto& key : m.str.order)
    {
        String t; int k; Model::srcKeyParts (key, t, k);
        if (t == "xy" || t == "midi" || ! m.srcActive (key)) continue;
        const String pre = (t == "xlfo" ? "x" : t == "eg" ? "e" : "f") + String (k + 1) + "_";
        std::vector<int> ps;
        for (int i = 0; i < NP; ++i) if (String (params()[(size_t) i].id).startsWith (pre)) ps.push_back (i);
        addGroup (m.srcName (key), ps);
    }
    return menu;
}

void View::openSlotChangeMenu (int i, float x, float y)
{
    juce::PopupMenu menu, srcs;
    const int curSrc = m.str.slots[(size_t) i].src;
    for (auto& key : m.str.order)
    {
        if (! m.srcActive (key)) continue;
        String t; int k; Model::srcKeyParts (key, t, k);
        juce::StringArray axes;
        if (t == "xy") { if (m.str.xy[(size_t) k].mode == 1) axes.add ("y"); else { axes.add ("x"); axes.add ("y"); } }
        else axes.add ("");
        for (auto& ax : axes)
        {
            const int si = Model::srcIndexOf (key, ax.isEmpty() ? "y" : ax);
            srcs.addItem (m.srcLabelForSlot (si), true, curSrc == si, [this, i, si] { m.pushUndo(); m.str.slots[(size_t) i].src = si; m.structChanged(); });
        }
    }
    menu.addSubMenu ("Source", srcs);
    menu.addSubMenu ("Target", slotTargetMenu ([this, i] (int t) { m.pushUndo(); m.str.slots[(size_t) i].tgt = t; m.structChanged(); }));
    showMenu (menu, x, y);
}
