// Pemetaan posisi knob (0..1) ke nilai sesuai skala yang tercetak di panel.
#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

namespace dbx
{
inline float interp (const float* xs, const float* ys, int n, float x)
{
    if (x <= xs[0]) return ys[0];
    for (int i = 1; i < n; ++i)
        if (x <= xs[i])
        {
            const float t = (x - xs[i - 1]) / (xs[i] - xs[i - 1]);
            return ys[i - 1] + t * (ys[i] - ys[i - 1]);
        }
    return ys[n - 1];
}

// Expander/Gate threshold: OFF .. +15 dBu (skala -60 -40 -20 0)
inline float expThresholdDbu (float n)
{
    if (n < 0.02f) return -999.0f;                                  // OFF
    static const float xs[] = { 0.02f, 0.20f, 0.47f, 0.67f, 0.83f, 1.0f };
    static const float ys[] = { -80.f, -60.f, -40.f, -20.f, 0.f, 15.f };
    return interp (xs, ys, 6, n);
}
// Expander/Gate ratio: 1:1 .. 4:1 (1.3 2 3)
inline float expRatio (float n)
{
    static const float xs[] = { 0.f, 0.25f, 0.5f, 0.75f, 1.f };
    static const float ys[] = { 1.f, 1.3f, 2.f, 3.f, 4.f };
    return interp (xs, ys, 5, n);
}
// Compressor threshold: -40 .. +20 dBu
inline float compThresholdDbu (float n) { return -40.0f + 60.0f * n; }
// Compressor ratio: 1:1 .. inf:1 (1.3 2 4)
inline float compRatio (float n)
{
    static const float xs[] = { 0.f, 0.25f, 0.5f, 0.75f, 0.92f, 1.f };
    static const float ys[] = { 1.f, 1.3f, 2.f, 4.f, 10.f, 1000.f };
    return interp (xs, ys, 6, n);
}
// Attack FAST..SLOW: waktu untuk 15 dB gain reduction (3 ms .. 340 ms); posisi tengah = dbx klasik
// Release FAST..SLOW: waktu untuk 15 dB gain reduction (75 ms .. 5000 ms);
// posisi tengah = dbx klasik (120 dB/s = 125 ms untuk 15 dB).
inline float interpLog (const float* xs, const float* ys, int n, float x)
{
    float l[4];
    for (int i = 0; i < n; ++i) l[i] = std::log (ys[i]);
    return std::exp (interp (xs, l, n, x));
}
inline float attackTime15Ms (float n)
{
    static const float xs[] = { 0.f, 0.5f, 1.f };
    static const float ys[] = { 3.f, 22.f, 340.f };
    return interpLog (xs, ys, 3, n);
}
inline float releaseDbPerS (float n)
{
    static const float xs[] = { 0.f, 0.5f, 1.f };
    static const float ys[] = { 200.f, 120.f, 3.f };
    return interpLog (xs, ys, 3, n);
}
// Konversi laju release internal menjadi waktu untuk menurunkan 15 dB gain reduction.
inline float releaseTime15Ms (float n) { return 15000.0f / releaseDbPerS (n); }
// Output gain -20 .. +20 dB
inline float outputGainDb (float n) { return -20.0f + 40.0f * n; }

inline juce::String textExpThr (float n)  { const float d = expThresholdDbu (n); return d < -500.f ? juce::String ("OFF") : juce::String (d, 1) + " dBu"; }
inline juce::String textExpRatio (float n) { return juce::String (expRatio (n), 1) + ":1"; }
inline juce::String textCompThr (float n) { return juce::String (compThresholdDbu (n), 1) + " dBu"; }
inline juce::String textCompRatio (float n) { const float r = compRatio (n); return r >= 100.f ? juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x9e:1")) : juce::String (r, 1) + ":1"; }
inline juce::String textAttack (float n)  { const float m = attackTime15Ms (n); return (m < 10.f ? juce::String (m, 1) : juce::String (juce::roundToInt (m))) + " ms"; }
inline juce::String textRelease (float n) { const float m = releaseTime15Ms (n); return juce::String (juce::roundToInt (m)) + " ms"; }
inline juce::String textOutGain (float n) { return juce::String (outputGainDb (n), 1) + " dB"; }

struct KnobSpec { const char* suffix; const char* name; float def; juce::String (*text) (float); };
inline const KnobSpec kKnobSpecs[7] = {
    { "exp_threshold",  "Exp/Gate Threshold", 0.0f, &textExpThr },
    { "exp_ratio",      "Exp/Gate Ratio",     0.5f, &textExpRatio },
    { "comp_threshold", "Comp Threshold",     0.5f, &textCompThr },
    { "comp_ratio",     "Comp Ratio",         0.5f, &textCompRatio },
    { "attack",         "Comp Attack",        0.5f, &textAttack },
    { "release",        "Comp Release",       0.5f, &textRelease },
    { "output_gain",    "Output Gain",        0.5f, &textOutGain },
};
} // namespace dbx
