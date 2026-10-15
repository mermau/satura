#include "Model.h"
#include "PluginProcessor.h"
#include "BinaryData.h"

using namespace spec;
using juce::String;
using juce::var;

// =================================================================== utilità
namespace
{
const ParamInfo& PI (int i) { return params()[(size_t) i]; }
double clampd (double x, double a, double b) { return x < a ? a : (x > b ? b : x); }
int typeIndex (const String& t) { return t == "xlfo" ? 0 : t == "eg" ? 1 : t == "ef" ? 2 : t == "midi" ? 3 : 4; }
const char* const NOTE_NAMES[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
var obj() { return var (new juce::DynamicObject()); }
void put (var& o, const char* k, const var& v) { o.getDynamicObject()->setProperty (k, v); }
double num (const var& o, const char* k, double def) { return o.hasProperty (k) ? (double) o[k] : def; }
}

// =================================================================== Struct <-> JSON (stesso formato della versione Max)
var Struct::toVar() const
{
    var o = obj();
    juce::Array<var> a;
    for (auto& x : xlfo)
    {
        var e = obj(); put (e, "active", x.active); put (e, "sync", x.sync); put (e, "retrig", x.retrig); put (e, "snap", x.snap);
        juce::Array<var> st;
        for (auto& s : x.steps) { var q = obj(); put (q, "v", s.v); put (q, "r", s.r); put (q, "c", s.c); put (q, "g", s.g); st.add (q); }
        put (e, "steps", st);
        a.add (e);
    }
    put (o, "xlfo", a); a.clear();
    for (auto& x : eg) { var e = obj(); put (e, "active", x.active); put (e, "trig", x.trig); put (e, "range", x.range); put (e, "as", x.as); put (e, "ds", x.ds); put (e, "rs", x.rs); a.add (e); }
    put (o, "eg", a); a.clear();
    for (auto& x : ef) { var e = obj(); put (e, "active", x.active); put (e, "trig", x.trig); put (e, "mode", x.mode); a.add (e); }
    put (o, "ef", a); a.clear();
    for (auto& x : midi) { var e = obj(); put (e, "active", x.active); put (e, "input", x.input); put (e, "cc", x.cc); put (e, "curve", x.curve); a.add (e); }
    put (o, "midi", a); a.clear();
    for (auto& x : xy) { var e = obj(); put (e, "active", x.active); put (e, "mode", x.mode); put (e, "range", x.range); a.add (e); }
    put (o, "xy", a); a.clear();
    for (auto& s : slots) { var e = obj(); put (e, "on", s.on); put (e, "src", s.src); put (e, "tgt", s.tgt); put (e, "amt", s.amt); put (e, "inv", s.inv); a.add (e); }
    put (o, "slots", a); a.clear();
    for (auto& k : order) a.add (k);
    put (o, "order", a);
    var nm = obj();
    for (auto& kv : names) put (nm, kv.first.toRawUTF8(), kv.second);
    put (o, "names", nm);
    put (o, "presetName", presetName);
    return o;
}

Struct Struct::fromVar (const var& v)
{
    Struct s;
    if (! v.isObject()) return s;
    if (auto* a = v["xlfo"].getArray())
        for (int k = 0; k < juce::jmin ((int) s.xlfo.size(), a->size()); ++k)
        {
            auto& x = s.xlfo[(size_t) k]; const var& e = (*a)[k];
            x.active = (int) num (e, "active", 0); x.sync = (int) num (e, "sync", 0); x.retrig = (int) num (e, "retrig", 0); x.snap = (int) num (e, "snap", 0);
            if (auto* st = e["steps"].getArray())
            {
                x.steps.clear();
                for (auto& q : *st) x.steps.push_back ({ num (q, "v", 0), (int) num (q, "r", 0), (int) num (q, "c", 3), num (q, "g", 1) });
                if (x.steps.empty()) x.steps.push_back ({});
                if (x.steps.size() > (size_t) NSTEPS) x.steps.resize ((size_t) NSTEPS);
            }
        }
    if (auto* a = v["eg"].getArray())
        for (int k = 0; k < juce::jmin ((int) s.eg.size(), a->size()); ++k)
        { auto& x = s.eg[(size_t) k]; const var& e = (*a)[k]; x.active = (int) num (e, "active", 0); x.trig = (int) num (e, "trig", 0); x.range = (int) num (e, "range", 0); x.as = num (e, "as", 0.3); x.ds = num (e, "ds", 0.5); x.rs = num (e, "rs", 0.5); }
    if (auto* a = v["ef"].getArray())
        for (int k = 0; k < juce::jmin ((int) s.ef.size(), a->size()); ++k)
        { auto& x = s.ef[(size_t) k]; const var& e = (*a)[k]; x.active = (int) num (e, "active", 0); x.trig = (int) num (e, "trig", 0); x.mode = (int) num (e, "mode", 0); }
    if (auto* a = v["midi"].getArray())
        for (int k = 0; k < juce::jmin ((int) s.midi.size(), a->size()); ++k)
        { auto& x = s.midi[(size_t) k]; const var& e = (*a)[k]; x.active = (int) num (e, "active", 0); x.input = (int) num (e, "input", 0); x.cc = (int) num (e, "cc", 1); x.curve = (int) num (e, "curve", 0); }
    if (auto* a = v["xy"].getArray())
        for (int k = 0; k < juce::jmin ((int) s.xy.size(), a->size()); ++k)
        { auto& x = s.xy[(size_t) k]; const var& e = (*a)[k]; x.active = (int) num (e, "active", 0); x.mode = (int) num (e, "mode", 0); x.range = (int) num (e, "range", 0); }
    if (auto* a = v["slots"].getArray())
        for (auto& e : *a)
        {
            Slot sl { (int) num (e, "on", 1), (int) num (e, "src", 0), (int) num (e, "tgt", 0), num (e, "amt", 0.5), (int) num (e, "inv", 0) };
            if (sl.src > 0 && sl.src < NSRC && sl.tgt >= 0 && sl.tgt < NT && (int) s.slots.size() < NSLOTS) s.slots.push_back (sl);
        }
    if (auto* a = v["order"].getArray()) for (auto& k : *a) s.order.add (k.toString());
    if (auto* o = v["names"].getDynamicObject()) for (auto& p : o->getProperties()) s.names[p.name.toString()] = p.value.toString();
    if (v.hasProperty ("presetName")) s.presetName = v["presetName"].toString();
    return s;
}

// =================================================================== Model
Model::Model (SaturaProcessor& p) : proc (p)
{
    sectionFactory = juce::JSON::parse (String::fromUTF8 (BinaryData::section_presets_json, BinaryData::section_presets_jsonSize));
    loadSectionFiles();
}

juce::File Model::appDataFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
#if JUCE_MAC
        .getChildFile ("Application Support")
#endif
        .getChildFile ("Collateral Effects").getChildFile ("SATURA");
}

