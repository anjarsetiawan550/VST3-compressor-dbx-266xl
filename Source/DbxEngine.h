// DSP inti ala dbx 266XL: Expander/Gate + Compressor (OverEasy), per channel.
//  - Kompresor: detektor True RMS, topologi feed-forward, gain computer di domain dB,
//    attack/release dengan laju tetap (dB per waktu) seperti dbx klasik, OverEasy soft-knee.
//  - Gate: expander 1:1..4:1, attack < 100 us, release bergantung program.
// Tanpa lookahead dan tanpa oversampling -> latensi 0 sample.
#pragma once
#include <algorithm>
#include <cmath>

namespace dbx
{
// Referensi level: 0 dBu = -20 dBFS  (+20 dBu = 0 dBFS)
constexpr float kDbuToDbfs = -20.0f;
constexpr float kOverEasyWidthDb = 14.0f;     // lebar knee OverEasy (dB)
constexpr float kAutoAttackDbPerMs = 10.0f / 15.0f;   // Auto: 15 ms per 10 dB (dbx klasik)
constexpr float kAutoReleaseDbPerS = 120.0f;          // Auto: 120 dB/detik

struct ChannelParams
{
    float expThrDbfs   = -999.0f;  // < -500 = OFF
    float expRatio     = 1.0f;
    float compThrDbfs  = -30.0f;
    float compRatio    = 2.0f;     // >= 100 = infinity
    float attackRate   = 0.5f;     // dB gain reduction per ms
    float releaseRate  = 100.0f;   // dB per detik
    float outGainDb    = 0.0f;
    bool  overEasy     = true;
    bool  autoMode     = false;
    bool  bypass       = false;
};

class ChannelDsp
{
public:
    void prepare (double sampleRate)
    {
        fs         = (float) sampleRate;
        rmsA       = decay (8.0f);       // jendela True RMS
        peakA      = decay (25.0f);
        gateOpenA  = decay (0.05f);      // attack gate < 100 us
        sustainA   = decay (300.0f);
        smoothA    = decay (8.0f);
        reset();
    }

    void reset()
    {
        rmsPow = peakEnv = gateGr = compGr = outSm = bypSm = sustain = 0.0f;
        first = true;
    }

    void beginBlock (const ChannelParams& own, const ChannelParams& dyn)
    {
        if (first)
        {
            outSm = own.outGainDb;
            bypSm = own.bypass ? 1.0f : 0.0f;
            first = false;
        }
        const float atk = dyn.autoMode ? kAutoAttackDbPerMs : dyn.attackRate;   // dB/ms
        const float rel = dyn.autoMode ? kAutoReleaseDbPerS : dyn.releaseRate;  // dB/s
        atkStep = atk * 1000.0f / fs;       // dB per sampel
        relStep = rel / fs;
    }

    float power() const { return rmsPow; }

    // level True RMS (dBFS)
    float detectRms (float x)
    {
        rmsPow = rmsA * rmsPow + (1.0f - rmsA) * x * x;
        return 10.0f * std::log10 (rmsPow + 1.0e-12f);
    }

    // level puncak yang diselaraskan ke skala RMS (dBFS), dipakai gate (respons sangat cepat)
    float detectPeak (float x)
    {
        peakEnv = std::max (std::fabs (x), peakEnv * peakA);
        return 20.0f * std::log10 (peakEnv + 1.0e-6f) - 3.0f;
    }

    // Expander/Gate: kembalikan gain (dB, <= 0). Kedalaman maksimum 80 dB (> 60 dB).
    float gateStep (const ChannelParams& p, float levelDb)
    {
        float target = 0.0f;
        const bool active = p.expThrDbfs > -500.0f && p.expRatio > 1.001f;
        const bool above  = levelDb >= p.expThrDbfs;
        if (active && ! above)
            target = std::max (-80.0f, (levelDb - p.expThrDbfs) * (p.expRatio - 1.0f));

        // release bergantung program: sinyal singkat -> menutup cepat, sinyal panjang -> lebih lambat
        sustain = sustainA * sustain + (1.0f - sustainA) * (above ? 1.0f : 0.0f);

        if (target > gateGr)
            gateGr = gateOpenA * gateGr + (1.0f - gateOpenA) * target;
        else
        {
            const float relMs = 40.0f + 360.0f * sustain;
            const float a = decay (relMs);
            gateGr = a * gateGr + (1.0f - a) * target;
        }
        return gateGr;
    }

    // Kompresor: kembalikan gain reduction (dB, <= 0)
    float compStep (const ChannelParams& p, float levelDb)
    {
        const float over  = levelDb - p.compThrDbfs;
        const float slope = (p.compRatio >= 100.0f ? 0.0f : 1.0f / p.compRatio) - 1.0f;
        float target = 0.0f;

        if (p.overEasy)
        {
            constexpr float W = kOverEasyWidthDb;
            if (2.0f * over >= W)        target = slope * over;
            else if (2.0f * over > -W)   { const float t = over + 0.5f * W; target = slope * t * t / (2.0f * W); }
        }
        else if (over > 0.0f)
        {
            target = slope * over;
        }

        // laju tetap dalam dB (seperti VCA/RMS dbx klasik)
        const float delta = target - compGr;
        if (delta < 0.0f) compGr += std::max (delta, -atkStep);   // gain reduction bertambah (attack)
        else              compGr += std::min (delta,  relStep);   // gain reduction berkurang (release)
        return compGr;
    }

    float apply (float x, float gainDb, const ChannelParams& p)
    {
        outSm += (1.0f - smoothA) * (p.outGainDb - outSm);
        bypSm += (1.0f - smoothA) * ((p.bypass ? 1.0f : 0.0f) - bypSm);

        const float total = std::min (40.0f, std::max (-120.0f, gainDb + outSm));
        const float wet   = x * std::exp (total * 0.11512925f);
        return wet + bypSm * (x - wet);
    }

private:
    float decay (float ms) const { return std::exp (-1.0f / (0.001f * std::max (0.01f, ms) * fs)); }

    float fs = 48000.0f;
    float rmsA = 0, peakA = 0, gateOpenA = 0, sustainA = 0, smoothA = 0, atkStep = 0, relStep = 0;
    float rmsPow = 0, peakEnv = 0, gateGr = 0, compGr = 0, outSm = 0, bypSm = 0, sustain = 0;
    bool first = true;
};
} // namespace dbx
