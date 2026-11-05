#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
juce::Image loadImage (const juce::String& resourceName)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), size))
        return juce::ImageCache::getFromMemory (data, size);
    jassertfalse;
    return {};
}

int colourIndex (const juce::String& c) { return c == "red" ? 0 : (c == "green" ? 1 : 2); }
const char* const kColours[3] = { "red", "green", "yellow" };
}

DbxEditor::DbxEditor (DbxProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    using namespace dbx::layout;

    bg = loadImage ("background_png");
    bgLevels.push_back (bg);
    while (bgLevels.back().getWidth() / 2 >= 600)
        bgLevels.push_back (bgLevels.back().rescaled (bgLevels.back().getWidth() / 2,
                                                      bgLevels.back().getHeight() / 2,
                                                      juce::Graphics::highResamplingQuality));
    const juce::Image knobStrip = loadImage ("knob_png");
    for (int i = 0; i < 3; ++i)
    {
        ledOn[i]  = loadImage (juce::String ("led_")  + kColours[i] + "_on_png");
        ledOff[i] = loadImage (juce::String ("led_")  + kColours[i] + "_off_png");
    }

    for (int i = 0; i < 14; ++i)
    {
        auto k = std::make_unique<KnobSlider> (knobStrip);
        addAndMakeVisible (*k);
        knobAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kKnobs[i].id, *k));
        k->setPopupDisplayEnabled (true, true, this);
        knobs[(size_t) i] = std::move (k);
    }

    for (int i = 0; i < 7; ++i)
    {
        const juce::String col (kButtons[i].color);
        auto b = std::make_unique<ImageToggle> (loadImage ("btn_" + col + "_on_png"),
                                                loadImage ("btn_" + col + "_off_png"));
        addAndMakeVisible (*b);
        btnAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kButtons[i].paramId, *b));
        buttons[(size_t) i] = std::move (b);
    }

    setResizable (true, true);
    setResizeLimits (680, 62, 4082, 368);
    getConstrainer()->setFixedAspectRatio ((double) kBgW / (double) kBgH);
    setSize (1360, 123);

    startTimerHz (30);
}

DbxEditor::~DbxEditor() { stopTimer(); }

juce::Rectangle<int> DbxEditor::scaled (int x, int y, int w, int h) const
{
    const float s = (float) getWidth() / (float) dbx::layout::kBgW;
    return juce::Rectangle<float> (x * s, y * s, w * s, h * s).getSmallestIntegerContainer();
}

void DbxEditor::resized()
{
    using namespace dbx::layout;
    if (getWidth() <= 0 || getHeight() <= 0) return;

    const juce::Image* src = &bgLevels.front();
    for (auto& lv : bgLevels)
        if (lv.getWidth() >= getWidth()) src = &lv;
    bgScaled = src->rescaled (getWidth(), getHeight(), juce::Graphics::highResamplingQuality);

    for (int i = 0; i < 14; ++i)
        knobs[(size_t) i]->setBounds (scaled (kKnobs[i].x, kKnobs[i].y, kKnobs[i].w, kKnobs[i].h));
    for (int i = 0; i < 7; ++i)
        buttons[(size_t) i]->setBounds (scaled (kButtons[i].x, kButtons[i].y, kButtons[i].w, kButtons[i].h));
}

void DbxEditor::paint (juce::Graphics& g)
{
    using namespace dbx::layout;
    g.drawImageAt (bgScaled, 0, 0);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

    for (int i = 0; i < 26; ++i)
    {
        const int ci = colourIndex (kLeds[i].color);
        g.drawImage (ledState[(size_t) i] ? ledOn[ci] : ledOff[ci],
                     scaled (kLeds[i].x, kLeds[i].y, kLeds[i].w, kLeds[i].h).toFloat());
    }
}

void DbxEditor::timerCallback()
{
    using namespace dbx::layout;
    constexpr float dt = 1.0f / 30.0f;
    static const float grThr[8] = { 30.f, 25.f, 20.f, 15.f, 10.f, 6.f, 3.f, 1.f };

    std::array<bool, 26> ns {};
    for (int c = 0; c < 2; ++c)
    {
        auto& m = proc.meters[c];
        const float gr = m.grPeak.exchange (0.0f);
        grDisp[c] = std::max (gr, grDisp[c] - 60.0f * dt);

        const bool  gateOff = m.gateOff.load();
        const float gd = m.gateDiff.load();
        const float cd = m.compDiff.load();
        const size_t b = (size_t) c * 13;

        ns[b + 0] = ! gateOff && gd < 0.0f;          // "-" merah : gate menutup / expander aktif
        ns[b + 1] = gateOff || gd >= 0.0f;           // "+" hijau  : di atas threshold gate
        // LED threshold kompresor:
        //  OverEasy nyala : hijau = di bawah knee, kuning = di dalam knee (+-7 dB), merah = di atas knee
        //  OverEasy mati  : hanya hijau (di bawah threshold) dan merah (di atas); kuning tidak aktif
        if (m.overEasy.load())
        {
            ns[b + 2] = cd < -7.0f;
            ns[b + 3] = std::abs (cd) <= 7.0f;
            ns[b + 4] = cd > 7.0f;
        }
        else
        {
            ns[b + 2] = cd <= 0.0f;
            ns[b + 3] = false;
            ns[b + 4] = cd > 0.0f;
        }
        for (size_t i = 0; i < 8; ++i)
            ns[b + 5 + i] = grDisp[c] >= grThr[i];
    }

    for (size_t i = 0; i < 26; ++i)
        if (ns[i] != ledState[i])
        {
            ledState[i] = ns[i];
            repaint (scaled (kLeds[i].x, kLeds[i].y, kLeds[i].w, kLeds[i].h));
        }
}
