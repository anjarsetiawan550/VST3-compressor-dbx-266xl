// Dibuat otomatis dari layout.json (koordinat skala 1x, relatif ke pojok kiri-atas background).
#pragma once

namespace dbx { namespace layout {

constexpr int kBgW = 2041;
constexpr int kBgH = 184;
constexpr int kKnobFrames = 64;

struct KnobItem { const char* id; int x, y, w, h; };
inline constexpr KnobItem kKnobs[14] = {
    {"ch1_exp_threshold", 156, 68, 68, 68},
    {"ch1_exp_ratio", 269, 69, 68, 68},
    {"ch1_comp_threshold", 382, 69, 68, 68},
    {"ch1_comp_ratio", 506, 69, 68, 68},
    {"ch1_attack", 619, 69, 68, 68},
    {"ch1_release", 743, 69, 68, 68},
    {"ch1_output_gain", 856, 69, 68, 68},
    {"ch2_exp_threshold", 1071, 70, 68, 68},
    {"ch2_exp_ratio", 1185, 70, 68, 68},
    {"ch2_comp_threshold", 1298, 70, 68, 68},
    {"ch2_comp_ratio", 1422, 70, 68, 68},
    {"ch2_attack", 1535, 71, 68, 68},
    {"ch2_release", 1659, 69, 68, 68},
    {"ch2_output_gain", 1773, 71, 68, 68},
};

struct ButtonItem { const char* paramId; const char* color; int x, y, w, h; };
inline constexpr ButtonItem kButtons[7] = {
    {"ch1_overeasy", "green", 463, 128, 30, 30},
    {"ch1_auto", "green", 700, 128, 30, 30},
    {"ch1_bypass", "red", 941, 129, 30, 30},
    {"stereo_couple", "yellow", 1004, 129, 30, 30},
    {"ch2_overeasy", "green", 1378, 129, 30, 30},
    {"ch2_auto", "green", 1619, 129, 30, 30},
    {"ch2_bypass", "red", 1858, 130, 30, 30},
};

// 26 LED: channel 1 (13) lalu channel 2 (13). Urutan per channel:
// 0 exp_below(merah) 1 exp_above(hijau) 2 comp_below(hijau) 3 comp_0db(kuning) 4 comp_above(merah) 5..12 gr_30..gr_1
struct LedItem { const char* color; int x, y, w, h; };
inline constexpr LedItem kLeds[26] = {
    {"red", 170, 25, 20, 20},
    {"green", 191, 25, 20, 20},
    {"green", 385, 25, 20, 20},
    {"yellow", 407, 25, 20, 20},
    {"red", 428, 25, 20, 20},
    {"red", 630, 25, 20, 20},
    {"red", 651, 25, 20, 20},
    {"red", 673, 25, 20, 20},
    {"red", 694, 25, 20, 20},
    {"red", 715, 25, 20, 20},
    {"red", 737, 25, 20, 20},
    {"red", 758, 25, 20, 20},
    {"red", 780, 25, 20, 20},
    {"red", 1085, 26, 20, 20},
    {"green", 1106, 26, 20, 20},
    {"green", 1300, 26, 20, 20},
    {"yellow", 1322, 26, 20, 20},
    {"red", 1343, 26, 20, 20},
    {"red", 1544, 26, 20, 20},
    {"red", 1565, 26, 20, 20},
    {"red", 1587, 26, 20, 20},
    {"red", 1609, 26, 20, 20},
    {"red", 1630, 26, 20, 20},
    {"red", 1652, 26, 20, 20},
    {"red", 1673, 26, 20, 20},
    {"red", 1695, 26, 20, 20},
};

} } // namespace dbx::layout
