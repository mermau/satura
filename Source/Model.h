// SATURA - modello dell'interfaccia (thread dei messaggi): struttura della modulazione, preset,
// undo/redo, A/B, preset di sezione, MIDI Learn, aggiunta/rimozione di bande e sorgenti.
// Porting di ui/core.js della versione Max for Live; i preset sono nello stesso formato JSON.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Spec.h"
#include <functional>
#include <map>

class SaturaProcessor;

struct Step { double v = 0; int r = 0; int c = 3; double g = 1; };
struct XlfoS { int active = 0, sync = 0, retrig = 0, snap = 0; std::vector<Step> steps { { 1, 0, 3, 1 }, { -1, 0, 3, 1 } }; };
struct EgS { int active = 0, trig = 0, range = 0; double as = 0.3, ds = 0.5, rs = 0.5; };
struct EfS { int active = 0, trig = 0, mode = 0; };
struct MidiS { int active = 0, input = 0, cc = 1, curve = 0; };
struct XyS { int active = 0, mode = 0, range = 0; };
struct Slot { int on = 1, src = 0, tgt = 0; double amt = 0.5; int inv = 0; };

struct Struct
{
    std::array<XlfoS, spec::NXLFO> xlfo;
    std::array<EgS, spec::NEG> eg;
    std::array<EfS, spec::NEF> ef;
    std::array<MidiS, spec::NMIDI> midi;
    std::array<XyS, spec::NXY> xy;
    std::vector<Slot> slots;
    juce::StringArray order;
    std::map<juce::String, juce::String> names;
    juce::String presetName { "Default Setting" };

    juce::var toVar() const;
    static Struct fromVar (const juce::var& v);
};

class Model
{
public:
    explicit Model (SaturaProcessor& p);

    // ---- parametri
    double pv (int i) const;
    void setParam (int i, double v);               // dall'interfaccia: con gesto per l'automazione
    void setParamNorm (int i, double n) { setParam (i, denorm (i, n)); }
    double getNorm (int i) const { return norm (i, pv (i)); }
    static double norm (int i, double v);
    static double denorm (int i, double n);
    static juce::String fmtVal (int i, double v);
    static bool parseEntry (int i, const juce::String& s, double& out);
    int nb() const { return juce::jlimit (1, spec::MAXBANDS, (int) std::lround (pv (spec::P::nbands))); }
    static int bp (int band, int off) { return spec::bandParam (band, off); }
    void setBandParam (int off, double value);     // su tutte le bande selezionate
    void beginEdit();
    void endEdit();

    // ---- struttura della modulazione
    Struct str;
    std::function<void()> onChange;   // chiamato dopo ogni modifica (ridisegno)
    void structChanged();
    juce::String srcName (const juce::String& key) const;
    juce::String srcLabelForSlot (int si) const;
    juce::String targetName (int t) const;
    static void srcKeyParts (const juce::String& key, juce::String& type, int& k);
    static int srcIndexOf (const juce::String& key, const juce::String& axis = "y");
    static juce::String srcKeyFromIndex (int si, juce::String* axis = nullptr);
    static juce::String srcTypeOfIndex (int si);
    bool srcActive (const juce::String& key) const;
    juce::String addSource (const juce::String& type, bool slider = false);
    void removeSource (const juce::String& key);
    int addSlot (int src, int tgt, double amt = 0.5);
    void deleteSlot (int i, bool quiet = false);
    std::vector<int> slotsForTarget (int t) const;
    juce::StringArray targetSourceTypes (int t) const;
    static int bandOfTarget (int t, int* off = nullptr);

    // ---- bande
    std::vector<int> sel { 0 };
    void addBandAt (double freq);
    void removeBands (std::vector<int> list);
    int bandAtFreq (double f) const;
    void bandEdges (int b, double& lo, double& hi) const;
    void moveXover (int i, double f);

    // ---- undo, A/B
    struct Snapshot { std::vector<double> params; juce::var str; };
    Snapshot snapshot() const;
    void applySnapshot (const Snapshot& s);
    void pushUndo();
    void doUndo();
    void doRedo();
    bool canUndo() const { return ! undo.empty(); }
    bool canRedo() const { return ! redo.empty(); }
    int abCur = 0;
    bool abCopied = false;
    void abSwitch();
    void abCopy();
    bool abDirty = false;

    // ---- preset
    bool presetDirty = false;
    juce::var presetData() const;
    void loadPresetData (const juce::var& d, const juce::String& name);
    juce::File presetFolder() const;
    void setPresetFolder (const juce::File& f);
    void ensureFactoryPresets (bool force = false);
    struct PresetEntry { juce::File file; juce::String name, cat; };
    std::vector<PresetEntry> allPresets() const;     // ordine del menu (e numerazione Program Change)
    bool loadPresetFile (const juce::File& f);
    bool savePresetFile (const juce::File& f);
    void stepPreset (int d);
    void loadDefaultPreset();
    void saveAsDefault();
    static juce::String presetExt() { return ".cep"; }

    // ---- preset di sezione: tipi "band", "xlfo", "eg", "ef", "midi"
    struct SectionPreset { juce::String name; juce::var d; bool user = false; };
    std::vector<SectionPreset> sectionPresets (const juce::String& type) const;
    juce::var captureSection (const juce::String& type, int k) const;
    void applySectionPreset (const juce::String& type, int k, const juce::var& d, bool quiet = false);
    void saveSectionPreset (const juce::String& type, int k, const juce::String& name);
    void saveSectionDefault (const juce::String& type, int k);

    // ---- MIDI
    bool learnMode = false;
    int learnArm = -1;
    std::map<int, int> learn;          // CC -> parametro
    std::map<int, int> learnSaved;
    bool midiEnable = true, pgmChanges = false;
    void setLearn (int cc, int param);
    void clearLearn (int cc);
    void saveLearn();
    void revertLearn();
    void programChange (int bank, int prog);
    void pushLearnToProcessor();

    // ---- impostazioni dell'interfaccia (salvate con l'istanza)
    bool showMod = true, autoCollapse = true, showFlow = true;
    juce::String expanded;
    juce::StringArray expList;
    // preferenze globali
    int sizeIdx = 1;
    double scaleMul = 1.0;
    void savePrefs();
    void loadPrefs();

    juce::var instanceState() const;
    void setInstanceState (const juce::var& v);
    void writeCfg();
    static juce::File appDataFolder();

    SaturaProcessor& proc;

private:
    std::vector<Snapshot> undo, redo;
    std::unique_ptr<Snapshot> pending;
    juce::String pendingStr;
    std::array<Snapshot, 2> ab;
    std::array<bool, 2> abHas { false, false };
    std::vector<int> gestures;
    juce::var sectionFactory, sectionUser, sectionDefaults;
    void loadSectionFiles();
    void saveSectionFiles();
    void remapBandTargets (std::function<int(int)> fn);
    void remapXoverTargets (std::function<int(int)> fn);
    void copyBand (int from, int to);
};