double Model::pv (int i) const { return proc.rawParam (i); }

double Model::norm (int i, double v)
{
    const auto& p = PI (i);
    double n;
    if (p.map == Map::exp) n = std::log (std::max (v, (double) p.min) / p.min) / std::log ((double) p.max / p.min);
    else if (p.map == Map::pow3) n = std::cbrt (std::max ((v - p.min) / (p.max - p.min), 0.0));
    else n = (v - p.min) / (p.max - p.min);
    return clampd (n, 0, 1);
}

double Model::denorm (int i, double n)
{
    const auto& p = PI (i);
    n = clampd (n, 0, 1);
    double v;
    if (p.map == Map::exp) v = p.min * std::pow ((double) p.max / p.min, n);
    else if (p.map == Map::pow3) v = p.min + (p.max - p.min) * n * n * n;
    else v = p.min + (p.max - p.min) * n;
    if (p.map == Map::choice || p.map == Map::integer) v = std::round (v);
    return v;
}

static String fmtHz (double f)
{
    if (f >= 10000) return String (f / 1000, 1) + " kHz";
    if (f >= 1000) return String (f / 1000, 2) + " kHz";
    if (f >= 100) return String (juce::roundToInt (f)) + " Hz";
    return String (f, 1) + " Hz";
}
static String fmtMs (double t)
{
    if (t >= 1000) return String (t / 1000, 2) + " s";
    if (t >= 100) return String (juce::roundToInt (t)) + " ms";
    return String (t, 1) + " ms";
}

String Model::fmtVal (int i, double v)
{
    const auto& p = PI (i);
    if (p.map == Map::choice)
    {
        auto ch = juce::StringArray::fromTokens (p.choices, "|", "");
        return ch[juce::jlimit (0, ch.size() - 1, (int) std::lround (v - p.min))];
    }
    if (p.map == Map::integer) return String ((int) std::lround (v));
    switch (p.unit)
    {
        case 4:
            if (p.map == Map::db && v <= -59.9) return "-inf dB";
            return (v > 0 ? "+" : "") + String (v, 1) + " dB";
        case 3: return fmtHz (v);
        case 2: return fmtMs (v);
        case 5: return String (juce::roundToInt (v)) + "%";
        case 6:
            if (std::abs (v) < 0.5) return "C";
            return String (juce::roundToInt (std::abs (v))) + (v < 0 ? " L" : " R");
        default: return String (v, 2);
    }
}

// interpreta '1k', 'A4', 'C#3+13', '2x', '50%', '250ms', '1.5s', '30L', numeri
bool Model::parseEntry (int i, const String& in, double& out)
{
    const auto& p = PI (i);
    String s = in.removeCharacters (" \t").replaceCharacter (',', '.').toLowerCase();
    for (auto u : { "khz", "hz", "db", "ms" })
        if (s.endsWith (u) && ! (String (u) == "hz" && s.endsWith ("khz"))) { if (String (u) != "ms") s = s.dropLastCharacters ((int) strlen (u)); break; }
    if (s.isEmpty()) return false;
    if (s.endsWithChar ('%')) { out = denorm (i, s.dropLastCharacters (1).getDoubleValue() / 100.0); return true; }
    if (p.map == Map::choice)
    {
        auto ch = juce::StringArray::fromTokens (p.choices, "|", "");
        for (int k = 0; k < ch.size(); ++k) if (ch[k].toLowerCase() == s) { out = p.min + k; return true; }
    }
    if (p.unit == 3)
    {
        const auto c0 = s[0];
        if (c0 >= 'a' && c0 <= 'g' && s.length() >= 2)
        {
            int idx = 1;
            int nn = juce::StringArray (NOTE_NAMES, 12).indexOf (String::charToString (c0).toUpperCase());
            if (s[idx] == '#') { ++nn; ++idx; }
            else if (s[idx] == 'b') { --nn; ++idx; }
            String rest = s.substring (idx);
            int sign = rest.indexOfAnyOf ("+-", 1);
            const int oct = (sign > 0 ? rest.substring (0, sign) : rest).getIntValue();
            const double cents = sign > 0 ? rest.substring (sign).getDoubleValue() : 0;
            const double midi = nn + (oct + 1) * 12 + cents / 100.0;
            out = clampd (440.0 * std::pow (2.0, (midi - 69) / 12.0), p.min, p.max);
            return true;
        }
        if (s.endsWithChar ('k')) { out = clampd (s.dropLastCharacters (1).getDoubleValue() * 1000, p.min, p.max); return true; }
    }
    if (p.unit == 4 && s.endsWithChar ('x')) { out = clampd (20 * std::log10 (std::max (1e-9, s.dropLastCharacters (1).getDoubleValue())), p.min, p.max); return true; }
    if (p.unit == 2)
    {
        if (s.endsWith ("ms")) { out = clampd (s.dropLastCharacters (2).getDoubleValue(), p.min, p.max); return true; }
        if (s.endsWithChar ('s')) { out = clampd (s.dropLastCharacters (1).getDoubleValue() * 1000, p.min, p.max); return true; }
    }
    if (p.unit == 6)
    {
        if (s == "c") { out = 0; return true; }
        if (s.endsWithChar ('l') || s.endsWithChar ('r')) { out = clampd (s.dropLastCharacters (1).getDoubleValue() * (s.endsWithChar ('l') ? -1 : 1), p.min, p.max); return true; }
    }
    if (s == "-inf") { out = p.min; return true; }
    if (! s.containsAnyOf ("0123456789")) return false;
    out = clampd (s.getDoubleValue(), p.min, p.max);
    return true;
}

