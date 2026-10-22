#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float maxDelaySeconds = 4.0f;

    float tapeSaturate (float x, float drive)
    {
        const float amount = 1.0f + drive * 7.0f;
        return std::tanh (x * amount) / std::tanh (amount);
    }
}

DubChamberAudioProcessor::DubChamberAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

DubChamberAudioProcessor::APVTS::ParameterLayout DubChamberAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;
    using Float = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;

    p.add (std::make_unique<Choice> ("division", "Division",
        juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/4D" }, 2));
    p.add (std::make_unique<Float> ("time", "Free Time (ms)",
        juce::NormalisableRange<float> (30.0f, 1200.0f, 1.0f), 360.0f));
    p.add (std::make_unique<Float> ("feedback", "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.94f, 0.001f), 0.46f));
    p.add (std::make_unique<Float> ("mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.26f));
    p.add (std::make_unique<Float> ("cutoff", "Filter Cutoff",
        juce::NormalisableRange<float> (180.0f, 12000.0f, 1.0f, 0.35f), 2800.0f));
    p.add (std::make_unique<Float> ("resonance", "Resonance",
        juce::NormalisableRange<float> (0.1f, 0.92f, 0.001f), 0.35f));
    p.add (std::make_unique<Float> ("drive", "Tape Heat",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.28f));
    p.add (std::make_unique<Float> ("width", "Stereo Offset",
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.001f), 0.14f));
    p.add (std::make_unique<Float> ("damping", "Repeat Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.42f));
    p.add (std::make_unique<juce::AudioParameterBool> ("sync", "Tempo Sync", true));
    p.add (std::make_unique<juce::AudioParameterBool> ("throw", "Feedback Throw", false));
    p.add (std::make_unique<juce::AudioParameterBool> ("panic", "Panic / Clear", false));
    p.add (std::make_unique<juce::AudioParameterBool> ("bypass", "Effect Bypass", false));
    return p;
}

void DubChamberAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int length = juce::roundToInt (sampleRate * maxDelaySeconds) + samplesPerBlock + 8;
    delayBuffer.setSize (2, length);
    delayBuffer.clear();
    writePosition = 0;

    for (auto& f : filters)
    {
        f.reset();
        f.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        f.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
    }

    mixSmoothed.reset (sampleRate, 0.025);
    feedbackSmoothed.reset (sampleRate, 0.035);
    cutoffSmoothed.reset (sampleRate, 0.03);
    driveSmoothed.reset (sampleRate, 0.03);
    dampingSmoothed.reset (sampleRate, 0.03);

    mixSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("mix")->load());
    feedbackSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("feedback")->load());
    cutoffSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("cutoff")->load());
    driveSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("drive")->load());
    dampingSmoothed.setCurrentAndTargetValue (parameters.getRawParameterValue ("damping")->load());
    panicGain = 1.0f;
    wasPanic = false;
}

void DubChamberAudioProcessor::releaseResources()
{
    delayBuffer.setSize (0, 0);
}

bool DubChamberAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void DubChamberAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int totalSamples = delayBuffer.getNumSamples();
    if (channels == 0 || totalSamples < 4) return;

    const auto get = [this] (const char* id) { return parameters.getRawParameterValue (id)->load(); };
    if (get ("bypass") > 0.5f) return;

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto pos = playHead->getPosition())
            if (auto hostBpm = pos->getBpm())
                bpm = juce::jlimit (30.0, 300.0, *hostBpm);

    const bool sync = get ("sync") > 0.5f;
    const int division = juce::roundToInt (get ("division"));
    const double beatMs = 60000.0 / bpm;
    const double multipliers[] = { 1.0, 0.5, 0.75, 1.0 / 3.0, 0.25, 1.5 };
    const double delayMs = sync ? beatMs * multipliers[juce::jlimit (0, 5, division)]
                                : (double) get ("time");
    const int baseDelay = juce::jlimit (1, totalSamples - 2,
        juce::roundToInt ((delayMs / 1000.0) * currentSampleRate));
    const float offset = get ("width") * 0.5f;
    delaySamples[0] = baseDelay;
    delaySamples[1] = juce::jlimit (1, totalSamples - 2,
        juce::roundToInt (baseDelay * (1.0f + offset)));

    const bool throwOn = get ("throw") > 0.5f;
    const bool panicOn = get ("panic") > 0.5f;
    if (panicOn && ! wasPanic)
    {
        delayBuffer.clear();
        for (auto& f : filters) f.reset();
        panicGain = 0.0f;
    }
    wasPanic = panicOn;

    const float targetFeedback = throwOn ? juce::jmin (0.94f, get ("feedback") + 0.22f) : get ("feedback");
    mixSmoothed.setTargetValue (get ("mix"));
    feedbackSmoothed.setTargetValue (targetFeedback);
    cutoffSmoothed.setTargetValue (get ("cutoff"));
    driveSmoothed.setTargetValue (get ("drive"));
    dampingSmoothed.setTargetValue (get ("damping"));

    for (int ch = 0; ch < channels; ++ch)
        filters[ch].setResonance (get ("resonance"));

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float mix = mixSmoothed.getNextValue();
        const float feedback = juce::jlimit (0.0f, 0.94f, feedbackSmoothed.getNextValue());
        const float cutoff = cutoffSmoothed.getNextValue();
        const float drive = driveSmoothed.getNextValue();
        const float damping = dampingSmoothed.getNextValue();

        for (int ch = 0; ch < channels; ++ch)
        {
            filters[ch].setCutoffFrequency (cutoff);
            const float dry = buffer.getSample (ch, i);
            int readPos = writePosition - delaySamples[ch];
            if (readPos < 0) readPos += totalSamples;
            const float delayed = delayBuffer.getSample (ch, readPos);
            const float filtered = filters[ch].processSample (0, delayed);
            const float damped = filtered * (1.0f - damping * 0.58f);
            const float tape = tapeSaturate (damped, drive);
            const float write = juce::jlimit (-1.25f, 1.25f, dry + tape * feedback);
            delayBuffer.setSample (ch, writePosition, write);

            if (ch == 0 && panicGain < 1.0f)
                panicGain = juce::jmin (1.0f, panicGain + 0.0008f);

            buffer.setSample (ch, i, dry * (1.0f - mix) + delayed * mix * panicGain);
        }
        writePosition = (writePosition + 1) % totalSamples;
    }
}

juce::AudioProcessorEditor* DubChamberAudioProcessor::createEditor()
{
    return new DubChamberAudioProcessorEditor (*this);
}

void DubChamberAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DubChamberAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DubChamberAudioProcessor();
}
