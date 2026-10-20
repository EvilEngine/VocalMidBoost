#include "PluginEditor.h"

namespace
{
    const juce::Colour bg        { 0xff15171c };
    const juce::Colour panel     { 0xff1e2128 };
    const juce::Colour accent    { 0xffffb347 };
    const juce::Colour textCol   { 0xffd8dbe2 };
    const juce::Colour gridCol   { 0xff2c303a };
}

VocalMidBoostAudioProcessorEditor::VocalMidBoostAudioProcessorEditor (VocalMidBoostAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    addKnob (hpf,      vmb::id::hpf,      "Low Cut",   " Hz");
    addKnob (mud,      vmb::id::mud,      "Mud Cut",   " dB");
    addKnob (midFreq,  vmb::id::midFreq,  "Mid Freq",  " Hz");
    addKnob (midGain,  vmb::id::midGain,  "Mid Boost", " dB");
    addKnob (midQ,     vmb::id::midQ,     "Width",     "");
    addKnob (presence, vmb::id::presence, "Presence",  " dB");
    addKnob (air,      vmb::id::air,      "Air",       " dB");
    addKnob (output,   vmb::id::output,   "Output",    " dB");

    // Highlight the main control
    midGain.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);

    bypass.setColour (juce::ToggleButton::textColourId, textCol);
    bypass.setColour (juce::ToggleButton::tickColourId, accent);
    addAndMakeVisible (bypass);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, vmb::id::bypass, bypass);

    setSize (760, 460);
    startTimerHz (30);
}

void VocalMidBoostAudioProcessorEditor::addKnob (Knob& k, const juce::String& paramId,
                                                 const juce::String& text, const juce::String& suffix)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    k.slider.setTextValueSuffix (suffix);
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, accent.withAlpha (0.7f));
    k.slider.setColour (juce::Slider::rotarySliderOutlineColourId, gridCol);
    k.slider.setColour (juce::Slider::thumbColourId, textCol);
    k.slider.setColour (juce::Slider::textBoxTextColourId, textCol);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setDoubleClickReturnValue (true, processor.apvts.getParameter (paramId)->convertFrom0to1 (
                                                  processor.apvts.getParameter (paramId)->getDefaultValue()));
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, textCol);
    k.label.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, k.slider);
}

float VocalMidBoostAudioProcessorEditor::responseDbAt (float freq, const std::array<vmb::Coeffs, 5>& stages,
                                                       float outputDb, double sr) const
{
    double mag = 1.0;

    for (const auto& c : stages)
        mag *= c->getMagnitudeForFrequency ((double) freq, sr);

    return juce::Decibels::gainToDecibels ((float) mag) + outputDb;
}

void VocalMidBoostAudioProcessorEditor::timerCallback()
{
    const auto s = vmb::getSettings (processor.apvts);

    if (! (s == lastDrawn) || s.output != lastDrawn.output || s.bypass != lastDrawn.bypass)
    {
        lastDrawn = s;
        repaint (curveArea);
    }
}

void VocalMidBoostAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    // Title
    g.setColour (textCol);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("VOCAL MID BOOST", 20, 10, 300, 30, juce::Justification::centredLeft);

    // Curve panel
    g.setColour (panel);
    g.fillRoundedRectangle (curveArea.toFloat(), 8.0f);

    const auto area = curveArea.reduced (12, 10).toFloat();
    const float minDb = -15.0f, maxDb = 15.0f;
    const float minF = 20.0f, maxF = 20000.0f;

    auto xForFreq = [&] (float f)
    {
        return area.getX() + area.getWidth() * (std::log (f / minF) / std::log (maxF / minF));
    };
    auto yForDb = [&] (float db)
    {
        return juce::jmap (db, minDb, maxDb, area.getBottom(), area.getY());
    };

    // Grid
    g.setFont (juce::FontOptions (11.0f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = xForFreq (f);
        g.setColour (gridCol);
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
        g.setColour (textCol.withAlpha (0.5f));
        g.drawText (f >= 1000.0f ? juce::String (f / 1000.0f, 0) + "k" : juce::String ((int) f),
                    (int) x - 16, (int) area.getBottom() - 14, 32, 12, juce::Justification::centred);
    }
    for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
    {
        const float y = yForDb (db);
        g.setColour (db == 0.0f ? textCol.withAlpha (0.35f) : gridCol);
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
        g.setColour (textCol.withAlpha (0.5f));
        g.drawText (juce::String ((int) db), (int) area.getX() + 2, (int) y - 12, 28, 12,
                    juce::Justification::centredLeft);
    }

    // Response curve
    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
    const auto s = vmb::getSettings (processor.apvts);

    const std::array<vmb::Coeffs, 5> stages { vmb::makeHpf (s, sr), vmb::makeMud (s, sr), vmb::makeMid (s, sr),
                                              vmb::makePresence (s, sr), vmb::makeAir (s, sr) };

    juce::Path curve;
    const int steps = (int) area.getWidth();
    for (int i = 0; i <= steps; ++i)
    {
        const float f = minF * std::pow (maxF / minF, (float) i / (float) steps);
        const float db = s.bypass ? 0.0f : juce::jlimit (minDb, maxDb, responseDbAt (f, stages, s.output, sr));
        const float x = area.getX() + (float) i;
        const float y = yForDb (db);

        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }

    juce::Path fill (curve);
    fill.lineTo (area.getRight(), yForDb (0.0f));
    fill.lineTo (area.getX(), yForDb (0.0f));
    fill.closeSubPath();

    g.setColour (accent.withAlpha (s.bypass ? 0.05f : 0.18f));
    g.fillPath (fill);
    g.setColour (s.bypass ? textCol.withAlpha (0.4f) : accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f));
}

void VocalMidBoostAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);

    auto top = r.removeFromTop (30);
    bypass.setBounds (top.removeFromRight (100));

    curveArea = r.removeFromTop (230);
    r.removeFromTop (10);

    Knob* knobs[] = { &hpf, &mud, &midFreq, &midGain, &midQ, &presence, &air, &output };
    const int w = r.getWidth() / 8;

    for (auto* k : knobs)
    {
        auto cell = r.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell);
    }
}