void Model::setParam (int i, double v)
{
    const auto& p = PI (i);
    v = clampd (v, p.min, p.max);
    if (p.map == Map::choice || p.map == Map::integer) v = std::round (v);
    auto* prm = proc.prm[(size_t) i];
    const float n = prm->convertTo0to1 ((float) v);
    if (std::abs (prm->getValue() - n) < 1e-7f) return;
    const bool inGesture = pending != nullptr;
    if (inGesture)
    {
        if (std::find (gestures.begin(), gestures.end(), i) == gestures.end()) { gestures.push_back (i); prm->beginChangeGesture(); }
        prm->setValueNotifyingHost (n);
    }
    else
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (n);
        prm->endChangeGesture();
    }
    if (i != P::bypass) { abDirty = true; presetDirty = true; }
    if (onChange) onChange();
}

void Model::setBandParam (int off, double value)
{
    if (sel.empty()) sel = { 0 };
    const int prim = sel[0];
    const int pn = bp (prim, off);
    const auto& p = PI (pn);
    const bool cont = ! (p.map == Map::choice || p.map == Map::integer);
    const double dn = norm (pn, value) - norm (pn, pv (pn));
    for (int b : sel)
    {
        const int name = bp (b, off);
        if (b == prim || ! cont) setParam (name, value);
        else setParamNorm (name, getNorm (name) + dn);
    }
}

void Model::beginEdit()
{
    pending = std::make_unique<Snapshot> (snapshot());
    pendingStr = juce::JSON::toString (pending->str, true);
}

void Model::endEdit()
{
    for (int i : gestures) proc.prm[(size_t) i]->endChangeGesture();
    gestures.clear();
    if (! pending) return;
    const auto now = snapshot();
    if (now.params != pending->params || juce::JSON::toString (now.str, true) != pendingStr)
    {
        undo.push_back (*pending);
        if (undo.size() > 100) undo.erase (undo.begin());
        redo.clear();
    }
    pending.reset();
}

// ---------------------------------------------------------------- sorgenti
void Model::srcKeyParts (const String& key, String& type, int& k)
{
    type = key.dropLastCharacters (1);
    k = key.getLastCharacters (1).getIntValue() - 1;
}
int Model::srcIndexOf (const String& key, const String& axis)
{
    String t; int k; srcKeyParts (key, t, k);
    const String n = t == "xy" ? "xy" + String (k + 1) + "_" + axis : key;
    for (int i = 0; i < NSRC; ++i) if (n == sourceIds[i]) return i;
    return 0;
}
String Model::srcKeyFromIndex (int si, String* axis)
{
    String n = sourceIds[juce::jlimit (0, NSRC - 1, si)];
    if (n.startsWith ("xy")) { if (axis) *axis = n.getLastCharacters (1); return n.substring (0, 3); }
    if (axis) *axis = {};
    return n;
}
String Model::srcTypeOfIndex (int si) { String t; int k; srcKeyParts (srcKeyFromIndex (si), t, k); return t; }

bool Model::srcActive (const String& key) const
{
    String t; int k; srcKeyParts (key, t, k);
    if (k < 0) return false;
    if (t == "xlfo") return str.xlfo[(size_t) k].active;
    if (t == "eg") return str.eg[(size_t) k].active;
    if (t == "ef") return str.ef[(size_t) k].active;
    if (t == "midi") return str.midi[(size_t) k].active;
    return str.xy[(size_t) k].active;
}

String Model::srcName (const String& key) const
{
    auto it = str.names.find (key);
    if (it != str.names.end() && it->second.isNotEmpty()) return it->second;
    String t; int k; srcKeyParts (key, t, k);
    if (t == "xy") return (str.xy[(size_t) k].mode == 1 ? "Slider " : "XY Controller ") + String (k + 1);
    const char* lab[] = { "XLFO", "EG", "EF", "MIDI" };
    return String (lab[typeIndex (t)]) + " " + String (k + 1);
}

String Model::srcLabelForSlot (int si) const
{
    String axis;
    const String key = srcKeyFromIndex (si, &axis);
    if (axis.isEmpty()) return srcName (key);
    String t; int k; srcKeyParts (key, t, k);
    if (str.xy[(size_t) k].mode == 1) return srcName (key);
    return srcName (key) + " " + axis.toUpperCase();
}

