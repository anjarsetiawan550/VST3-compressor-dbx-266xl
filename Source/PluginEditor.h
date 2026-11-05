#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "Layout.h"

// Knob filmstrip 64 frame (frame 0 = minimum, frame 63 = maksimum)
class KnobSlider : public juce::Slider
{
public:
    explicit KnobSlider (juce::Image filmstrip) : strip (std::move (filmstrip))
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setMouseDragSensitivity (220);
        setScrollWheelEnabled (true);
    }

    void paint (juce::Graphics& g) override
    {
        const int frames = dbx::layout::kKnobFrames;
        const int fh = strip.getHeight() / frames;
        const int idx = juce::jlimit (0, frames - 1,
                                      (int) std::round (valueToProportionOfLength (getValue()) * (frames - 1)));
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (strip, 0, 0, getWidth(), getHeight(), 0, idx * fh, strip.getWidth(), fh);
    }

private:
    juce::Image strip;
};

// Tombol dua gambar (on = menyala, off = redup)
class ImageToggle : public juce::Button
{
public:
    ImageToggle (juce::Image on, juce::Image off) : juce::Button (""), imgOn (std::move (on)), imgOff (std::move (off))
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (getToggleState() ? imgOn : imgOff, getLocalBounds().toFloat());
    }

private:
    juce::Image imgOn, imgOff;
};

class DbxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DbxEditor (DbxProcessor&);
    ~DbxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::Rectangle<int> scaled (int x, int y, int w, int h) const;

    DbxProcessor& proc;
    juce::Image bg, bgScaled;
    std::vector<juce::Image> bgLevels;      // piramida (dibagi dua) agar hasil kecil tetap tajam
    juce::Image ledOn[3], ledOff[3];       // 0 red, 1 green, 2 yellow

    std::array<std::unique_ptr<KnobSlider>, 14> knobs;
    std::array<std::unique_ptr<ImageToggle>, 7> buttons;
    std::array<bool, 26> ledState {};
    float grDisp[2] { 0.0f, 0.0f };

    // attachment dideklarasikan terakhir -> dihancurkan lebih dulu
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DbxEditor)
};
