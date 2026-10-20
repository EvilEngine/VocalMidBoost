#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace vmb
{
    // Parameter IDs
    namespace id
    {
        inline constexpr const char* hpf      = "hpf";
        inline constexpr const char* mud      = "mud";
        inline constexpr const char* midFreq  = "midFreq";
        inline constexpr const char* midGain  = "midGain";
        inline constexpr const char* midQ     = "midQ";
        inline constexpr const char* presence = "presence";
        inline constexpr const char* air      = "air";
        inline constexpr const char* output   = "output";
        inline constexpr const char* bypass   = "bypass";
    }

    struct Settings
    {
        float hpf = 80.0f;        // Hz
        float mud = 0.0f;         // dB (<= 0), cut at ~300 Hz
        float midFreq = 2800.0f;  // Hz
        float midGain = 3.0f;     // dB (>= 0)
        float midQ = 1.0f;
        float presence = 0.0f;    // dB, high shelf @ 5.5 kHz
        float air = 0.0f;         // dB, high shelf @ 12 kHz
        float output = 0.0f;      // dB
        bool  bypass = false;

        bool operator== (const Settings& o) const noexcept
        {
            return hpf == o.hpf && mud == o.mud && midFreq == o.midFreq && midGain == o.midGain
                && midQ == o.midQ && presence == o.presence && air == o.air;
        }
    };

    using Coeffs = juce::dsp::IIR::Coefficients<float>::Ptr;

    constexpr float mudFreq = 300.0f;
    constexpr float presenceFreq = 5500.0f;
    constexpr float airFreq = 12000.0f;

    Settings getSettings (juce::AudioProcessorValueTreeState& apvts);

    Coeffs makeHpf      (const Settings&, double sampleRate);
    Coeffs makeMud      (const Settings&, double sampleRate);
    Coeffs makeMid      (const Settings&, double sampleRate);
    Coeffs makePresence (const Settings&, double sampleRate);
    Coeffs makeAir      (const Settings&, double sampleRate);
}

class VocalMidBoostAudioProcessor : public juce::AudioProcessor
{
public:
    VocalMidBoostAudioProcessor();
    ~VocalMidBoostAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
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

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int numStages = 5;   // HPF, Mud, Mid, Presence, Air
    static constexpr int maxChannels = 2;

    using Filter = juce::dsp::IIR::Filter<float>;
    Filter filters[numStages][maxChannels];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain;
    vmb::Settings lastSettings;

    void updateFilters (const vmb::Settings&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalMidBoostAudioProcessor)
};