String Model::addSource (const String& type, bool slider)
{
    auto doAdd = [&] (auto& arr) -> int
    {
        for (int k = 0; k < (int) arr.size(); ++k)
            if (! arr[(size_t) k].active)
            {
                arr[(size_t) k] = {};
                arr[(size_t) k].active = 1;
                return k;
            }
        return -1;
    };
    int k = -1;
    if (type == "xlfo") k = doAdd (str.xlfo);
    else if (type == "eg") k = doAdd (str.eg);
    else if (type == "ef") k = doAdd (str.ef);
    else if (type == "midi") k = doAdd (str.midi);
    else { k = doAdd (str.xy); if (k >= 0 && slider) str.xy[(size_t) k].mode = 1; }
    if (k < 0) return {};
    const String key = type + String (k + 1);
    str.names.erase (key);
    if (! str.order.contains (key)) str.order.add (key);
    if (type != "xy")
    {
        // preset di sezione "Default" (eventualmente sovrascritto con Save As Default)
        var d = sectionDefaults[type.toRawUTF8()];
        if (d.isVoid())
            if (auto* a = sectionFactory[type.toRawUTF8()].getArray(); a != nullptr && ! a->isEmpty()) d = (*a)[0]["d"];
        if (! d.isVoid()) applySectionPreset (type, k, d, true);
    }
    else
    {
        setParam (P::xy1_x + k * 2, 0);
        setParam (P::xy1_y + k * 2, 0);
    }
    structChanged();
    return key;
}

void Model::removeSource (const String& key)
{
    String t; int k; srcKeyParts (key, t, k);
    if (t == "xlfo") str.xlfo[(size_t) k].active = 0;
    else if (t == "eg") str.eg[(size_t) k].active = 0;
    else if (t == "ef") str.ef[(size_t) k].active = 0;
    else if (t == "midi") str.midi[(size_t) k].active = 0;
    else str.xy[(size_t) k].active = 0;
    std::vector<int> idx;
    if (t == "xy") { idx.push_back (srcIndexOf (key, "x")); idx.push_back (srcIndexOf (key, "y")); }
    else idx.push_back (srcIndexOf (key));
    for (int i = (int) str.slots.size() - 1; i >= 0; --i)
        if (std::find (idx.begin(), idx.end(), str.slots[(size_t) i].src) != idx.end()) deleteSlot (i, true);
    str.order.removeString (key);
    expList.removeString (key);
    if (expanded == key) expanded = {};
    str.names.erase (key);
    structChanged();
}

int Model::addSlot (int src, int tgt, double amt)
{
    for (int i = 0; i < (int) str.slots.size(); ++i)
        if (str.slots[(size_t) i].src == src && str.slots[(size_t) i].tgt == tgt) return i;
    if ((int) str.slots.size() >= NSLOTS) return -1;
    str.slots.push_back ({ 1, src, tgt, amt, 0 });
    structChanged();
    return (int) str.slots.size() - 1;
}

void Model::deleteSlot (int i, bool quiet)
{
    std::vector<bool> del (str.slots.size(), false);
    if (i < 0 || i >= (int) del.size()) return;
    del[(size_t) i] = true;
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (size_t j = 0; j < str.slots.size(); ++j)
        {
            const int t = str.slots[j].tgt - SLOT0;
            if (! del[j] && t >= 0 && t < (int) del.size() && del[(size_t) t]) { del[j] = true; changed = true; }
        }
    }
    std::vector<int> map (str.slots.size(), -1);
    std::vector<Slot> out;
    for (size_t j = 0; j < str.slots.size(); ++j) if (! del[j]) { map[j] = (int) out.size(); out.push_back (str.slots[j]); }
    for (auto& s : out) if (s.tgt >= SLOT0) s.tgt = SLOT0 + map[(size_t) (s.tgt - SLOT0)];
    str.slots = out;
    if (! quiet) structChanged();
}

std::vector<int> Model::slotsForTarget (int t) const
{
    std::vector<int> r;
    for (int i = 0; i < (int) str.slots.size(); ++i) if (str.slots[(size_t) i].tgt == t) r.push_back (i);
    return r;
}

juce::StringArray Model::targetSourceTypes (int t) const
{
    juce::StringArray r;
    if (t < 0) return r;
    for (auto& s : str.slots) if (s.tgt == t && s.on) r.addIfNotAlreadyThere (srcTypeOfIndex (s.src));
    return r;
}

int Model::bandOfTarget (int t, int* off)
{
    if (t < 0 || t >= SLOT0) return -1;
    String n = targetIds[t];
    if (n.length() > 3 && n[0] == 'b' && n[2] == '_' && juce::CharacterFunctions::isDigit (n[1]))
    {
        const int b = n[1] - '1';
        if (off)
        {
            const String sfx = n.substring (3);
            for (int j = 0; j < NBP; ++j) if (sfx == String (params()[(size_t) bandParam (0, j)].id).substring (3)) *off = j;
        }
        return b;
    }
    return -1;
}

String Model::targetName (int t) const
{
    if (t < 0 || t >= NT) return {};
    if (t >= SLOT0)
    {
        const int i = t - SLOT0;
        if (i >= (int) str.slots.size()) return "Slot";
        return "Slot " + srcLabelForSlot (str.slots[(size_t) i].src) + " > " + targetName (str.slots[(size_t) i].tgt);
    }
    for (int i = 0; i < NP; ++i)
        if (PI (i).target == t)
        {
            String g = PI (i).group;
            if (g.startsWith ("xlfo") || g.startsWith ("eg") || g.startsWith ("ef")) return srcName (g) + " " + PI (i).ui;
            return PI (i).label;
        }
    return {};
}

