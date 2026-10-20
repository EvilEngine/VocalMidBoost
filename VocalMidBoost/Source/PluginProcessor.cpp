#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace vmb
{
    static float clampFreq (float f, double sr)
    {
        return juce::jlimit (10.0f, (float) (sr * 0.45), f);
    }

    Settings getSettings (juce::AudioProcessorValueTreeState& apvts)
    {
        Settings s;
        s.hpf      = apvts.getRawParameterValue (id::hpf)->load();
        s.mud      = apvts.getRawParameterValue (id::mud)->load();
        s.midFreq  = apvts.getRawParameterValue (id::midFreq)->load();
        s.midGain  = apvts.getRawParameterValue (id::midGain)->load();
        s.midQ     = apvts.getRawParameterValue (id::midQ)->load();
        s.presence = apvts.getRawParameterValue (id::presence)->load();
        s.air      = apvts.getRawParameterValue (id::air)->load();
        s.output   = apvts.getRawParameterValue (id::output)->load();
        s.bypass   = apvts.getRawParameterValue (id::bypass)->load() > 0.5f;
        return s;
    }

    Coeffs makeHpf (const Settings& s, double sr)
    {
        return juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, clampFreq (s.hpf, sr), 0.7071f);
    }

    Coeffs makeMud (const Settings& s, double sr)
    {
        return juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sr, clampFreq (mudFreq, sr), 0.9f, juce::Decibels::decibelsToGain (s.mud));
    }

    Coeffs makeMid (const Settings& s, double sr)
    {
        return juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sr, clampFreq (s.midFreq, sr), s.midQ, juce::Decibels::decibelsToGain (s.midGain));
    }

    Coeffs makePresence (const Settings& s, double sr)
    {
        return juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sr, clampFreq (presenceFreq, sr), 0.7071f, juce::Decibels::decibelsToGain (s.presence));
    }

    Coeffs makeAir (const Settings& s, double sr)
    {
        return juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sr, clampFreq (airFreq, sr), 0.7071f, juce::Decibels::decibelsToGain (s.air));
    }
}

//==============================================================================
VocalMidBoostAudioProcessor::VocalMidBoostAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalMidBoostAudioProcessor::createParameterLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;

    AudioProcessorValueTreeState::ParameterLayout layout;

    auto hzText = [] (float v, int) { return String (roundToInt (v)) + " Hz"; };
    auto dbText = [] (float v, int) { return String (v, 1) + " dB"; };

    layout.add (std::make_unique<P> (ParameterID { vmb::id::hpf, 1 }, "Low Cut",
                                     NormalisableRange<float> (20.0f, 300.0f, 1.0f, 0.5f), 80.0f,
                                     AudioParameterFloatAttributes().withStringFromValueFunction (hzText)));

    layout.add (std::make_unique<P> (ParameterID { vmb::id::mud, 1 }, "Mud Cut",
                                     NormalisableRange<float> (-9.0f, 0.0f, 0.1f), 0.0f,
                                     AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<P> (ParameterID { vmb::id::midFreq, 1 }, "Mid Freq",
                                     NormalisableRange<float> (500.0f, 6000.0f, 1.0f, 0.4f), 2800.0f,
                                     AudioParameterFloatAttributes().withStringFromValueFunction (hzText)));

    layout.add (std::make_unique<P> (ParameterID { vmb::id::midGain, 1 }, "Mid Boost",
                                     NormalisableRange<float> (0.0f, 12.0f, 0.1f), 3.0f,
                                     AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<P> (ParameterID { vmb::id::midQ, 1 }, "Mid Width (Q)",
                                     NormalisableRange<float> (0.3f, 4.0f, 0.01f, 0.5f), 1.0f));

    layout.add (std::make_unique<P> (ParameterID { vmb::id::presence, 1 }, "Presence",
                                     NormalisableRange<float> (0.0f, 8.0f, 0.1f), 0.0f,
                                     AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<P> (ParameterID { vmb::id::air, 1 }, "Air",
                                     NormalisableRange<float> (0.0f, 8.0f, 0.1f), 0.0f,
                                     AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<P> (ParameterID { vmb::id::output, 1 }, "Output",
                                     NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                                     AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { vmb::id::bypass, 1 }, "Bypass", false));

    return layout;
}

//==============================================================================
bool VocalMidBoostAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void VocalMidBoostAudioProcessor::prepareToPlay (double sampleRate, int)
{
    const auto s = vmb::getSettings (apvts);

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        filters[0][ch].coefficients = vmb::makeHpf (s, sampleRate);
        filters[1][ch].coefficients = vmb::makeMud (s, sampleRate);
        filters[2][ch].coefficients = vmb::makeMid (s, sampleRate);
        filters[3][ch].coefficients = vmb::makePresence (s, sampleRate);
        filters[4][ch].coefficients = vmb::makeAir (s, sampleRate);

        // Run a block of silence through each filter so its internal state is allocated
        // here, not on the audio thread.
        juce::AudioBuffer<float> warmup (1, 64);
        warmup.clear();
        juce::dsp::AudioBlock<float> warmupBlock (warmup);
        juce::dsp::ProcessContextReplacing<float> warmupCtx (warmupBlock);

        for (int st = 0; st < numStages; ++st)
        {
            filters[st][ch].process (warmupCtx);
            filters[st][ch].reset();
        }
    }

    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (s.output));

    lastSettings = s;
}

void VocalMidBoostAudioProcessor::updateFilters (const vmb::Settings& s)
{
    const double sr = getSampleRate();

    // Build once, copy into both channels' filters.
    const vmb::Coeffs c[numStages] = { vmb::makeHpf (s, sr), vmb::makeMud (s, sr), vmb::makeMid (s, sr),
                                       vmb::makePresence (s, sr), vmb::makeAir (s, sr) };

    for (int ch = 0; ch < maxChannels; ++ch)
        for (int st = 0; st < numStages; ++st)
            *filters[st][ch].coefficients = *c[st];
}

void VocalMidBoostAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto s = vmb::getSettings (apvts);

    if (s.bypass)
        return;

    if (! (s == lastSettings))
    {
        updateFilters (s);
        lastSettings = s;
    }

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.output));

    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);
    const int numSamples = buffer.getNumSamples();

    juce::dsp::AudioBlock<float> block (buffer);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto channelBlock = block.getSingleChannelBlock ((size_t) ch);
        juce::dsp::ProcessContextReplacing<float> ctx (channelBlock);

        for (int st = 0; st < numStages; ++st)
            filters[st][ch].process (ctx);
    }

    // Output gain (smoothed)
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }
}

//==============================================================================
juce::AudioProcessorEditor* VocalMidBoostAudioProcessor::createEditor()
{
    return new VocalMidBoostAudioProcessorEditor (*this);
}

void VocalMidBoostAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalMidBoostAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalMidBoostAudioProcessor();
}
