#include "PluginEditor.h"

namespace
{
    const juce::Colour ink       (0xff111a17);
    const juce::Colour panel     (0xff1b2923);
    const juce::Colour panelHi   (0xff24372e);
    const juce::Colour brass     (0xffd7ad63);
    const juce::Colour cream     (0xfff3e7cb);
    const juce::Colour muted     (0xffa9b7a9);
    const juce::Colour red       (0xffdb765e);
    const juce::Colour green     (0xff8fbd86);
}

void DubChamberLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float pos, float start, float end, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (9.0f);
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 14.0f;
    auto knob = bounds.withSizeKeepingCentre (diameter, diameter);
    const auto centre = knob.getCentre();
    const float radius = diameter * 0.5f;
    const float angle = start + pos * (end - start);

    g.setColour (juce::Colour (0x55000000));
    g.fillEllipse (knob.translated (0.0f, 4.0f));
    g.setColour (juce::Colour (0xff101713));
    g.fillEllipse (knob);
    g.setColour (juce::Colour (0xff43564a));
    g.drawEllipse (knob, 1.4f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius - 5.0f, radius - 5.0f, 0.0f, start, end, true);
    g.setColour (juce::Colour (0xff405148));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius - 5.0f, radius - 5.0f, 0.0f, start, angle, true);
    g.setColour (slider.isMouseOverOrDragging() ? cream : brass);
    g.strokePath (valueArc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.7f, -radius + 10.0f, 3.4f, radius * 0.42f, 1.6f);
    g.setColour (cream);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    g.setColour (panelHi);
    g.fillEllipse (juce::Rectangle<float> (centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f));
}