// ---------------------------------------------------------------- configurazione verso il motore
void Model::writeCfg()
{
    std::array<float, CFG::SIZE> a {};
    for (int i = 0; i < (int) str.slots.size() && i < NSLOTS; ++i)
    {
        const auto& s = str.slots[(size_t) i];
        const int b = CFG::SLOT_BASE + i * CFG::SLOT_STRIDE;
        a[(size_t) b] = (float) s.on; a[(size_t) b + 1] = (float) s.src; a[(size_t) b + 2] = (float) s.tgt; a[(size_t) b + 3] = (float) s.amt; a[(size_t) b + 4] = (float) s.inv;
    }
    for (int k = 0; k < NXLFO; ++k)
    {
        const auto& x = str.xlfo[(size_t) k];
        const int b = CFG::XLFO_BASE + k * CFG::XLFO_STRIDE;
        a[(size_t) b] = (float) x.active; a[(size_t) b + 1] = (float) x.steps.size(); a[(size_t) b + 2] = (float) x.sync; a[(size_t) b + 3] = (float) x.retrig; a[(size_t) b + 4] = (float) x.snap;
        for (int i = 0; i < (int) x.steps.size() && i < NSTEPS; ++i)
        {
            const auto& s = x.steps[(size_t) i];
            a[(size_t) (b + 8 + i * 4)] = (float) s.v; a[(size_t) (b + 9 + i * 4)] = (float) s.r; a[(size_t) (b + 10 + i * 4)] = (float) s.c; a[(size_t) (b + 11 + i * 4)] = (float) s.g;
        }
    }
    for (int k = 0; k < NEG; ++k)
    {
        const auto& e = str.eg[(size_t) k]; const int b = CFG::EG_BASE + k * CFG::EG_STRIDE;
        a[(size_t) b] = (float) e.active; a[(size_t) b + 1] = (float) e.trig; a[(size_t) b + 2] = (float) e.range; a[(size_t) b + 3] = (float) e.as; a[(size_t) b + 4] = (float) e.ds; a[(size_t) b + 5] = (float) e.rs;
    }
    for (int k = 0; k < NEF; ++k)
    { const auto& e = str.ef[(size_t) k]; const int b = CFG::EF_BASE + k * CFG::EF_STRIDE; a[(size_t) b] = (float) e.active; a[(size_t) b + 1] = (float) e.trig; a[(size_t) b + 2] = (float) e.mode; }
    for (int k = 0; k < NXY; ++k)
    { const auto& e = str.xy[(size_t) k]; const int b = CFG::XY_BASE + k * CFG::XY_STRIDE; a[(size_t) b] = (float) e.active; a[(size_t) b + 1] = (float) e.mode; a[(size_t) b + 2] = (float) e.range; }
    for (int k = 0; k < NMIDI; ++k)
    { const auto& e = str.midi[(size_t) k]; const int b = CFG::MIDI_BASE + k * CFG::MIDI_STRIDE; a[(size_t) b] = (float) e.active; a[(size_t) b + 1] = (float) e.input; a[(size_t) b + 2] = (float) e.cc; a[(size_t) b + 3] = (float) e.curve; }
    proc.pushConfig (a);
}

void Model::structChanged()
{
    writeCfg();
    abDirty = true;
    presetDirty = true;
    if (onChange) onChange();
}

// ---------------------------------------------------------------- bande
void Model::bandEdges (int b, double& lo, double& hi) const
{
    const int n = nb();
    lo = b == 0 ? 10.0 : pv (P::xf1 + b - 1);
    hi = b == n - 1 ? 30000.0 : pv (P::xf1 + b);
}

int Model::bandAtFreq (double f) const
{
    const int n = nb();
    for (int b = 0; b < n - 1; ++b) if (f < pv (P::xf1 + b)) return b;
    return n - 1;
}

void Model::moveXover (int i, double f)
{
    const int n = nb();
    const double lo = i > 0 ? pv (P::xf1 + i - 1) * 1.06 : 10.0;
    const double hi = i < n - 2 ? pv (P::xf1 + i + 1) / 1.06 : 30000.0;
    setParam (P::xf1 + i, clampd (f, lo, hi));
}

void Model::copyBand (int from, int to)
{
    for (int j = 0; j < NBP; ++j) setParam (bp (to, j), pv (bp (from, j)));
}

void Model::remapBandTargets (std::function<int(int)> fn)
{
    for (int i = (int) str.slots.size() - 1; i >= 0; --i)
    {
        int off = 0;
        const int b = bandOfTarget (str.slots[(size_t) i].tgt, &off);
        if (b >= 0)
        {
            const int nbd = fn (b);
            if (nbd < 0) deleteSlot (i, true);
            else str.slots[(size_t) i].tgt = PI (bp (nbd, off)).target;
        }
    }
}

void Model::remapXoverTargets (std::function<int(int)> fn)
{
    for (int i = (int) str.slots.size() - 1; i >= 0; --i)
    {
        const int t = str.slots[(size_t) i].tgt;
        for (int x = 0; x < MAXBANDS - 1; ++x)
            if (PI (P::xf1 + x).target == t)
            {
                const int nx = fn (x);
                if (nx < 0) deleteSlot (i, true);
                else str.slots[(size_t) i].tgt = PI (P::xf1 + nx).target;
                break;
            }
    }
}

void Model::addBandAt (double freq)
{
    const int n = nb();
    if (n >= MAXBANDS) return;
    pushUndo();
    const int b = bandAtFreq (freq);
    for (int i = n - 1; i > b; --i) copyBand (i, i + 1);
    copyBand (b, b + 1);
    setParam (bp (b + 1, BP::solo), 0);
    for (int i = n - 1; i >= b + 1; --i) { setParam (P::xf1 + i, pv (P::xf1 + i - 1)); setParam (P::xs1 + i, pv (P::xs1 + i - 1)); }
    const double prevSlope = b > 0 ? pv (P::xs1 + b - 1) : (b + 1 < n ? pv (P::xs1 + b + 1) : 2.0);
    setParam (P::xf1 + b, freq);
    setParam (P::xs1 + b, prevSlope);
    remapBandTargets ([b] (int bd) { return bd > b ? bd + 1 : bd; });
    remapXoverTargets ([b] (int x) { return x >= b ? x + 1 : x; });
    setParam (P::nbands, n + 1);
    sel = { b + 1 };
    structChanged();
}

