#pragma once

#include "PluginProcessor.h"

class VocalMidBoostAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit VocalMidBoostAudioProcessorEditor (VocalMidBoostAudioProcessor&);
    ~VocalMidBoostAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    float responseDbAt (float freq, const std::array<vmb::Coeffs, 5>& stages, float outputDb, double sr) const;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& text, const juce::String& suffix);

    VocalMidBoostAudioProcessor& processor;

    Knob hpf, mud, midFreq, midGain, midQ, presence, air, output;
    juce::ToggleButton bypass { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<int> curveArea;
    vmb::Settings lastDrawn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalMidBoostAudioProcessorEditor)
};
