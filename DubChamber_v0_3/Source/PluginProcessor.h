#pragma once
#include <JuceHeader.h>

class DubChamberAudioProcessor : public juce::AudioProcessor
{
public:
    DubChamberAudioProcessor();
    ~DubChamberAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Dub Chamber"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 16.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;

private:
    static APVTS::ParameterLayout createParameterLayout();

    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    double currentSampleRate = 44100.0;
    juce::dsp::StateVariableTPTFilter<float> filters[2];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed, feedbackSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> cutoffSmoothed, driveSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dampingSmoothed;
    int delaySamples[2] { 18000, 20000 };
    float panicGain = 1.0f;
    bool wasPanic = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DubChamberAudioProcessor)
};