void Model::removeBands (std::vector<int> list)
{
    std::sort (list.begin(), list.end(), std::greater<int>());
    list.erase (std::unique (list.begin(), list.end()), list.end());
    if ((int) list.size() >= nb()) list.resize ((size_t) (nb() - 1));
    if (list.empty()) return;
    pushUndo();
    for (int b : list)
    {
        const int cn = nb();
        const int xr = b < cn - 1 ? b : b - 1;
        for (int i = b; i < cn - 1; ++i) copyBand (i + 1, i);
        for (int i = xr; i < cn - 2; ++i) { setParam (P::xf1 + i, pv (P::xf1 + i + 1)); setParam (P::xs1 + i, pv (P::xs1 + i + 1)); }
        remapBandTargets ([b] (int bd) { return bd == b ? -1 : (bd > b ? bd - 1 : bd); });
        remapXoverTargets ([xr] (int x) { return x == xr ? -1 : (x > xr ? x - 1 : x); });
        setParam (P::nbands, cn - 1);
    }
    sel = { std::min (list.back(), nb() - 1) };
    structChanged();
}

// ---------------------------------------------------------------- undo, A/B
Model::Snapshot Model::snapshot() const
{
    Snapshot s;
    s.params.resize ((size_t) NP);
    for (int i = 0; i < NP; ++i) s.params[(size_t) i] = i == P::bypass ? 0.0 : pv (i);
    s.str = str.toVar();
    return s;
}

void Model::applySnapshot (const Snapshot& s)
{
    const String name = str.presetName;
    str = Struct::fromVar (s.str);
    for (int i = 0; i < NP; ++i) if (i != P::bypass) setParam (i, s.params[(size_t) i]);
    for (int i = (int) sel.size() - 1; i >= 0; --i) if (sel[(size_t) i] >= nb()) sel.erase (sel.begin() + i);
    if (sel.empty()) sel = { 0 };
    if (expanded.isNotEmpty() && ! srcActive (expanded)) expanded = {};
    juce::ignoreUnused (name);
    structChanged();
}

void Model::pushUndo()
{
    undo.push_back (snapshot());
    if (undo.size() > 100) undo.erase (undo.begin());
    redo.clear();
}
void Model::doUndo()
{
    if (undo.empty()) return;
    redo.push_back (snapshot());
    auto s = undo.back(); undo.pop_back();
    applySnapshot (s);
}
void Model::doRedo()
{
    if (redo.empty()) return;
    undo.push_back (snapshot());
    auto s = redo.back(); redo.pop_back();
    applySnapshot (s);
}
void Model::abSwitch()
{
    pushUndo();
    ab[(size_t) abCur] = snapshot(); abHas[(size_t) abCur] = true;
    abCur = 1 - abCur;
    if (abHas[(size_t) abCur]) applySnapshot (ab[(size_t) abCur]);
    else { ab[(size_t) abCur] = snapshot(); abHas[(size_t) abCur] = true; }
    abDirty = false;
    abCopied = false;
    if (onChange) onChange();
}
void Model::abCopy()
{
    ab[(size_t) (1 - abCur)] = snapshot();
    abHas[(size_t) (1 - abCur)] = true;
    abCopied = true;
    abDirty = false;
    if (onChange) onChange();
}

// ---------------------------------------------------------------- preset
var Model::presetData() const
{
    var d = obj();
    put (d, "format", "satura-preset");
    put (d, "version", 1);
    var p = obj();
    for (int i = 0; i < NP; ++i)
    {
        if (i == P::bypass) continue;
        const double v = pv (i);
        if (std::abs (v - PI (i).def) > 1e-6) put (p, PI (i).id, v);
    }
    put (d, "params", p);
    put (d, "struct", str.toVar());
    return d;
}

void Model::loadPresetData (const var& d, const String& name)
{
    pushUndo();
    Snapshot s;
    s.params.resize ((size_t) NP);
    for (int i = 0; i < NP; ++i) s.params[(size_t) i] = PI (i).def;
    if (auto* o = d["params"].getDynamicObject())
        for (auto& kv : o->getProperties())
            for (int i = 0; i < NP; ++i)
                if (kv.name.toString() == PI (i).id && i != P::bypass) s.params[(size_t) i] = (double) kv.value;
    s.str = d["struct"].isObject() ? d["struct"] : Struct().toVar();
    applySnapshot (s);
    str.presetName = name;
    sel = { 0 };
    expanded = {};
    expList.clear();
    presetDirty = false;
    abDirty = true;
    if (onChange) onChange();
}

juce::File Model::presetFolder() const
{
    auto cfgFile = appDataFolder().getChildFile ("preset_folder.txt");
    if (cfgFile.existsAsFile())
    {
        juce::File f (cfgFile.loadFileAsString().trim());
        if (f.isDirectory()) return f;
    }
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Collateral Effects").getChildFile ("Presets").getChildFile ("SATURA");
}

void Model::setPresetFolder (const juce::File& f)
{
    appDataFolder().createDirectory();
    appDataFolder().getChildFile ("preset_folder.txt").replaceWithText (f.getFullPathName());
}

