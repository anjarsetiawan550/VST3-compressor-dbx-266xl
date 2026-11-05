#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "DbxEngine.h"
#include "ParamMap.h"

class DbxProcessor : public juce::AudioProcessor
{
public:
    DbxProcessor();
    ~DbxProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // data meter untuk LED (dibaca oleh editor)
    struct ChMeter
    {
        std::atomic<float> grPeak  { 0.0f };    // gain reduction (dB, positif), puncak sejak dibaca
        std::atomic<float> compDiff { -60.0f }; // level - threshold kompresor (dB)
        std::atomic<float> gateDiff { 60.0f };  // level - threshold gate (dB)
        std::atomic<bool>  gateOff { true };
        std::atomic<bool>  overEasy { true };   // status tombol OverEasy (untuk LED)
    };
    ChMeter meters[2];

private:
    dbx::ChannelParams readParams (int ch) const;

    std::atomic<float>* knobRaw[2][7] {};
    std::atomic<float>* overEasyRaw[2] {};
    std::atomic<float>* autoRaw[2] {};
    std::atomic<float>* bypassRaw[2] {};
    std::atomic<float>* coupleRaw = nullptr;

    dbx::ChannelDsp dsp[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DbxProcessor)
};
