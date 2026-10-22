#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DubChamberLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

class DubChamberAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit DubChamberAudioProcessorEditor (DubChamberAudioProcessor&);
    ~DubChamberAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    DubChamberAudioProcessor& processor;
    DubChamberLookAndFeel customLook;
    juce::Label title, subtitle, statusLabel, delayLabel, filterLabel, colourLabel;
    juce::Slider feedback, mix, cutoff, resonance, drive, width, damping, time;
    juce::ComboBox division;
    juce::ToggleButton sync, panicButton;
    juce::TextButton throwButton, rootsPreset, skankPreset, runawayPreset;
    juce::Label labels[8];
    std::unique_ptr<SliderAttachment> feedbackA, mixA, cutoffA, resonanceA, driveA, widthA, dampingA, timeA;
    std::unique_ptr<ButtonAttachment> syncA, panicA;
    std::unique_ptr<ComboAttachment> divisionA;

    void styleSlider (juce::Slider&, int labelIndex, const juce::String&);
    void applyPreset (int presetIndex);
    void setMomentaryThrowParameter();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubChamberAudioProcessorEditor)
};