void Model::ensureFactoryPresets (bool force)
{
    auto dir = presetFolder();
    if (dir.isDirectory() && ! force) return;
    dir.createDirectory();
    auto list = juce::JSON::parse (String::fromUTF8 (BinaryData::factory_presets_json, BinaryData::factory_presets_jsonSize));
    if (auto* a = list.getArray())
        for (auto& p : *a)
        {
            const String cat = p["cat"].toString(), name = p["name"].toString();
            auto sub = cat == "Default" ? dir : dir.getChildFile (cat);
            sub.createDirectory();
            sub.getChildFile (name + presetExt()).replaceWithText (juce::JSON::toString (p["data"], false));
        }
}

std::vector<Model::PresetEntry> Model::allPresets() const
{
    std::vector<PresetEntry> out, progs, rest, top;
    auto dir = presetFolder();
    auto isPreset = [] (const juce::File& f) { return f.hasFileExtension (presetExt()) || f.hasFileExtension (".json"); };
    juce::Array<juce::File> subs = dir.findChildFiles (juce::File::findDirectories, false);
    subs.sort();
    for (auto& s : subs)
    {
        auto files = s.findChildFiles (juce::File::findFiles, true);
        files.sort();
        for (auto& f : files)
            if (isPreset (f))
            {
                PresetEntry e { f, f.getFileNameWithoutExtension(), f.getParentDirectory().getRelativePathFrom (dir) };
                (s.getFileName().startsWith ("__") ? progs : rest).push_back (e);
            }
    }
    auto files = dir.findChildFiles (juce::File::findFiles, false);
    files.sort();
    for (auto& f : files) if (isPreset (f)) top.push_back ({ f, f.getFileNameWithoutExtension(), {} });
    out.insert (out.end(), progs.begin(), progs.end());
    out.insert (out.end(), top.begin(), top.end());
    out.insert (out.end(), rest.begin(), rest.end());
    return out;
}

bool Model::loadPresetFile (const juce::File& f)
{
    auto d = juce::JSON::parse (f.loadFileAsString());
    if (! d.isObject() || ! (d.hasProperty ("params") || d.hasProperty ("struct"))) return false;
    loadPresetData (d, f.getFileNameWithoutExtension());
    return true;
}

bool Model::savePresetFile (const juce::File& f0)
{
    auto f = f0.hasFileExtension (presetExt()) ? f0 : f0.withFileExtension (presetExt());
    f.getParentDirectory().createDirectory();
    str.presetName = f.getFileNameWithoutExtension();
    if (! f.replaceWithText (juce::JSON::toString (presetData(), false))) return false;
    presetDirty = false;
    if (onChange) onChange();
    return true;
}

void Model::stepPreset (int d)
{
    auto l = allPresets();
    if (l.empty()) return;
    int cur = -1;
    for (int i = 0; i < (int) l.size(); ++i) if (l[(size_t) i].name == str.presetName) cur = i;
    cur = ((cur + d) % (int) l.size() + (int) l.size()) % (int) l.size();
    loadPresetFile (l[(size_t) cur].file);
}

void Model::loadDefaultPreset()
{
    auto f = presetFolder().getChildFile ("Default Setting" + presetExt());
    if (! (f.existsAsFile() && loadPresetFile (f)))
    {
        auto list = juce::JSON::parse (String::fromUTF8 (BinaryData::factory_presets_json, BinaryData::factory_presets_jsonSize));
        if (auto* a = list.getArray()) loadPresetData ((*a)[0]["data"], "Default Setting");
    }
    undo.clear(); redo.clear();
    abDirty = false;
}

void Model::saveAsDefault()
{
    const String name = str.presetName;
    savePresetFile (presetFolder().getChildFile ("Default Setting" + presetExt()));
    str.presetName = name;
}

void Model::programChange (int bank, int prog)
{
    auto l = allPresets();
    const int idx = bank * 128 + prog;
    if (idx >= 0 && idx < (int) l.size()) loadPresetFile (l[(size_t) idx].file);
}

// ---------------------------------------------------------------- preset di sezione
void Model::loadSectionFiles()
{
    auto f = appDataFolder().getChildFile ("Component Presets").getChildFile ("section_presets.json");
    auto v = juce::JSON::parse (f.loadFileAsString());
    sectionUser = v["user"].isObject() ? v["user"] : obj();
    sectionDefaults = v["defaults"].isObject() ? v["defaults"] : obj();
}
void Model::saveSectionFiles()
{
    auto dir = appDataFolder().getChildFile ("Component Presets");
    dir.createDirectory();
    var v = obj(); put (v, "user", sectionUser); put (v, "defaults", sectionDefaults);
    dir.getChildFile ("section_presets.json").replaceWithText (juce::JSON::toString (v, false));
}

std::vector<Model::SectionPreset> Model::sectionPresets (const String& type) const
{
    std::vector<SectionPreset> r;
    if (auto* a = sectionFactory[type.toRawUTF8()].getArray()) for (auto& p : *a) r.push_back ({ p["name"].toString(), p["d"], false });
    if (auto* a = sectionUser[type.toRawUTF8()].getArray()) for (auto& p : *a) r.push_back ({ p["name"].toString(), p["d"], true });
    return r;
}

var Model::captureSection (const String& type, int k) const
{
    var d = obj();
    if (type == "band")
    {
        for (int j = 0; j < NBP; ++j)
        {
            if (j == BP::en || j == BP::solo || j == BP::mute) continue;
            put (d, String (PI (bp (k, j)).id).substring (3).toRawUTF8(), pv (bp (k, j)));
        }
        return d;
    }
    var full = str.toVar()[type.toRawUTF8()][k];
    if (auto* o = full.getDynamicObject()) for (auto& kv : o->getProperties()) if (kv.name.toString() != "active") put (d, kv.name.toString().toRawUTF8(), kv.value);
    const char* pre = type == "xlfo" ? "x" : type == "eg" ? "e" : type == "ef" ? "f" : nullptr;
    if (pre)
    {
        var p = obj();
        const String prefix = String (pre) + String (k + 1) + "_";
        for (int i = 0; i < NP; ++i) if (String (PI (i).id).startsWith (prefix)) put (p, String (PI (i).id).substring (prefix.length()).toRawUTF8(), pv (i));
        put (d, "p", p);
    }
    return d;
}

