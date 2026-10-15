// Generato da tools/gen_spec_h.py: non modificare a mano.
#pragma once
#include <array>

namespace spec
{
constexpr int MAXBANDS = 6;
constexpr int NXLFO = 4;
constexpr int NEG = 4;
constexpr int NEF = 4;
constexpr int NMIDI = 4;
constexpr int NXY = 6;
constexpr int NSLOTS = 32;
constexpr int NSTEPS = 16;
constexpr int NP = 189;
constexpr int NT = 170;
constexpr int NSRC = 29;
constexpr int SLOT0 = 138;
constexpr int NBP = 16;
constexpr int NSTYLES = 27;
constexpr int DEFAULT_STYLE = 7;

enum class Map { lin, exp, pow3, db, choice, integer };

struct ParamInfo { const char* id; const char* label; const char* ui; float min, max, def; Map map; int unit; bool mod; int target; const char* group; const char* choices; int invisible; };

// unit: 0 nessuna, 1 valore, 2 tempo (ms), 3 Hz, 4 dB, 5 %, 6 pan
inline const std::array<ParamInfo, NP>& params()
{
    static const std::array<ParamInfo, NP> p {{
        { "gain_in", "Input", "Input", -36.0f, 36.0f, 0.0f, Map::lin, 4, true, 0, "global", "", 0 },
        { "pan_in", "Input Pan", "Input Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 1, "global", "", 0 },
        { "gain_out", "Output", "Output", -36.0f, 36.0f, 0.0f, Map::lin, 4, true, 2, "global", "", 0 },
        { "pan_out", "Output Pan", "Output Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 3, "global", "", 0 },
        { "gmix", "Mix", "Mix", 0.0f, 100.0f, 100.0f, Map::lin, 5, true, 4, "global", "", 0 },
        { "hq", "High Quality", "High Quality", 0.0f, 2.0f, 0.0f, Map::choice, 0, false, -1, "global", "Off|Good|Superb", 0 },
        { "linphase", "Linear Phase", "Linear Phase", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "global", "Off|On", 0 },
        { "msmode", "Channel Mode", "Channel Mode", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "global", "Stereo|Mid/Side", 0 },
        { "unrestricted", "Unrestricted FB", "Unrestricted FB", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "global", "Off|On", 0 },
        { "bypass", "Bypass", "Bypass", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "global", "Off|On", 0 },
        { "nbands", "Bands", "Bands", 1.0f, 6.0f, 1.0f, Map::integer, 0, false, -1, "global", "", 1 },
        { "xf1", "Crossover 1", "Crossover 1", 10.0f, 30000.0f, 200.0f, Map::exp, 3, true, 5, "xover", "", 0 },
        { "xf2", "Crossover 2", "Crossover 2", 10.0f, 30000.0f, 2000.0f, Map::exp, 3, true, 6, "xover", "", 0 },
        { "xf3", "Crossover 3", "Crossover 3", 10.0f, 30000.0f, 5000.0f, Map::exp, 3, true, 7, "xover", "", 0 },
        { "xf4", "Crossover 4", "Crossover 4", 10.0f, 30000.0f, 9000.0f, Map::exp, 3, true, 8, "xover", "", 0 },
        { "xf5", "Crossover 5", "Crossover 5", 10.0f, 30000.0f, 14000.0f, Map::exp, 3, true, 9, "xover", "", 0 },
        { "xs1", "Slope 1", "Slope 1", 0.0f, 3.0f, 2.0f, Map::choice, 0, false, -1, "xover", "6 dB/oct|12 dB/oct|24 dB/oct|48 dB/oct", 0 },
        { "xs2", "Slope 2", "Slope 2", 0.0f, 3.0f, 2.0f, Map::choice, 0, false, -1, "xover", "6 dB/oct|12 dB/oct|24 dB/oct|48 dB/oct", 0 },
        { "xs3", "Slope 3", "Slope 3", 0.0f, 3.0f, 2.0f, Map::choice, 0, false, -1, "xover", "6 dB/oct|12 dB/oct|24 dB/oct|48 dB/oct", 0 },
        { "xs4", "Slope 4", "Slope 4", 0.0f, 3.0f, 2.0f, Map::choice, 0, false, -1, "xover", "6 dB/oct|12 dB/oct|24 dB/oct|48 dB/oct", 0 },
        { "xs5", "Slope 5", "Slope 5", 0.0f, 3.0f, 2.0f, Map::choice, 0, false, -1, "xover", "6 dB/oct|12 dB/oct|24 dB/oct|48 dB/oct", 0 },
        { "b1_en", "B1 Enabled", "Enabled", 0.0f, 1.0f, 1.0f, Map::choice, 0, false, -1, "band1", "Off|On", 0 },
        { "b1_solo", "B1 Solo", "Solo", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band1", "Off|On", 0 },
        { "b1_mute", "B1 Mute", "Mute", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band1", "Off|On", 0 },
        { "b1_style", "B1 Style", "Style", 0.0f, 26.0f, 7.0f, Map::choice, 0, false, -1, "band1", "Subtle Tube|Clean Tube|Warm Tube|Juicy Tube|Broken Tube|Subtle Tape|Clean Tape|Warm Tape|Extreme Tape|Subtle Saturation|Gentle Saturation|Heavy Saturation|American Clean|American Crunch|British Crunch|British Lead|Smooth Amp|Crunchy Amp|Screaming Amp|Subtle Transformer|Warm Transformer|Lush Transformer|Smudge|Breakdown|Foldback|Rectify|Destroy", 0 },
        { "b1_mix", "B1 Mix", "Mix", 0.0f, 100.0f, 100.0f, Map::lin, 5, true, 10, "band1", "", 0 },
        { "b1_fba", "B1 Feedback", "Feedback", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 11, "band1", "", 0 },
        { "b1_fbf", "B1 Feedback Freq", "Feedback Freq", 20.0f, 20000.0f, 1000.0f, Map::exp, 3, true, 12, "band1", "", 0 },
        { "b1_dyn", "B1 Dynamics", "Dynamics", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 13, "band1", "", 0 },
        { "b1_drive", "B1 Drive", "Drive", 0.0f, 48.0f, 6.0f, Map::lin, 4, true, 14, "band1", "", 0 },
        { "b1_dpan", "B1 Drive Pan", "Drive Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 15, "band1", "", 0 },
        { "b1_bass", "B1 Bass", "Bass", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 16, "band1", "", 0 },
        { "b1_mid", "B1 Mid", "Mid", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 17, "band1", "", 0 },
        { "b1_treb", "B1 Treble", "Treble", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 18, "band1", "", 0 },
        { "b1_pres", "B1 Presence", "Presence", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 19, "band1", "", 0 },
        { "b1_level", "B1 Level", "Level", -60.0f, 36.0f, 0.0f, Map::db, 4, true, 20, "band1", "", 0 },
        { "b1_lpan", "B1 Level Pan", "Level Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 21, "band1", "", 0 },
        { "b2_en", "B2 Enabled", "Enabled", 0.0f, 1.0f, 1.0f, Map::choice, 0, false, -1, "band2", "Off|On", 0 },
        { "b2_solo", "B2 Solo", "Solo", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band2", "Off|On", 0 },
        { "b2_mute", "B2 Mute", "Mute", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band2", "Off|On", 0 },
        { "b2_style", "B2 Style", "Style", 0.0f, 26.0f, 7.0f, Map::choice, 0, false, -1, "band2", "Subtle Tube|Clean Tube|Warm Tube|Juicy Tube|Broken Tube|Subtle Tape|Clean Tape|Warm Tape|Extreme Tape|Subtle Saturation|Gentle Saturation|Heavy Saturation|American Clean|American Crunch|British Crunch|British Lead|Smooth Amp|Crunchy Amp|Screaming Amp|Subtle Transformer|Warm Transformer|Lush Transformer|Smudge|Breakdown|Foldback|Rectify|Destroy", 0 },
        { "b2_mix", "B2 Mix", "Mix", 0.0f, 100.0f, 100.0f, Map::lin, 5, true, 22, "band2", "", 0 },
        { "b2_fba", "B2 Feedback", "Feedback", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 23, "band2", "", 0 },
        { "b2_fbf", "B2 Feedback Freq", "Feedback Freq", 20.0f, 20000.0f, 1000.0f, Map::exp, 3, true, 24, "band2", "", 0 },
        { "b2_dyn", "B2 Dynamics", "Dynamics", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 25, "band2", "", 0 },
        { "b2_drive", "B2 Drive", "Drive", 0.0f, 48.0f, 6.0f, Map::lin, 4, true, 26, "band2", "", 0 },
        { "b2_dpan", "B2 Drive Pan", "Drive Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 27, "band2", "", 0 },
        { "b2_bass", "B2 Bass", "Bass", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 28, "band2", "", 0 },
        { "b2_mid", "B2 Mid", "Mid", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 29, "band2", "", 0 },
        { "b2_treb", "B2 Treble", "Treble", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 30, "band2", "", 0 },
        { "b2_pres", "B2 Presence", "Presence", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 31, "band2", "", 0 },
        { "b2_level", "B2 Level", "Level", -60.0f, 36.0f, 0.0f, Map::db, 4, true, 32, "band2", "", 0 },
        { "b2_lpan", "B2 Level Pan", "Level Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 33, "band2", "", 0 },
        { "b3_en", "B3 Enabled", "Enabled", 0.0f, 1.0f, 1.0f, Map::choice, 0, false, -1, "band3", "Off|On", 0 },
        { "b3_solo", "B3 Solo", "Solo", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band3", "Off|On", 0 },
        { "b3_mute", "B3 Mute", "Mute", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band3", "Off|On", 0 },
        { "b3_style", "B3 Style", "Style", 0.0f, 26.0f, 7.0f, Map::choice, 0, false, -1, "band3", "Subtle Tube|Clean Tube|Warm Tube|Juicy Tube|Broken Tube|Subtle Tape|Clean Tape|Warm Tape|Extreme Tape|Subtle Saturation|Gentle Saturation|Heavy Saturation|American Clean|American Crunch|British Crunch|British Lead|Smooth Amp|Crunchy Amp|Screaming Amp|Subtle Transformer|Warm Transformer|Lush Transformer|Smudge|Breakdown|Foldback|Rectify|Destroy", 0 },
        { "b3_mix", "B3 Mix", "Mix", 0.0f, 100.0f, 100.0f, Map::lin, 5, true, 34, "band3", "", 0 },
        { "b3_fba", "B3 Feedback", "Feedback", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 35, "band3", "", 0 },
        { "b3_fbf", "B3 Feedback Freq", "Feedback Freq", 20.0f, 20000.0f, 1000.0f, Map::exp, 3, true, 36, "band3", "", 0 },
        { "b3_dyn", "B3 Dynamics", "Dynamics", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 37, "band3", "", 0 },
        { "b3_drive", "B3 Drive", "Drive", 0.0f, 48.0f, 6.0f, Map::lin, 4, true, 38, "band3", "", 0 },
        { "b3_dpan", "B3 Drive Pan", "Drive Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 39, "band3", "", 0 },
        { "b3_bass", "B3 Bass", "Bass", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 40, "band3", "", 0 },
        { "b3_mid", "B3 Mid", "Mid", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 41, "band3", "", 0 },
        { "b3_treb", "B3 Treble", "Treble", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 42, "band3", "", 0 },
        { "b3_pres", "B3 Presence", "Presence", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 43, "band3", "", 0 },
        { "b3_level", "B3 Level", "Level", -60.0f, 36.0f, 0.0f, Map::db, 4, true, 44, "band3", "", 0 },
        { "b3_lpan", "B3 Level Pan", "Level Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 45, "band3", "", 0 },
        { "b4_en", "B4 Enabled", "Enabled", 0.0f, 1.0f, 1.0f, Map::choice, 0, false, -1, "band4", "Off|On", 0 },
        { "b4_solo", "B4 Solo", "Solo", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band4", "Off|On", 0 },
        { "b4_mute", "B4 Mute", "Mute", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band4", "Off|On", 0 },
        { "b4_style", "B4 Style", "Style", 0.0f, 26.0f, 7.0f, Map::choice, 0, false, -1, "band4", "Subtle Tube|Clean Tube|Warm Tube|Juicy Tube|Broken Tube|Subtle Tape|Clean Tape|Warm Tape|Extreme Tape|Subtle Saturation|Gentle Saturation|Heavy Saturation|American Clean|American Crunch|British Crunch|British Lead|Smooth Amp|Crunchy Amp|Screaming Amp|Subtle Transformer|Warm Transformer|Lush Transformer|Smudge|Breakdown|Foldback|Rectify|Destroy", 0 },
        { "b4_mix", "B4 Mix", "Mix", 0.0f, 100.0f, 100.0f, Map::lin, 5, true, 46, "band4", "", 0 },
        { "b4_fba", "B4 Feedback", "Feedback", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 47, "band4", "", 0 },
        { "b4_fbf", "B4 Feedback Freq", "Feedback Freq", 20.0f, 20000.0f, 1000.0f, Map::exp, 3, true, 48, "band4", "", 0 },
        { "b4_dyn", "B4 Dynamics", "Dynamics", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 49, "band4", "", 0 },
        { "b4_drive", "B4 Drive", "Drive", 0.0f, 48.0f, 6.0f, Map::lin, 4, true, 50, "band4", "", 0 },
        { "b4_dpan", "B4 Drive Pan", "Drive Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 51, "band4", "", 0 },
        { "b4_bass", "B4 Bass", "Bass", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 52, "band4", "", 0 },
        { "b4_mid", "B4 Mid", "Mid", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 53, "band4", "", 0 },
        { "b4_treb", "B4 Treble", "Treble", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 54, "band4", "", 0 },
        { "b4_pres", "B4 Presence", "Presence", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 55, "band4", "", 0 },
        { "b4_level", "B4 Level", "Level", -60.0f, 36.0f, 0.0f, Map::db, 4, true, 56, "band4", "", 0 },
        { "b4_lpan", "B4 Level Pan", "Level Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 57, "band4", "", 0 },
        { "b5_en", "B5 Enabled", "Enabled", 0.0f, 1.0f, 1.0f, Map::choice, 0, false, -1, "band5", "Off|On", 0 },
        { "b5_solo", "B5 Solo", "Solo", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band5", "Off|On", 0 },
        { "b5_mute", "B5 Mute", "Mute", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band5", "Off|On", 0 },
        { "b5_style", "B5 Style", "Style", 0.0f, 26.0f, 7.0f, Map::choice, 0, false, -1, "band5", "Subtle Tube|Clean Tube|Warm Tube|Juicy Tube|Broken Tube|Subtle Tape|Clean Tape|Warm Tape|Extreme Tape|Subtle Saturation|Gentle Saturation|Heavy Saturation|American Clean|American Crunch|British Crunch|British Lead|Smooth Amp|Crunchy Amp|Screaming Amp|Subtle Transformer|Warm Transformer|Lush Transformer|Smudge|Breakdown|Foldback|Rectify|Destroy", 0 },
        { "b5_mix", "B5 Mix", "Mix", 0.0f, 100.0f, 100.0f, Map::lin, 5, true, 58, "band5", "", 0 },
        { "b5_fba", "B5 Feedback", "Feedback", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 59, "band5", "", 0 },
        { "b5_fbf", "B5 Feedback Freq", "Feedback Freq", 20.0f, 20000.0f, 1000.0f, Map::exp, 3, true, 60, "band5", "", 0 },
        { "b5_dyn", "B5 Dynamics", "Dynamics", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 61, "band5", "", 0 },
        { "b5_drive", "B5 Drive", "Drive", 0.0f, 48.0f, 6.0f, Map::lin, 4, true, 62, "band5", "", 0 },
        { "b5_dpan", "B5 Drive Pan", "Drive Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 63, "band5", "", 0 },
        { "b5_bass", "B5 Bass", "Bass", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 64, "band5", "", 0 },
        { "b5_mid", "B5 Mid", "Mid", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 65, "band5", "", 0 },
        { "b5_treb", "B5 Treble", "Treble", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 66, "band5", "", 0 },
        { "b5_pres", "B5 Presence", "Presence", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 67, "band5", "", 0 },
        { "b5_level", "B5 Level", "Level", -60.0f, 36.0f, 0.0f, Map::db, 4, true, 68, "band5", "", 0 },
        { "b5_lpan", "B5 Level Pan", "Level Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 69, "band5", "", 0 },
        { "b6_en", "B6 Enabled", "Enabled", 0.0f, 1.0f, 1.0f, Map::choice, 0, false, -1, "band6", "Off|On", 0 },
        { "b6_solo", "B6 Solo", "Solo", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band6", "Off|On", 0 },
        { "b6_mute", "B6 Mute", "Mute", 0.0f, 1.0f, 0.0f, Map::choice, 0, false, -1, "band6", "Off|On", 0 },
        { "b6_style", "B6 Style", "Style", 0.0f, 26.0f, 7.0f, Map::choice, 0, false, -1, "band6", "Subtle Tube|Clean Tube|Warm Tube|Juicy Tube|Broken Tube|Subtle Tape|Clean Tape|Warm Tape|Extreme Tape|Subtle Saturation|Gentle Saturation|Heavy Saturation|American Clean|American Crunch|British Crunch|British Lead|Smooth Amp|Crunchy Amp|Screaming Amp|Subtle Transformer|Warm Transformer|Lush Transformer|Smudge|Breakdown|Foldback|Rectify|Destroy", 0 },
        { "b6_mix", "B6 Mix", "Mix", 0.0f, 100.0f, 100.0f, Map::lin, 5, true, 70, "band6", "", 0 },
        { "b6_fba", "B6 Feedback", "Feedback", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 71, "band6", "", 0 },
        { "b6_fbf", "B6 Feedback Freq", "Feedback Freq", 20.0f, 20000.0f, 1000.0f, Map::exp, 3, true, 72, "band6", "", 0 },
        { "b6_dyn", "B6 Dynamics", "Dynamics", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 73, "band6", "", 0 },
        { "b6_drive", "B6 Drive", "Drive", 0.0f, 48.0f, 6.0f, Map::lin, 4, true, 74, "band6", "", 0 },
        { "b6_dpan", "B6 Drive Pan", "Drive Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 75, "band6", "", 0 },
        { "b6_bass", "B6 Bass", "Bass", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 76, "band6", "", 0 },
        { "b6_mid", "B6 Mid", "Mid", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 77, "band6", "", 0 },
        { "b6_treb", "B6 Treble", "Treble", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 78, "band6", "", 0 },
        { "b6_pres", "B6 Presence", "Presence", -12.0f, 12.0f, 0.0f, Map::lin, 4, true, 79, "band6", "", 0 },
        { "b6_level", "B6 Level", "Level", -60.0f, 36.0f, 0.0f, Map::db, 4, true, 80, "band6", "", 0 },
        { "b6_lpan", "B6 Level Pan", "Level Pan", -100.0f, 100.0f, 0.0f, Map::lin, 6, true, 81, "band6", "", 0 },
        { "x1_rate", "XLFO1 Rate", "Rate", 0.02f, 500.0f, 1.0f, Map::exp, 3, true, 82, "xlfo1", "", 0 },
        { "x1_div", "XLFO1 Sync", "Sync", 0.0f, 10.0f, 6.0f, Map::choice, 0, false, -1, "xlfo1", "16|8|4|2|1|1/2|1/4|1/8|1/16|1/32|1/64", 0 },
        { "x1_ofs", "XLFO1 Offset", "Offset", 0.5f, 2.0f, 1.0f, Map::exp, 1, true, 83, "xlfo1", "", 0 },
        { "x1_bal", "XLFO1 Balance", "Balance", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 84, "xlfo1", "", 0 },
        { "x1_glide", "XLFO1 Glide", "Glide", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 85, "xlfo1", "", 0 },
        { "x1_phase", "XLFO1 Phase", "Phase", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 86, "xlfo1", "", 0 },
        { "x2_rate", "XLFO2 Rate", "Rate", 0.02f, 500.0f, 1.0f, Map::exp, 3, true, 87, "xlfo2", "", 0 },
        { "x2_div", "XLFO2 Sync", "Sync", 0.0f, 10.0f, 6.0f, Map::choice, 0, false, -1, "xlfo2", "16|8|4|2|1|1/2|1/4|1/8|1/16|1/32|1/64", 0 },
        { "x2_ofs", "XLFO2 Offset", "Offset", 0.5f, 2.0f, 1.0f, Map::exp, 1, true, 88, "xlfo2", "", 0 },
        { "x2_bal", "XLFO2 Balance", "Balance", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 89, "xlfo2", "", 0 },
        { "x2_glide", "XLFO2 Glide", "Glide", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 90, "xlfo2", "", 0 },
        { "x2_phase", "XLFO2 Phase", "Phase", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 91, "xlfo2", "", 0 },
        { "x3_rate", "XLFO3 Rate", "Rate", 0.02f, 500.0f, 1.0f, Map::exp, 3, true, 92, "xlfo3", "", 0 },
        { "x3_div", "XLFO3 Sync", "Sync", 0.0f, 10.0f, 6.0f, Map::choice, 0, false, -1, "xlfo3", "16|8|4|2|1|1/2|1/4|1/8|1/16|1/32|1/64", 0 },
        { "x3_ofs", "XLFO3 Offset", "Offset", 0.5f, 2.0f, 1.0f, Map::exp, 1, true, 93, "xlfo3", "", 0 },
        { "x3_bal", "XLFO3 Balance", "Balance", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 94, "xlfo3", "", 0 },
        { "x3_glide", "XLFO3 Glide", "Glide", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 95, "xlfo3", "", 0 },
        { "x3_phase", "XLFO3 Phase", "Phase", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 96, "xlfo3", "", 0 },
        { "x4_rate", "XLFO4 Rate", "Rate", 0.02f, 500.0f, 1.0f, Map::exp, 3, true, 97, "xlfo4", "", 0 },
        { "x4_div", "XLFO4 Sync", "Sync", 0.0f, 10.0f, 6.0f, Map::choice, 0, false, -1, "xlfo4", "16|8|4|2|1|1/2|1/4|1/8|1/16|1/32|1/64", 0 },
        { "x4_ofs", "XLFO4 Offset", "Offset", 0.5f, 2.0f, 1.0f, Map::exp, 1, true, 98, "xlfo4", "", 0 },
        { "x4_bal", "XLFO4 Balance", "Balance", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 99, "xlfo4", "", 0 },
        { "x4_glide", "XLFO4 Glide", "Glide", -100.0f, 100.0f, 0.0f, Map::lin, 5, true, 100, "xlfo4", "", 0 },
        { "x4_phase", "XLFO4 Phase", "Phase", 0.0f, 100.0f, 0.0f, Map::lin, 5, true, 101, "xlfo4", "", 0 },
        { "e1_thr", "EG1 Threshold", "Threshold", -60.0f, 0.0f, -24.0f, Map::lin, 4, true, 102, "eg1", "", 0 },
        { "e1_del", "EG1 Delay", "Delay", 0.0f, 2000.0f, 0.0f, Map::pow3, 2, true, 103, "eg1", "", 0 },
        { "e1_att", "EG1 Attack", "Attack", 0.0f, 5000.0f, 5.0f, Map::pow3, 2, true, 104, "eg1", "", 0 },
        { "e1_dec", "EG1 Decay", "Decay", 0.0f, 10000.0f, 200.0f, Map::pow3, 2, true, 105, "eg1", "", 0 },
        { "e1_sus", "EG1 Sustain", "Sustain", 0.0f, 100.0f, 50.0f, Map::lin, 5, true, 106, "eg1", "", 0 },
        { "e1_hold", "EG1 Hold", "Hold", 0.0f, 5000.0f, 0.0f, Map::pow3, 2, true, 107, "eg1", "", 0 },
        { "e1_rel", "EG1 Release", "Release", 0.0f, 10000.0f, 300.0f, Map::pow3, 2, true, 108, "eg1", "", 0 },
        { "e2_thr", "EG2 Threshold", "Threshold", -60.0f, 0.0f, -24.0f, Map::lin, 4, true, 109, "eg2", "", 0 },
        { "e2_del", "EG2 Delay", "Delay", 0.0f, 2000.0f, 0.0f, Map::pow3, 2, true, 110, "eg2", "", 0 },
        { "e2_att", "EG2 Attack", "Attack", 0.0f, 5000.0f, 5.0f, Map::pow3, 2, true, 111, "eg2", "", 0 },
        { "e2_dec", "EG2 Decay", "Decay", 0.0f, 10000.0f, 200.0f, Map::pow3, 2, true, 112, "eg2", "", 0 },
        { "e2_sus", "EG2 Sustain", "Sustain", 0.0f, 100.0f, 50.0f, Map::lin, 5, true, 113, "eg2", "", 0 },
        { "e2_hold", "EG2 Hold", "Hold", 0.0f, 5000.0f, 0.0f, Map::pow3, 2, true, 114, "eg2", "", 0 },
        { "e2_rel", "EG2 Release", "Release", 0.0f, 10000.0f, 300.0f, Map::pow3, 2, true, 115, "eg2", "", 0 },
        { "e3_thr", "EG3 Threshold", "Threshold", -60.0f, 0.0f, -24.0f, Map::lin, 4, true, 116, "eg3", "", 0 },
        { "e3_del", "EG3 Delay", "Delay", 0.0f, 2000.0f, 0.0f, Map::pow3, 2, true, 117, "eg3", "", 0 },
        { "e3_att", "EG3 Attack", "Attack", 0.0f, 5000.0f, 5.0f, Map::pow3, 2, true, 118, "eg3", "", 0 },
        { "e3_dec", "EG3 Decay", "Decay", 0.0f, 10000.0f, 200.0f, Map::pow3, 2, true, 119, "eg3", "", 0 },
        { "e3_sus", "EG3 Sustain", "Sustain", 0.0f, 100.0f, 50.0f, Map::lin, 5, true, 120, "eg3", "", 0 },
        { "e3_hold", "EG3 Hold", "Hold", 0.0f, 5000.0f, 0.0f, Map::pow3, 2, true, 121, "eg3", "", 0 },
        { "e3_rel", "EG3 Release", "Release", 0.0f, 10000.0f, 300.0f, Map::pow3, 2, true, 122, "eg3", "", 0 },
        { "e4_thr", "EG4 Threshold", "Threshold", -60.0f, 0.0f, -24.0f, Map::lin, 4, true, 123, "eg4", "", 0 },
        { "e4_del", "EG4 Delay", "Delay", 0.0f, 2000.0f, 0.0f, Map::pow3, 2, true, 124, "eg4", "", 0 },
        { "e4_att", "EG4 Attack", "Attack", 0.0f, 5000.0f, 5.0f, Map::pow3, 2, true, 125, "eg4", "", 0 },
        { "e4_dec", "EG4 Decay", "Decay", 0.0f, 10000.0f, 200.0f, Map::pow3, 2, true, 126, "eg4", "", 0 },
        { "e4_sus", "EG4 Sustain", "Sustain", 0.0f, 100.0f, 50.0f, Map::lin, 5, true, 127, "eg4", "", 0 },
        { "e4_hold", "EG4 Hold", "Hold", 0.0f, 5000.0f, 0.0f, Map::pow3, 2, true, 128, "eg4", "", 0 },
        { "e4_rel", "EG4 Release", "Release", 0.0f, 10000.0f, 300.0f, Map::pow3, 2, true, 129, "eg4", "", 0 },
        { "f1_att", "EF1 Attack", "Attack", 0.1f, 1000.0f, 5.0f, Map::exp, 2, true, 130, "ef1", "", 0 },
        { "f1_rel", "EF1 Release", "Release", 1.0f, 5000.0f, 150.0f, Map::exp, 2, true, 131, "ef1", "", 0 },
        { "f2_att", "EF2 Attack", "Attack", 0.1f, 1000.0f, 5.0f, Map::exp, 2, true, 132, "ef2", "", 0 },
        { "f2_rel", "EF2 Release", "Release", 1.0f, 5000.0f, 150.0f, Map::exp, 2, true, 133, "ef2", "", 0 },
        { "f3_att", "EF3 Attack", "Attack", 0.1f, 1000.0f, 5.0f, Map::exp, 2, true, 134, "ef3", "", 0 },
        { "f3_rel", "EF3 Release", "Release", 1.0f, 5000.0f, 150.0f, Map::exp, 2, true, 135, "ef3", "", 0 },
        { "f4_att", "EF4 Attack", "Attack", 0.1f, 1000.0f, 5.0f, Map::exp, 2, true, 136, "ef4", "", 0 },
        { "f4_rel", "EF4 Release", "Release", 1.0f, 5000.0f, 150.0f, Map::exp, 2, true, 137, "ef4", "", 0 },
        { "xy1_x", "XY1 X", "X", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy1", "", 0 },
        { "xy1_y", "XY1 Y", "Y", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy1", "", 0 },
        { "xy2_x", "XY2 X", "X", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy2", "", 0 },
        { "xy2_y", "XY2 Y", "Y", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy2", "", 0 },
        { "xy3_x", "XY3 X", "X", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy3", "", 0 },
        { "xy3_y", "XY3 Y", "Y", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy3", "", 0 },
        { "xy4_x", "XY4 X", "X", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy4", "", 0 },
        { "xy4_y", "XY4 Y", "Y", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy4", "", 0 },
        { "xy5_x", "XY5 X", "X", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy5", "", 0 },
        { "xy5_y", "XY5 Y", "Y", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy5", "", 0 },
        { "xy6_x", "XY6 X", "X", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy6", "", 0 },
        { "xy6_y", "XY6 Y", "Y", -100.0f, 100.0f, 0.0f, Map::lin, 5, false, -1, "xy6", "", 0 },
    }};
    return p;
}

namespace P
{
    constexpr int gain_in = 0;
    constexpr int pan_in = 1;
    constexpr int gain_out = 2;
    constexpr int pan_out = 3;
    constexpr int gmix = 4;
    constexpr int hq = 5;
    constexpr int linphase = 6;
    constexpr int msmode = 7;
    constexpr int unrestricted = 8;
    constexpr int bypass = 9;
    constexpr int nbands = 10;
    constexpr int xf1 = 11;
    constexpr int xf2 = 12;
    constexpr int xf3 = 13;
    constexpr int xf4 = 14;
    constexpr int xf5 = 15;
    constexpr int xs1 = 16;
    constexpr int xs2 = 17;
    constexpr int xs3 = 18;
    constexpr int xs4 = 19;
    constexpr int xs5 = 20;
    constexpr int b1_en = 21;
    constexpr int b1_solo = 22;
    constexpr int b1_mute = 23;
    constexpr int b1_style = 24;
    constexpr int b1_mix = 25;
    constexpr int b1_fba = 26;
    constexpr int b1_fbf = 27;
    constexpr int b1_dyn = 28;
    constexpr int b1_drive = 29;
    constexpr int b1_dpan = 30;
    constexpr int b1_bass = 31;
    constexpr int b1_mid = 32;
    constexpr int b1_treb = 33;
    constexpr int b1_pres = 34;
    constexpr int b1_level = 35;
    constexpr int b1_lpan = 36;
    constexpr int b2_en = 37;
    constexpr int b2_solo = 38;
    constexpr int b2_mute = 39;
    constexpr int b2_style = 40;
    constexpr int b2_mix = 41;
    constexpr int b2_fba = 42;
    constexpr int b2_fbf = 43;
    constexpr int b2_dyn = 44;
    constexpr int b2_drive = 45;
    constexpr int b2_dpan = 46;
    constexpr int b2_bass = 47;
    constexpr int b2_mid = 48;
    constexpr int b2_treb = 49;
    constexpr int b2_pres = 50;
    constexpr int b2_level = 51;
    constexpr int b2_lpan = 52;
    constexpr int b3_en = 53;
    constexpr int b3_solo = 54;
    constexpr int b3_mute = 55;
    constexpr int b3_style = 56;
    constexpr int b3_mix = 57;
    constexpr int b3_fba = 58;
    constexpr int b3_fbf = 59;
    constexpr int b3_dyn = 60;
    constexpr int b3_drive = 61;
    constexpr int b3_dpan = 62;
    constexpr int b3_bass = 63;
    constexpr int b3_mid = 64;
    constexpr int b3_treb = 65;
    constexpr int b3_pres = 66;
    constexpr int b3_level = 67;
    constexpr int b3_lpan = 68;
    constexpr int b4_en = 69;
    constexpr int b4_solo = 70;
    constexpr int b4_mute = 71;
    constexpr int b4_style = 72;
    constexpr int b4_mix = 73;
    constexpr int b4_fba = 74;
    constexpr int b4_fbf = 75;
    constexpr int b4_dyn = 76;
    constexpr int b4_drive = 77;
    constexpr int b4_dpan = 78;
    constexpr int b4_bass = 79;
    constexpr int b4_mid = 80;
    constexpr int b4_treb = 81;
    constexpr int b4_pres = 82;
    constexpr int b4_level = 83;
    constexpr int b4_lpan = 84;
    constexpr int b5_en = 85;
    constexpr int b5_solo = 86;
    constexpr int b5_mute = 87;
    constexpr int b5_style = 88;
    constexpr int b5_mix = 89;
    constexpr int b5_fba = 90;
    constexpr int b5_fbf = 91;
    constexpr int b5_dyn = 92;
    constexpr int b5_drive = 93;
    constexpr int b5_dpan = 94;
    constexpr int b5_bass = 95;
    constexpr int b5_mid = 96;
    constexpr int b5_treb = 97;
    constexpr int b5_pres = 98;
    constexpr int b5_level = 99;
    constexpr int b5_lpan = 100;
    constexpr int b6_en = 101;
    constexpr int b6_solo = 102;
    constexpr int b6_mute = 103;
    constexpr int b6_style = 104;
    constexpr int b6_mix = 105;
    constexpr int b6_fba = 106;
    constexpr int b6_fbf = 107;
    constexpr int b6_dyn = 108;
    constexpr int b6_drive = 109;
    constexpr int b6_dpan = 110;
    constexpr int b6_bass = 111;
    constexpr int b6_mid = 112;
    constexpr int b6_treb = 113;
    constexpr int b6_pres = 114;
    constexpr int b6_level = 115;
    constexpr int b6_lpan = 116;
    constexpr int x1_rate = 117;
    constexpr int x1_div = 118;
    constexpr int x1_ofs = 119;
    constexpr int x1_bal = 120;
    constexpr int x1_glide = 121;
    constexpr int x1_phase = 122;
    constexpr int x2_rate = 123;
    constexpr int x2_div = 124;
    constexpr int x2_ofs = 125;
    constexpr int x2_bal = 126;
    constexpr int x2_glide = 127;
    constexpr int x2_phase = 128;
    constexpr int x3_rate = 129;
    constexpr int x3_div = 130;
    constexpr int x3_ofs = 131;
    constexpr int x3_bal = 132;
    constexpr int x3_glide = 133;
    constexpr int x3_phase = 134;
    constexpr int x4_rate = 135;
    constexpr int x4_div = 136;
    constexpr int x4_ofs = 137;
    constexpr int x4_bal = 138;
    constexpr int x4_glide = 139;
    constexpr int x4_phase = 140;
    constexpr int e1_thr = 141;
    constexpr int e1_del = 142;
    constexpr int e1_att = 143;
    constexpr int e1_dec = 144;
    constexpr int e1_sus = 145;
    constexpr int e1_hold = 146;
    constexpr int e1_rel = 147;
    constexpr int e2_thr = 148;
    constexpr int e2_del = 149;
    constexpr int e2_att = 150;
    constexpr int e2_dec = 151;
    constexpr int e2_sus = 152;
    constexpr int e2_hold = 153;
    constexpr int e2_rel = 154;
    constexpr int e3_thr = 155;
    constexpr int e3_del = 156;
    constexpr int e3_att = 157;
    constexpr int e3_dec = 158;
    constexpr int e3_sus = 159;
    constexpr int e3_hold = 160;
    constexpr int e3_rel = 161;
    constexpr int e4_thr = 162;
    constexpr int e4_del = 163;
    constexpr int e4_att = 164;
    constexpr int e4_dec = 165;
    constexpr int e4_sus = 166;
    constexpr int e4_hold = 167;
    constexpr int e4_rel = 168;
    constexpr int f1_att = 169;
    constexpr int f1_rel = 170;
    constexpr int f2_att = 171;
    constexpr int f2_rel = 172;
    constexpr int f3_att = 173;
    constexpr int f3_rel = 174;
    constexpr int f4_att = 175;
    constexpr int f4_rel = 176;
    constexpr int xy1_x = 177;
    constexpr int xy1_y = 178;
    constexpr int xy2_x = 179;
    constexpr int xy2_y = 180;
    constexpr int xy3_x = 181;
    constexpr int xy3_y = 182;
    constexpr int xy4_x = 183;
    constexpr int xy4_y = 184;
    constexpr int xy5_x = 185;
    constexpr int xy5_y = 186;
    constexpr int xy6_x = 187;
    constexpr int xy6_y = 188;
}

// offset dei parametri di banda (indice = b1_en + banda*NBP + offset)
namespace BP
{
    constexpr int en = 0;
    constexpr int solo = 1;
    constexpr int mute = 2;
    constexpr int style = 3;
    constexpr int mix = 4;
    constexpr int fba = 5;
    constexpr int fbf = 6;
    constexpr int dyn = 7;
    constexpr int drive = 8;
    constexpr int dpan = 9;
    constexpr int bass = 10;
    constexpr int mid = 11;
    constexpr int treb = 12;
    constexpr int pres = 13;
    constexpr int level = 14;
    constexpr int lpan = 15;
}
inline constexpr int bandParam (int band, int off) { return P::b1_en + band * NBP + off; }

struct Style { const char* name; const char* cat; float shtype, dscale, dofs, bias, prehp, preeqF, preeqG, preeqQ, posteqF, posteqG, posteqQ, postlp, cabF, cabG, special, inner; };
inline const std::array<Style, NSTYLES>& styles()
{
    static const std::array<Style, NSTYLES> s {{
        { "Subtle Tube", "Tube", 1.0f, 0.35f, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 0.0f, 0.6f },
        { "Clean Tube", "Tube", 1.0f, 0.6f, 0.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Warm Tube", "Tube", 1.0f, 0.8f, 0.0f, 0.25f, 0.0f, 150.0f, 1.5f, 0.7f, 6000.0f, -1.5f, 0.7f, 16000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Juicy Tube", "Tube", 1.0f, 1.0f, 6.0f, 0.4f, 0.0f, 800.0f, 3.0f, 0.6f, 3000.0f, -2.0f, 0.7f, 15000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Broken Tube", "Tube", 6.0f, 1.0f, 6.0f, 0.35f, 60.0f, 1200.0f, 4.0f, 0.8f, 0.0f, 0.0f, 0.7f, 12000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Subtle Tape", "Tape", 2.0f, 0.35f, 0.0f, 0.0f, 0.0f, 5000.0f, 4.0f, 0.6f, 5000.0f, -4.0f, 0.6f, 18000.0f, 90.0f, 1.0f, 0.0f, 0.6f },
        { "Clean Tape", "Tape", 2.0f, 0.6f, 0.0f, 0.0f, 0.0f, 5000.0f, 5.0f, 0.6f, 5000.0f, -5.0f, 0.6f, 16000.0f, 90.0f, 1.5f, 0.0f, 1.0f },
        { "Warm Tape", "Tape", 2.0f, 0.8f, 0.0f, 0.05f, 0.0f, 5000.0f, 6.0f, 0.6f, 5000.0f, -6.0f, 0.6f, 13000.0f, 80.0f, 2.5f, 0.0f, 1.0f },
        { "Extreme Tape", "Tape", 2.0f, 1.0f, 9.0f, 0.1f, 0.0f, 4500.0f, 8.0f, 0.6f, 4500.0f, -8.0f, 0.6f, 9000.0f, 70.0f, 3.0f, 0.0f, 1.0f },
        { "Subtle Saturation", "Saturation", 3.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 0.0f, 0.6f },
        { "Gentle Saturation", "Saturation", 3.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Heavy Saturation", "Saturation", 3.0f, 1.1f, 6.0f, 0.05f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 18000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "American Clean", "Amplifier", 5.0f, 0.6f, 3.0f, 0.1f, 70.0f, 600.0f, 2.0f, 0.7f, 500.0f, -3.0f, 0.8f, 7500.0f, 2400.0f, 2.0f, 0.0f, 1.0f },
        { "American Crunch", "Amplifier", 5.0f, 0.9f, 15.0f, 0.2f, 90.0f, 700.0f, 4.0f, 0.7f, 450.0f, -5.0f, 0.9f, 6000.0f, 2500.0f, 3.0f, 0.0f, 1.0f },
        { "British Crunch", "Amplifier", 5.0f, 0.9f, 15.0f, 0.25f, 100.0f, 900.0f, 5.0f, 0.8f, 1000.0f, 2.0f, 0.8f, 5500.0f, 2200.0f, 3.0f, 0.0f, 1.0f },
        { "British Lead", "Amplifier", 5.0f, 1.0f, 27.0f, 0.25f, 120.0f, 1000.0f, 6.0f, 0.8f, 800.0f, 2.0f, 0.9f, 5000.0f, 2000.0f, 4.0f, 0.0f, 1.0f },
        { "Smooth Amp", "Amplifier", 5.0f, 0.9f, 18.0f, 0.15f, 90.0f, 700.0f, 3.0f, 0.6f, 600.0f, -2.0f, 0.7f, 4500.0f, 1800.0f, 2.0f, 0.0f, 1.0f },
        { "Crunchy Amp", "Amplifier", 5.0f, 1.0f, 21.0f, 0.3f, 110.0f, 1400.0f, 6.0f, 0.9f, 700.0f, -4.0f, 0.9f, 6000.0f, 2800.0f, 4.0f, 0.0f, 1.0f },
        { "Screaming Amp", "Amplifier", 5.0f, 1.1f, 33.0f, 0.3f, 150.0f, 1600.0f, 8.0f, 1.0f, 500.0f, -6.0f, 1.0f, 5500.0f, 3000.0f, 5.0f, 0.0f, 1.0f },
        { "Subtle Transformer", "Transformer", 4.0f, 0.4f, 0.0f, 0.1f, 0.0f, 60.0f, 4.0f, 0.7f, 60.0f, -4.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 0.0f, 0.6f },
        { "Warm Transformer", "Transformer", 4.0f, 0.8f, 0.0f, 0.25f, 0.0f, 70.0f, 6.0f, 0.7f, 70.0f, -5.0f, 0.7f, 17000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Lush Transformer", "Transformer", 4.0f, 1.0f, 3.0f, 0.45f, 0.0f, 80.0f, 7.0f, 0.7f, 80.0f, -5.0f, 0.7f, 14000.0f, 3500.0f, 1.5f, 0.0f, 1.0f },
        { "Smudge", "FX", 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 1.0f, 1.0f },
        { "Breakdown", "FX", 9.0f, 1.0f, 6.0f, 0.0f, 30.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 16000.0f, 0.0f, 0.0f, 2.0f, 1.0f },
        { "Foldback", "FX", 7.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Rectify", "FX", 8.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 0.0f, 1.0f },
        { "Destroy", "FX", 9.0f, 1.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 20000.0f, 0.0f, 0.0f, 3.0f, 1.0f },
    }};
    return s;
}

inline const char* const targetIds[NT] = { "gain_in", "pan_in", "gain_out", "pan_out", "gmix", "xf1", "xf2", "xf3", "xf4", "xf5", "b1_mix", "b1_fba", "b1_fbf", "b1_dyn", "b1_drive", "b1_dpan", "b1_bass", "b1_mid", "b1_treb", "b1_pres", "b1_level", "b1_lpan", "b2_mix", "b2_fba", "b2_fbf", "b2_dyn", "b2_drive", "b2_dpan", "b2_bass", "b2_mid", "b2_treb", "b2_pres", "b2_level", "b2_lpan", "b3_mix", "b3_fba", "b3_fbf", "b3_dyn", "b3_drive", "b3_dpan", "b3_bass", "b3_mid", "b3_treb", "b3_pres", "b3_level", "b3_lpan", "b4_mix", "b4_fba", "b4_fbf", "b4_dyn", "b4_drive", "b4_dpan", "b4_bass", "b4_mid", "b4_treb", "b4_pres", "b4_level", "b4_lpan", "b5_mix", "b5_fba", "b5_fbf", "b5_dyn", "b5_drive", "b5_dpan", "b5_bass", "b5_mid", "b5_treb", "b5_pres", "b5_level", "b5_lpan", "b6_mix", "b6_fba", "b6_fbf", "b6_dyn", "b6_drive", "b6_dpan", "b6_bass", "b6_mid", "b6_treb", "b6_pres", "b6_level", "b6_lpan", "x1_rate", "x1_ofs", "x1_bal", "x1_glide", "x1_phase", "x2_rate", "x2_ofs", "x2_bal", "x2_glide", "x2_phase", "x3_rate", "x3_ofs", "x3_bal", "x3_glide", "x3_phase", "x4_rate", "x4_ofs", "x4_bal", "x4_glide", "x4_phase", "e1_thr", "e1_del", "e1_att", "e1_dec", "e1_sus", "e1_hold", "e1_rel", "e2_thr", "e2_del", "e2_att", "e2_dec", "e2_sus", "e2_hold", "e2_rel", "e3_thr", "e3_del", "e3_att", "e3_dec", "e3_sus", "e3_hold", "e3_rel", "e4_thr", "e4_del", "e4_att", "e4_dec", "e4_sus", "e4_hold", "e4_rel", "f1_att", "f1_rel", "f2_att", "f2_rel", "f3_att", "f3_rel", "f4_att", "f4_rel", "slot1", "slot2", "slot3", "slot4", "slot5", "slot6", "slot7", "slot8", "slot9", "slot10", "slot11", "slot12", "slot13", "slot14", "slot15", "slot16", "slot17", "slot18", "slot19", "slot20", "slot21", "slot22", "slot23", "slot24", "slot25", "slot26", "slot27", "slot28", "slot29", "slot30", "slot31", "slot32" };
inline const char* const sourceIds[NSRC] = { "none", "xlfo1", "xlfo2", "xlfo3", "xlfo4", "eg1", "eg2", "eg3", "eg4", "ef1", "ef2", "ef3", "ef4", "midi1", "midi2", "midi3", "midi4", "xy1_x", "xy1_y", "xy2_x", "xy2_y", "xy3_x", "xy3_y", "xy4_x", "xy4_y", "xy5_x", "xy5_y", "xy6_x", "xy6_y" };
namespace SRC
{
    constexpr int none_ = 0;
    constexpr int xlfo1 = 1;
    constexpr int xlfo2 = 2;
    constexpr int xlfo3 = 3;
    constexpr int xlfo4 = 4;
    constexpr int eg1 = 5;
    constexpr int eg2 = 6;
    constexpr int eg3 = 7;
    constexpr int eg4 = 8;
    constexpr int ef1 = 9;
    constexpr int ef2 = 10;
    constexpr int ef3 = 11;
    constexpr int ef4 = 12;
    constexpr int midi1 = 13;
    constexpr int midi2 = 14;
    constexpr int midi3 = 15;
    constexpr int midi4 = 16;
    constexpr int xy1_x = 17;
    constexpr int xy1_y = 18;
    constexpr int xy2_x = 19;
    constexpr int xy2_y = 20;
    constexpr int xy3_x = 21;
    constexpr int xy3_y = 22;
    constexpr int xy4_x = 23;
    constexpr int xy4_y = 24;
    constexpr int xy5_x = 25;
    constexpr int xy5_y = 26;
    constexpr int xy6_x = 27;
    constexpr int xy6_y = 28;
}
inline const char* const syncDivs[11] = { "16", "8", "4", "2", "1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };

// buffer di configurazione (interfaccia -> motore)
namespace CFG
{
    constexpr int SLOT_BASE = 100;
    constexpr int SLOT_STRIDE = 8;
    constexpr int XLFO_BASE = 400;
    constexpr int XLFO_STRIDE = 80;
    constexpr int EG_BASE = 800;
    constexpr int EG_STRIDE = 16;
    constexpr int EF_BASE = 900;
    constexpr int EF_STRIDE = 16;
    constexpr int XY_BASE = 1000;
    constexpr int XY_STRIDE = 4;
    constexpr int MIDI_BASE = 1100;
    constexpr int MIDI_STRIDE = 4;
    constexpr int SIZE = 1280;
}
// buffer di visualizzazione (motore -> interfaccia)
namespace VIEW
{
    constexpr int TGT = 0;
    constexpr int SRC = 200;
    constexpr int SCOPEPOS = 240;
    constexpr int TRIG = 241;
    constexpr int LFOPH = 250;
    constexpr int LFOSTEP = 254;
    constexpr int EGSTAGE = 258;
    constexpr int METER = 262;
    constexpr int BANDOUT = 266;
    constexpr int LATENCY = 272;
    constexpr int SIZE = 512;
}
} // namespace spec
