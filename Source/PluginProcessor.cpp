#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace dbx;

static juce::String pid (int ch, const char* suffix) { return "ch" + juce::String (ch + 1) + "_" + suffix; }

juce::AudioProcessorValueTreeState::ParameterLayout DbxProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int ch = 0; ch < 2; ++ch)
    {
        const juce::String prefix = "Ch" + juce::String (ch + 1) + " ";

        for (const auto& k : kKnobSpecs)
        {
            auto text = k.text;
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { pid (ch, k.suffix), 1 },
                prefix + k.name,
                juce::NormalisableRange<float> (0.0f, 1.0f),
                k.def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                    [text] (float v, int) { return text (v); })));
        }

        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { pid (ch, "overeasy"), 1 }, prefix + "OverEasy", true));
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { pid (ch, "auto"), 1 },     prefix + "Auto",     false));
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { pid (ch, "bypass"), 1 },   prefix + "Bypass",   false));
    }

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "stereo_couple", 1 }, "Stereo Couple", false));
    return layout;
}

DbxProcessor::DbxProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DBX266XL", createLayout())
{
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int k = 0; k < 7; ++k)
            knobRaw[ch][k] = apvts.getRawParameterValue (pid (ch, kKnobSpecs[k].suffix));
        overEasyRaw[ch] = apvts.getRawParameterValue (pid (ch, "overeasy"));
        autoRaw[ch]     = apvts.getRawParameterValue (pid (ch, "auto"));
        bypassRaw[ch]   = apvts.getRawParameterValue (pid (ch, "bypass"));
    }
    coupleRaw = apvts.getRawParameterValue ("stereo_couple");
    setLatencySamples (0);          // tanpa lookahead -> latensi nol
}

bool DbxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void DbxProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& d : dsp) d.prepare (sampleRate);
    setLatencySamples (0);
}

dbx::ChannelParams DbxProcessor::readParams (int ch) const
{
    ChannelParams p;
    const float t = expThresholdDbu (knobRaw[ch][0]->load());
    p.expThrDbfs  = (t < -500.0f) ? -999.0f : t + kDbuToDbfs;
    p.expRatio    = expRatio (knobRaw[ch][1]->load());
    p.compThrDbfs = compThresholdDbu (knobRaw[ch][2]->load()) + kDbuToDbfs;
    p.compRatio   = compRatio (knobRaw[ch][3]->load());
    p.attackRate  = 15.0f / attackTime15Ms (knobRaw[ch][4]->load());     // dB per ms
    p.releaseRate = releaseDbPerS (knobRaw[ch][5]->load());              // dB per detik
    p.outGainDb   = outputGainDb (knobRaw[ch][6]->load());
    p.overEasy    = overEasyRaw[ch]->load() > 0.5f;
    p.autoMode    = autoRaw[ch]->load() > 0.5f;
    p.bypass      = bypassRaw[ch]->load() > 0.5f;
    return p;
}

void DbxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n     = buffer.getNumSamples();
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);
    if (numCh == 0 || n == 0) return;

    ChannelParams P[2];
    P[0] = readParams (0);
    if (numCh > 1) P[1] = readParams (1);

    const bool coupled = (numCh > 1) && coupleRaw->load() > 0.5f;
    const ChannelParams* Pd[2] = { &P[0], coupled ? &P[0] : &P[1] };   // parameter dinamika (gate/komp)

    for (int c = 0; c < numCh; ++c)
        dsp[c].beginBlock (P[c], *Pd[c]);

    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float blockGr[2] = { 0.0f, 0.0f };
    float lastCmp[2] = { -120.0f, -120.0f }, lastGate[2] = { -120.0f, -120.0f };

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { 0.0f, 0.0f }, rms[2] = { -120.0f, -120.0f }, pk[2] = { -120.0f, -120.0f };
        for (int c = 0; c < numCh; ++c)
        {
            x[c] = data[c][i];
            if (! std::isfinite (x[c])) x[c] = 0.0f;
            rms[c] = dsp[c].detectRms (x[c]);
            pk[c]  = dsp[c].detectPeak (x[c]);
        }

        float cmpLvl[2] = { rms[0], rms[1] }, gateLvl[2] = { pk[0], pk[1] };
        if (coupled)
        {
            cmpLvl[0] = cmpLvl[1]  = 10.0f * std::log10 (0.5f * (dsp[0].power() + dsp[1].power()) + 1.0e-12f);   // True RMS power summing
            gateLvl[0] = gateLvl[1] = std::max (pk[0], pk[1]);
        }

        for (int c = 0; c < numCh; ++c)
        {
            const float gg = dsp[c].gateStep (*Pd[c], gateLvl[c]);
            const float cg = dsp[c].compStep (*Pd[c], cmpLvl[c]);
            data[c][i] = dsp[c].apply (x[c], gg + cg, P[c]);
            blockGr[c] = std::max (blockGr[c], std::max (-cg, -gg));
            lastCmp[c] = cmpLvl[c];
            lastGate[c] = gateLvl[c];
        }
    }

    for (int c = 0; c < numCh; ++c)
    {
        auto& m = meters[c];
        m.compDiff.store (lastCmp[c] - Pd[c]->compThrDbfs, std::memory_order_relaxed);
        const bool off = Pd[c]->expThrDbfs < -500.0f || Pd[c]->expRatio < 1.001f;
        m.gateOff.store (off, std::memory_order_relaxed);
        m.overEasy.store (Pd[c]->overEasy, std::memory_order_relaxed);
        m.gateDiff.store (lastGate[c] - Pd[c]->expThrDbfs, std::memory_order_relaxed);
        if (blockGr[c] > m.grPeak.load (std::memory_order_relaxed))
            m.grPeak.store (blockGr[c], std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* DbxProcessor::createEditor() { return new DbxEditor (*this); }

void DbxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DbxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DbxProcessor(); }
