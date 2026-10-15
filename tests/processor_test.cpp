// Test del plugin completo: processor, stato, preset, latenza, side chain, MIDI e scenario d'uso dell'interfaccia
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace spec;
static int fails = 0;
#define CHECK(c, ...) do { printf ((c) ? "PASS " : "FAIL "); printf (__VA_ARGS__); printf ("\n"); if (!(c)) ++fails; } while (0)

static void run (SaturaProcessor& p, juce::AudioBuffer<float>& b, juce::MidiBuffer& mid, int blocks, float freq = 440, float amp = 0.3f, bool sc = false)
{
    static int t = 0;
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < b.getNumSamples(); ++i, ++t)
        {
            const float v = amp * std::sin (2 * juce::MathConstants<float>::pi * freq * t / 48000.0f);
            b.setSample (0, i, v); b.setSample (1, i, v);
            if (b.getNumChannels() > 2) { b.setSample (2, i, sc ? 0.5f * std::sin (t * 0.05f) : 0.0f); b.setSample (3, i, b.getSample (2, i)); }
        }
        p.processBlock (b, mid);
        mid.clear();
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    // cartelle di prova isolate
    auto home = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("satura_test_home");
    home.deleteRecursively();
    home.createDirectory();
    setenv ("HOME", home.getFullPathName().toRawUTF8(), 1);

    SaturaProcessor p;
    auto& m = p.model;
    // ---- bus: stereo + side chain
    auto layout = p.getBusesLayout();
    layout.inputBuses.getReference (1) = juce::AudioChannelSet::stereo();
    CHECK (p.setBusesLayout (layout), "layout stereo con side chain stereo accettato");
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add (juce::AudioChannelSet::mono()); mono.inputBuses.add (juce::AudioChannelSet::disabled());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    CHECK (p.checkBusesLayoutSupported (mono), "layout mono accettato");
    p.setRateAndBufferSizeDetails (48000, 256);
    p.prepareToPlay (48000, 256);
    juce::AudioBuffer<float> buf (4, 256);
    juce::MidiBuffer mid;
    run (p, buf, mid, 20);
    CHECK (p.getLatencySamples() == 0, "latenza 0 in modalità normale");
    // ---- preset di fabbrica su disco
    auto list = m.allPresets();
    CHECK (list.size() == 22, "preset di fabbrica installati nella cartella dei preset: %d", (int) list.size());
    CHECK (m.str.presetName == "Default Setting", "all'avvio carica Default Setting");
    // ---- Linear Phase: latenza dichiarata
    m.setParam (P::linphase, 1);
    run (p, buf, mid, 40);
    p.model.proc.getLatencySamples();
    p.handleMessages();
    CHECK (p.getLatencySamples() == 2304, "Linear Phase: latenza dichiarata all'host %d campioni", p.getLatencySamples());
    m.setParam (P::hq, 2);
    run (p, buf, mid, 40);
    p.handleMessages();
    CHECK (p.getLatencySamples() == 2380, "Linear Phase + Superb: latenza %d", p.getLatencySamples());
    m.setParam (P::linphase, 0); m.setParam (P::hq, 0);
    run (p, buf, mid, 40);
    p.handleMessages();
    // ---- side chain: EG attivato dal bus esterno
    m.addSource ("eg");
    m.str.eg[0].trig = 1;
    m.structChanged();
    double peak = 0;
    for (int i = 0; i < 40; ++i) { run (p, buf, mid, 1, 440, 0.0f, true); peak = std::max (peak, (double) p.engine.view[(size_t) (VIEW::SRC + SRC::eg1)].load()); }
    CHECK (peak > 0.9, "EG sul side chain esterno (bus dell'host): picco %.2f", peak);
    // ---- MIDI: program change carica un preset; MIDI Learn
    m.pgmChanges = true;
    mid.addEvent (juce::MidiMessage::programChange (1, 3), 0);
    run (p, buf, mid, 1);
    p.handleMessages();
    CHECK (m.str.presetName == list[3].name, "Program Change 3 carica '%s'", m.str.presetName.toRawUTF8());
    m.learnMode = true; m.learnArm = P::b1_drive; p.learnActive = true;
    mid.addEvent (juce::MidiMessage::controllerEvent (1, 21, 64), 0);
    run (p, buf, mid, 1);
    p.handleMessages();
    m.learnMode = false; p.learnActive = false;
    mid.addEvent (juce::MidiMessage::controllerEvent (1, 21, 127), 0);
    run (p, buf, mid, 1);
    p.handleMessages();
    CHECK (std::abs (m.pv (P::b1_drive) - 48) < 0.01, "MIDI Learn: CC21 comanda il Drive (%.1f dB)", m.pv (P::b1_drive));
    // ---- stato: salva e ripristina in un'altra istanza
    m.loadDefaultPreset();
    m.addBandAt (2000);
    m.addSource ("xlfo");
    const int si = Model::srcIndexOf ("xlfo1");
    m.addSlot (si, params()[(size_t) P::b2_drive].target, 0.7);
    juce::MemoryBlock state;
    p.getStateInformation (state);
    SaturaProcessor p2;
    p2.setStateInformation (state.getData(), (int) state.getSize());
    CHECK (p2.model.nb() == 2 && p2.model.str.slots.size() == 1 && std::abs (p2.model.str.slots[0].amt - 0.7) < 1e-9 && p2.model.str.xlfo[0].active,
           "stato del progetto: bande, sorgenti e slot ripristinati (%d bande, %d slot)", p2.model.nb(), (int) p2.model.str.slots.size());
    // ---- preset utente: salva e ricarica, compatibile con la versione Max (JSON)
    auto f = m.presetFolder().getChildFile ("Test").getChildFile ("Mio preset.cep");
    CHECK (m.savePresetFile (f) && f.existsAsFile(), "salvataggio preset utente");
    m.loadDefaultPreset();
    CHECK (m.nb() == 1 && m.str.slots.empty(), "Default Setting ricaricato");
    CHECK (m.loadPresetFile (f) && m.nb() == 2 && m.str.slots.size() == 1, "preset utente ricaricato");
    // ---- undo/redo e A/B
    m.pushUndo(); m.setParam (P::b1_mix, 30);
    m.doUndo();
    CHECK (std::abs (m.pv (P::b1_mix) - 100) < 1e-6, "undo");
    m.doRedo();
    CHECK (std::abs (m.pv (P::b1_mix) - 30) < 1e-3, "redo");
    m.abSwitch(); m.setParam (P::b1_mix, 80); m.abSwitch();
    CHECK (std::abs (m.pv (P::b1_mix) - 30) < 1e-3, "A/B");
    // ---- inserimento di testo
    double v;
    CHECK (Model::parseEntry (P::b1_fbf, "A4", v) && std::abs (v - 440) < 0.01, "testo 'A4' = 440 Hz");
    CHECK (Model::parseEntry (P::b1_fbf, "1k", v) && std::abs (v - 1000) < 0.01, "testo '1k' = 1000 Hz");
    CHECK (Model::parseEntry (P::b1_fbf, "C#3+13", v) && std::abs (v - 139.6) < 0.1, "testo 'C#3+13' = %.1f Hz", v);
    CHECK (Model::parseEntry (P::b1_drive, "2x", v) && std::abs (v - 6.02) < 0.01, "testo '2x' = +6 dB");
    CHECK (Model::parseEntry (P::b1_mix, "50%", v) && std::abs (v - 50) < 0.01, "testo '50%%'");

    // =================================================== scenario dell'interfaccia
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto& view = dynamic_cast<SaturaEditor*> (ed.get())->getView();
    ed->setSize (View::BW, View::BH);
    m.loadDefaultPreset();
    juce::PopupMenu lastMenu;
    view.menuHook = [&] (juce::PopupMenu& pm, std::function<void (int)>) { lastMenu = pm; };
    auto snap = [&] (const char* name) { auto img = ed->createComponentSnapshot (ed->getLocalBounds()); juce::File out ("/tmp/" + juce::String (name) + ".png"); out.deleteFile(); juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os); };
    auto repaintNow = [&] { auto img = ed->createComponentSnapshot (ed->getLocalBounds()); juce::ignoreUnused (img); };
    auto find = [&] (std::function<bool (const View::Reg&)> pr) -> const View::Reg*
    {
        repaintNow();
        auto& R = view.regions();
        for (auto it = R.rbegin(); it != R.rend(); ++it) if (pr (*it)) return &*it;
        return nullptr;
    };
    auto click = [&] (const View::Reg* r) { const auto c = r->r.getCentre(); view.simMove (c.x, c.y); repaintNow(); view.simDown (c.x, c.y); view.simUp (c.x, c.y); repaintNow(); };
    // trova una voce in un menu (anche nei sottomenu) e la esegue
    std::function<bool (juce::PopupMenu&, const juce::String&)> pick = [&] (juce::PopupMenu& pm, const juce::String& label) -> bool
    {
        for (juce::PopupMenu::MenuItemIterator it (pm, true); it.next();)
        {
            auto& item = it.getItem();
            if (item.text == label && item.action) { item.action(); return true; }
        }
        return false;
    };
    // menu dei preset
    m.pgmChanges = false;
    click (find ([] (const View::Reg& r) { return r.bid == "preset"; }));
    CHECK (pick (lastMenu, "Plexi Crunch") && m.str.presetName == "Plexi Crunch", "menu dei preset: scelta di 'Plexi Crunch'");
    // menu Help
    click (find ([] (const View::Reg& r) { return r.bid == "help"; }));
    CHECK (pick (lastMenu, "Quick Help"), "menu Help: Quick Help");
    view.simDown (450, 300); view.simUp (450, 300);
    // stili
    click (find ([] (const View::Reg& r) { return r.kind == "style"; }));
    CHECK (pick (lastMenu, "Extreme Tape") && std::lround (m.pv (P::b1_style)) == 8, "menu degli stili: Extreme Tape");
    // aggiunta bande con '+'
    view.simMove (620, 45);
    auto* add = find ([] (const View::Reg& r) { return r.kind == "addband"; });
    CHECK (add != nullptr, "pulsante '+' sul display");
    if (add) click (add);
    CHECK (m.nb() == 2, "aggiunta di una banda: %d bande", m.nb());
    // trascinamento del Drive
    m.sel = { 0 };
    auto* dk = find ([] (const View::Reg& r) { return r.kind == "knob" && r.pname == P::b1_drive; });
    const double d0 = m.pv (P::b1_drive);
    view.simDown (dk->cx, dk->cy + 10); view.simDrag (dk->cx, dk->cy - 40); view.simUp (dk->cx, dk->cy - 40);
    CHECK (m.pv (P::b1_drive) > d0 + 3, "drive trascinato: %.1f -> %.1f dB", d0, m.pv (P::b1_drive));
    // sorgenti dal menu '+'
    click (find ([] (const View::Reg& r) { return r.bid == "addsrc"; }));
    CHECK (pick (lastMenu, "New XLFO") && m.srcActive ("xlfo1"), "menu '+': nuova XLFO");
    click (find ([] (const View::Reg& r) { return r.bid == "addsrc"; }));
    pick (lastMenu, "New Envelope Generator");
    click (find ([] (const View::Reg& r) { return r.bid == "addsrc"; }));
    pick (lastMenu, "New XY Controller");
    m.expanded = {};
    // drag & drop: XLFO -> Drive
    auto* dot = find ([] (const View::Reg& r) { return r.kind == "srcdot" && r.srckey == "xlfo1"; });
    auto* dk2 = find ([] (const View::Reg& r) { return r.kind == "knob" && r.pname == P::b1_drive; });
    const auto c0 = dot->r.getCentre(); const float tx = dk2->cx, ty = dk2->cy;
    view.simDown (c0.x, c0.y); view.simDrag (400, 420); view.simDrag (tx, ty);
    snap ("satura_drag");
    view.simUp (tx, ty);
    CHECK (m.str.slots.size() == 1 && m.str.slots[0].tgt == params()[P::b1_drive].target, "drag & drop della XLFO sul Drive crea lo slot");
    // doppio clic: campo di testo nativo
    auto* fb = find ([] (const View::Reg& r) { return r.kind == "knob" && r.pname == P::b1_fbf; });
    view.simDouble (fb->cx, fb->cy);
    CHECK (view.textEntry().isVisible(), "doppio clic: compare il campo di testo");
    view.textEntry().setText ("A4");
    view.textEntry().onReturnKey();
    CHECK (std::abs (m.pv (P::b1_fbf) - 440) < 0.5 && ! view.textEntry().isVisible(), "testo 'A4' + Invio: %.1f Hz", m.pv (P::b1_fbf));
    // sorgenti espanse
    for (auto key : { "xlfo1", "eg1" })
    {
        auto* sb = find ([key] (const View::Reg& r) { return r.kind == "srcbtn" && r.srckey == key; });
        CHECK (sb != nullptr, "pulsante della sorgente %s", key);
        if (sb) click (sb);
        CHECK (m.expanded == key, "espansione di %s", key);
        snap (key);
    }
    // simulazione dei dati dal motore e screenshot completo
    run (p, buf, mid, 100);
    view.tick(); view.tick();
    m.expanded = {};
    snap ("satura_full");
    // fuzz: eventi casuali su tutta l'interfaccia
    juce::Random rng (7);
    int nEv = 0;
    for (; nEv < 4000; ++nEv)
    {
        repaintNow();
        auto& R = view.regions();
        float x = rng.nextFloat() * View::BW, y = rng.nextFloat() * View::BH;
        if (rng.nextFloat() < 0.6f && ! R.empty()) { auto& r = R[(size_t) rng.nextInt ((int) R.size())]; x = r.r.getX() + rng.nextFloat() * r.r.getWidth(); y = r.r.getY() + rng.nextFloat() * r.r.getHeight(); }
        View::M mm; mm.shift = rng.nextFloat() < 0.15f; mm.alt = rng.nextFloat() < 0.1f; mm.cmd = rng.nextFloat() < 0.08f; mm.right = rng.nextFloat() < 0.05f;
        const float t = rng.nextFloat();
        if (t < 0.4f) { view.simMove (x, y); view.simDown (x, y, mm); for (int k = rng.nextInt (4); k > 0; --k) view.simDrag (x + rng.nextFloat() * 80 - 40, y + rng.nextFloat() * 80 - 40, mm); view.simUp (x, y); }
        else if (t < 0.55f) view.simMove (x, y);
        else if (t < 0.62f) { view.simDouble (x, y); if (view.textEntry().isVisible()) { view.textEntry().setText (rng.nextBool() ? "1k" : "-3"); view.textEntry().onReturnKey(); } }
        else if (t < 0.75f && lastMenu.getNumItems() > 0)
        {
            std::vector<std::function<void()>> acts;
            for (juce::PopupMenu::MenuItemIterator it (lastMenu, true); it.next();) if (it.getItem().action && ! it.getItem().text.endsWith ("...") && it.getItem().text != "Restore Factory Presets" && it.getItem().text != "Save") acts.push_back (it.getItem().action);
            if (! acts.empty()) acts[(size_t) rng.nextInt ((int) acts.size())]();
            lastMenu = {};
        }
        else if (t < 0.8f) run (p, buf, mid, 1);
        else if (t < 0.83f) { if (rng.nextBool()) m.doUndo(); else m.doRedo(); }
        else if (t < 0.85f) { juce::MemoryBlock st; p.getStateInformation (st); p.setStateInformation (st.getData(), (int) st.getSize()); }
        else view.tick();
    }
    bool ok = true;
    for (auto& s : m.str.slots) if (s.src <= 0 || s.src >= NSRC || s.tgt < 0 || s.tgt >= NT) ok = false;
    CHECK (ok, "fuzz di %d eventi senza errori, struttura coerente (%d bande, %d slot, %d sorgenti)", nEv, m.nb(), (int) m.str.slots.size(), m.str.order.size());
    float mx = 0;
    for (int i = 0; i < 40; ++i) { run (p, buf, mid, 1); mx = std::max (mx, buf.getMagnitude (0, 0, 256)); }
    CHECK (std::isfinite (mx) && mx < 50, "audio finito dopo il fuzz (picco %.2f)", mx);
    printf (fails ? "%d test falliti\n" : "Tutti i test superati\n", fails);
    home.deleteRecursively();
    return fails ? 1 : 0;
}