DubChamberAudioProcessorEditor::DubChamberAudioProcessorEditor (DubChamberAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (760, 600);
    setResizable (true, true);
    setResizeLimits (700, 560, 900, 720);

    setLookAndFeel (&customLook);

    title.setText ("DUB CHAMBER", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, cream);
    addAndMakeVisible (title);

    subtitle.setText ("ROOTS AUDIO LAB     /     ANALOG-INSPIRED DUB SYSTEM", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    subtitle.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (subtitle);

    statusLabel.setText ("STEREO • TAPE FEEDBACK • TEMPO LOCK", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    statusLabel.setColour (juce::Label::textColourId, green);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel);

    delayLabel.setText ("01   /   DELAY ENGINE", juce::dontSendNotification);
    filterLabel.setText ("02   /   TONE & MOVEMENT", juce::dontSendNotification);
    colourLabel.setText ("PERFORMANCE DECK", juce::dontSendNotification);
    for (auto* l : { &delayLabel, &filterLabel, &colourLabel })
    {
        l->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        l->setColour (juce::Label::textColourId, brass);
        addAndMakeVisible (*l);
    }

    styleSlider (feedback, 0, "FEEDBACK");
    styleSlider (mix, 1, "WET / DRY");
    styleSlider (cutoff, 2, "FILTER CUTOFF");
    styleSlider (resonance, 3, "RESONANCE");
    styleSlider (drive, 4, "TAPE HEAT");
    styleSlider (width, 5, "STEREO OFFSET");
    styleSlider (damping, 6, "REPEAT DAMPING");
    styleSlider (time, 7, "FREE TIME");

    division.addItemList (juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/4D" }, 1);
    division.setColour (juce::ComboBox::backgroundColourId, panelHi);
    division.setColour (juce::ComboBox::textColourId, cream);
    division.setColour (juce::ComboBox::outlineColourId, brass.withAlpha (0.6f));
    addAndMakeVisible (division);
    divisionA = std::make_unique<ComboAttachment> (processor.parameters, "division", division);

    sync.setButtonText ("TEMPO SYNC");
    sync.setColour (juce::ToggleButton::textColourId, cream);
    sync.setColour (juce::ToggleButton::tickColourId, green);
    addAndMakeVisible (sync);
    syncA = std::make_unique<ButtonAttachment> (processor.parameters, "sync", sync);

    throwButton.setButtonText ("HOLD TO THROW  ↗");
    throwButton.setClickingTogglesState (false);
    throwButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff74432f));
    throwButton.setColour (juce::TextButton::buttonOnColourId, red);
    throwButton.setColour (juce::TextButton::textColourOffId, cream);
    throwButton.setColour (juce::TextButton::textColourOnId, cream);
    throwButton.onStateChange = [this] { setMomentaryThrowParameter(); };
    addAndMakeVisible (throwButton);

    panicButton.setButtonText ("PANIC / CLEAR");
    panicButton.setColour (juce::ToggleButton::textColourId, red);
    panicButton.setColour (juce::ToggleButton::tickColourId, red);
    addAndMakeVisible (panicButton);
    panicA = std::make_unique<ButtonAttachment> (processor.parameters, "panic", panicButton);

    auto stylePreset = [this] (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId, panelHi);
        b.setColour (juce::TextButton::buttonOnColourId, brass);
        b.setColour (juce::TextButton::textColourOffId, cream);
        b.setColour (juce::TextButton::textColourOnId, ink);
        addAndMakeVisible (b);
    };
    rootsPreset.setButtonText ("ROOTS TAPE");
    skankPreset.setButtonText ("FILTERED SKANK");
    runawayPreset.setButtonText ("RUNAWAY DUB");
    stylePreset (rootsPreset);
    stylePreset (skankPreset);
    stylePreset (runawayPreset);
    rootsPreset.onClick = [this] { applyPreset (0); };
    skankPreset.onClick = [this] { applyPreset (1); };
    runawayPreset.onClick = [this] { applyPreset (2); };

    feedbackA = std::make_unique<SliderAttachment> (processor.parameters, "feedback", feedback);
    mixA = std::make_unique<SliderAttachment> (processor.parameters, "mix", mix);
    cutoffA = std::make_unique<SliderAttachment> (processor.parameters, "cutoff", cutoff);
    resonanceA = std::make_unique<SliderAttachment> (processor.parameters, "resonance", resonance);
    driveA = std::make_unique<SliderAttachment> (processor.parameters, "drive", drive);
    widthA = std::make_unique<SliderAttachment> (processor.parameters, "width", width);
    dampingA = std::make_unique<SliderAttachment> (processor.parameters, "damping", damping);
    timeA = std::make_unique<SliderAttachment> (processor.parameters, "time", time);
}

DubChamberAudioProcessorEditor::~DubChamberAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void DubChamberAudioProcessorEditor::styleSlider (juce::Slider& s, int labelIndex, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
    s.setColour (juce::Slider::textBoxTextColourId, cream);
    s.setColour (juce::Slider::textBoxBackgroundColourId, panelHi);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);

    labels[labelIndex].setText (name, juce::dontSendNotification);
    labels[labelIndex].setJustificationType (juce::Justification::centred);
    labels[labelIndex].setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    labels[labelIndex].setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (labels[labelIndex]);
}