void Model::applySectionPreset (const String& type, int k, const var& d, bool quiet)
{
    if (type == "band")
    {
        if (auto* o = d.getDynamicObject())
            for (auto& kv : o->getProperties())
                for (int j = 0; j < NBP; ++j)
                    if (kv.name.toString() == String (PI (bp (k, j)).id).substring (3)) setParam (bp (k, j), (double) kv.value);
        return;
    }
    var full = str.toVar();
    var cur = full[type.toRawUTF8()][k];
    if (auto* o = d.getDynamicObject())
        for (auto& kv : o->getProperties())
            if (kv.name.toString() != "p") cur.getDynamicObject()->setProperty (kv.name, kv.value);
    str = Struct::fromVar (full);
    if (d["p"].isObject())
    {
        const char* pre = type == "xlfo" ? "x" : type == "eg" ? "e" : "f";
        const String prefix = String (pre) + String (k + 1) + "_";
        for (auto& kv : d["p"].getDynamicObject()->getProperties())
            for (int i = 0; i < NP; ++i) if (String (PI (i).id) == prefix + kv.name.toString()) setParam (i, (double) kv.value);
    }
    if (! quiet) structChanged();
}

void Model::saveSectionPreset (const String& type, int k, const String& name)
{
    var list = sectionUser[type.toRawUTF8()];
    juce::Array<var> a;
    if (auto* arr = list.getArray()) for (auto& p : *arr) if (p["name"].toString() != name) a.add (p);
    var e = obj(); put (e, "name", name); put (e, "d", captureSection (type, k));
    a.add (e);
    sectionUser.getDynamicObject()->setProperty (type.toRawUTF8(), a);
    saveSectionFiles();
}

void Model::saveSectionDefault (const String& type, int k)
{
    sectionDefaults.getDynamicObject()->setProperty (type.toRawUTF8(), captureSection (type, k));
    saveSectionFiles();
}

// ---------------------------------------------------------------- MIDI Learn (mappatura globale, come Saturn 2)
void Model::pushLearnToProcessor()
{
    for (auto& c : proc.ccMap) c.store (-1);
    for (auto& kv : learn) if (kv.first >= 0 && kv.first < 128) proc.ccMap[(size_t) kv.first].store (kv.second);
    proc.midiEnabled.store (midiEnable);
}
void Model::setLearn (int cc, int param)
{
    for (auto it = learn.begin(); it != learn.end();) { if (it->second == param) it = learn.erase (it); else ++it; }
    learn[cc] = param;
    pushLearnToProcessor();
    if (onChange) onChange();
}
void Model::clearLearn (int cc)
{
    if (cc < 0) learn.clear(); else learn.erase (cc);
    pushLearnToProcessor();
    if (onChange) onChange();
}
void Model::saveLearn()
{
    learnSaved = learn;
    var v = obj();
    for (auto& kv : learn) put (v, String (kv.first).toRawUTF8(), String (PI (kv.second).id));
    appDataFolder().createDirectory();
    appDataFolder().getChildFile ("midi_learn.json").replaceWithText (juce::JSON::toString (v, false));
}
void Model::revertLearn()
{
    learn.clear();
    auto v = juce::JSON::parse (appDataFolder().getChildFile ("midi_learn.json").loadFileAsString());
    if (auto* o = v.getDynamicObject())
        for (auto& kv : o->getProperties())
            for (int i = 0; i < NP; ++i) if (kv.value.toString() == PI (i).id) learn[kv.name.toString().getIntValue()] = i;
    learnSaved = learn;
    pushLearnToProcessor();
    if (onChange) onChange();
}

// ---------------------------------------------------------------- stato dell'istanza e preferenze
var Model::instanceState() const
{
    var v = obj();
    put (v, "showMod", showMod); put (v, "autoCollapse", autoCollapse); put (v, "showFlow", showFlow);
    put (v, "midiEnable", midiEnable); put (v, "pgm", pgmChanges); put (v, "abCur", abCur);
    juce::Array<var> s; for (int b : sel) s.add (b);
    put (v, "sel", s);
    put (v, "dirty", presetDirty);
    return v;
}
void Model::setInstanceState (const var& v)
{
    showMod = (bool) v.getProperty ("showMod", true);
    autoCollapse = (bool) v.getProperty ("autoCollapse", true);
    showFlow = (bool) v.getProperty ("showFlow", true);
    midiEnable = (bool) v.getProperty ("midiEnable", true);
    pgmChanges = (bool) v.getProperty ("pgm", false);
    presetDirty = (bool) v.getProperty ("dirty", false);
    sel.clear();
    if (auto* a = v["sel"].getArray()) for (auto& b : *a) if ((int) b < nb()) sel.push_back ((int) b);
    if (sel.empty()) sel = { 0 };
    pushLearnToProcessor();
}
void Model::savePrefs()
{
    var v = obj(); put (v, "size", sizeIdx); put (v, "scale", scaleMul);
    appDataFolder().createDirectory();
    appDataFolder().getChildFile ("prefs.json").replaceWithText (juce::JSON::toString (v, false));
}
void Model::loadPrefs()
{
    auto v = juce::JSON::parse (appDataFolder().getChildFile ("prefs.json").loadFileAsString());
    sizeIdx = juce::jlimit (0, 3, (int) v.getProperty ("size", 1));
    scaleMul = juce::jlimit (0.5, 3.0, (double) v.getProperty ("scale", 1.0));
}