void DubChamberAudioProcessorEditor::setMomentaryThrowParameter()
{
    if (auto* parameter = processor.parameters.getParameter ("throw"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (throwButton.isDown() ? 1.0f : 0.0f);
        parameter->endChangeGesture();
    }
}

void DubChamberAudioProcessorEditor::applyPreset (int index)
{
    auto set = [this] (const char* id, float value)
    {
        if (auto* p = processor.parameters.getParameter (id))
        {
            p->beginChangeGesture();
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                ranged->setValueNotifyingHost (ranged->convertTo0to1 (value));
            p->endChangeGesture();
        }
    };

    // Presets are intentionally starting points; all controls remain editable.
    if (index == 0) // warm, rounded roots echoes
    {
        set ("feedback", 0.42f); set ("mix", 0.23f); set ("cutoff", 2600.0f);
        set ("resonance", 0.22f); set ("drive", 0.24f); set ("width", 0.08f); set ("damping", 0.48f);
    }
    else if (index == 1) // skank-friendly, darker rhythmic echo
    {
        set ("feedback", 0.48f); set ("mix", 0.27f); set ("cutoff", 2100.0f);
        set ("resonance", 0.38f); set ("drive", 0.30f); set ("width", 0.18f); set ("damping", 0.52f);
    }
    else // more animated, higher feedback — still capped by the processor
    {
        set ("feedback", 0.70f); set ("mix", 0.32f); set ("cutoff", 3400.0f);
        set ("resonance", 0.58f); set ("drive", 0.42f); set ("width", 0.24f); set ("damping", 0.30f);
    }
}

void DubChamberAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ink);

    auto outer = getLocalBounds().toFloat().reduced (10.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (outer, 18.0f);
    g.setColour (brass.withAlpha (0.9f));
    g.drawRoundedRectangle (outer, 18.0f, 1.2f);

    // Top accent rails and small hardware-style fasteners.
    g.setColour (brass);
    g.fillRoundedRectangle (22.0f, 20.0f, 5.0f, 58.0f, 2.0f);
    g.setColour (green);
    g.fillRoundedRectangle (31.0f, 20.0f, 2.0f, 58.0f, 1.0f);

    for (auto pt : { juce::Point<float> (28, 28), juce::Point<float> ((float) getWidth()-28, 28),
                     juce::Point<float> (28, (float) getHeight()-28), juce::Point<float> ((float) getWidth()-28, (float) getHeight()-28) })
    {
        g.setColour (juce::Colour (0xff080d0b));
        g.fillEllipse (pt.x - 3.0f, pt.y - 3.0f, 6.0f, 6.0f);
        g.setColour (brass.withAlpha (0.65f));
        g.drawEllipse (pt.x - 3.0f, pt.y - 3.0f, 6.0f, 6.0f, 0.8f);
    }

    g.setColour (brass.withAlpha (0.22f));
    g.drawLine (28.0f, 84.0f, (float) getWidth() - 28.0f, 84.0f, 1.0f);
    g.drawLine (28.0f, 292.0f, (float) getWidth() - 28.0f, 292.0f, 1.0f);
    g.drawLine (28.0f, 440.0f, (float) getWidth() - 28.0f, 440.0f, 1.0f);

    g.setColour (green.withAlpha (0.07f));
    for (int i = 0; i < 22; ++i)
        g.fillRoundedRectangle ((float) (getWidth() - 194 + i * 6), 52.0f, 3.0f, 12.0f, 1.0f);
}

void DubChamberAudioProcessorEditor::resized()
{
    const int w = getWidth();
    title.setBounds (42, 22, 310, 36);
    subtitle.setBounds (44, 57, 360, 20);
    statusLabel.setBounds (w - 330, 32, 275, 22);

    delayLabel.setBounds (34, 91, 250, 20);
    filterLabel.setBounds (34, 299, 300, 20);
    division.setBounds (w - 220, 91, 78, 28);
    sync.setBounds (w - 138, 91, 110, 28);

    const int margin = 28;
    const int cellW = (w - margin * 2) / 4;
    juce::Slider* sliders[] = { &feedback, &mix, &time, &width, &cutoff, &resonance, &drive, &damping };
    const int orderLabels[] = { 0, 1, 7, 5, 2, 3, 4, 6 };
    for (int i = 0; i < 8; ++i)
    {
        const int row = i / 4;
        const int col = i % 4;
        const int x = margin + col * cellW;
        const int y = row == 0 ? 111 : 319;
        labels[orderLabels[i]].setBounds (x, y, cellW - 4, 18);
        sliders[i]->setBounds (x + 5, y + 17, cellW - 14, 118);
    }

    colourLabel.setBounds (34, 449, 180, 20);
    rootsPreset.setBounds (34, 478, 132, 34);
    skankPreset.setBounds (174, 478, 160, 34);
    runawayPreset.setBounds (342, 478, 142, 34);
    throwButton.setBounds (w - 282, 472, 168, 42);
    panicButton.setBounds (w - 108, 478, 80, 34);
}
