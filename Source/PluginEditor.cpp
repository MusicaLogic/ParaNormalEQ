/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
ParaNormalEQ_testsAudioProcessorEditor::ParaNormalEQ_testsAudioProcessorEditor (ParaNormalEQ_testsAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);
    
    addAndMakeVisible(eqDrawingArea);
    addAndMakeVisible(qBand);
    addAndMakeVisible(gainBand);
    addAndMakeVisible(frequencyBand);
//    addAndMakeVisible(bandSelector);
    
    // add reset button
    addAndMakeVisible(resetButton);
    configure_resetButton();
    
    // add shelf/pass button
    addAndMakeVisible(shelfPassButton);
    configure_shelfPassButton();
    
    // load gains from audio processor
    eqState =
        audioProcessor.getEQState();
    eqDrawingArea.setEQState(eqState);
    
    // at start gesture
    qAtGestureStart = eqState.bands[selectedBand].q;
    gainAtGestureStart = eqState.bands[selectedBand].gain;
    frequencyAtGestureStart = eqState.bands[selectedBand].frequency;
    
    // reflect initial state on drawing area
    eqDrawingArea.setSelectedBand(selectedBand);
    
    // make sure self/pass is not/showing approapriately
    updateShelfPassButton();
    
//    // make sure DSP starts the same
//    updateDSP();
    
    // Connect the spectrum visualization to the processor.
    eqDrawingArea.setSpectrumBuffer(
        audioProcessor.getSpectrumData());

    setupCallbacks();
//    setInteractionMode(interactionMode);
    
    audioProcessor.addChangeListener(this);
}

ParaNormalEQ_testsAudioProcessorEditor::~ParaNormalEQ_testsAudioProcessorEditor()
{
    audioProcessor.removeChangeListener(this);
}

//==============================================================================
void ParaNormalEQ_testsAudioProcessorEditor::paint (juce::Graphics& g)
{
//    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//
//    g.setColour (juce::Colours::white);
//    g.setFont (juce::FontOptions (15.0f));
//    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
    
    g.fillAll(VisualStyle::background);
}

void ParaNormalEQ_testsAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto area =
        getLocalBounds().reduced(
            VisualStyle::Geometry::componentMargin);

    auto bottom = area.removeFromBottom(50);

    auto left  = area.removeFromLeft(60);
    auto right = area.removeFromRight(60);

    // The drawing area defines the exact horizontal extent
    // shared by the graph and the band selector.
    eqDrawingArea.setBounds(area);

    qBand.setBounds(left.reduced(10));
    gainBand.setBounds(right.reduced(10));
    
    // 2. Chop off 10px from the left (this creates your empty margin)
    auto left_margin = bottom.removeFromLeft (60);

    // 3. Chop off 10px from the right (this creates the reset_area)
    auto resetArea = bottom.removeFromRight(60).reduced(5, 10);

    // 4. Whatever is left in the middle becomes your selector_area
    auto selector_area = bottom;
    
    frequencyBand.setBounds(selector_area.reduced(0, 10));
    
    shelfPassButton.setBounds(left_margin.reduced(5, 10));
    resetButton.setBounds(resetArea);
}

//==============================================================================
// Interaction mode
//==============================================================================

//void ParaNormalEQ_testsAudioProcessorEditor::setInteractionMode(
//    EQDrawingArea::InteractionMode newMode)
//{
//    interactionMode = newMode;
//
//    eqDrawingArea.setInteractionMode(newMode);
//
//    const bool bandMode =
//        newMode == EQDrawingArea::InteractionMode::bandSelected;
//
//    // In drawing mode there are deliberately no touch controls.
//    smoothnessBand.setActive(bandMode);
//    gainBand.setActive(bandMode);
//    bandSelector.setActive(bandMode);
//
//    bandSelector.setSelectedBand(selectedBand);
//    eqDrawingArea.setSelectedBand(selectedBand);
//
//    repaint();
//}

//==============================================================================
// Callbacks
//==============================================================================

void ParaNormalEQ_testsAudioProcessorEditor::setupCallbacks()
{
//    // BandSelector -> EQEditor
//    bandSelector.onBandChanged =
//        [this](int band)
//        {
////            setInteractionMode(
////                    EQDrawingArea::InteractionMode::bandSelected);
//            setSelectedBand(band);
//        };

    // EQDrawingArea -> EQEditor
    eqDrawingArea.onSelectedBandChanged =
        [this](int band)
        {
            if (band >= 0)
                setSelectedBand(band);
        };

    eqDrawingArea.onEQChanged =
        [this](const EQState& newState)
        {
            for (int i = 0; i < numBands; ++i)
                eqState.bands[static_cast<size_t>(i)] =
                    newState.bands[static_cast<size_t>(i)];
            
            // The drawing interaction determines the relevant band.
            const int band =
                eqDrawingArea.getSelectedBand();

            if (band >= 0)
                setSelectedBand(band);
            
            audioProcessor.setEQState(eqState);
//            updateDSP();
        };
//
    // Q gesture
    qBand.onGestureStart =
        [this]()
        {
            if (selectedBand < 0)
                return;

            qAtGestureStart =
                eqState.bands[
                    static_cast<size_t>(selectedBand)].q;
        };

    qBand.onValueChanged =
        [this](float amount)
        {
            handleQGesture(amount);
        };

    // Gain gesture
    gainBand.onGestureStart =
        [this]()
        {
            if (selectedBand < 0)
                return;

            gainAtGestureStart =
                eqState.bands[
                    static_cast<size_t>(selectedBand)].gain;
        };

    gainBand.onValueChanged =
        [this](float amount)
        {
            handleGainGesture(amount);
        };
    
    // frequency gesture
    frequencyBand.onGestureStart =
        [this]()
    {
        if (selectedBand < 0)
            return;

        frequencyAtGestureStart =
            eqState.bands[
                static_cast<size_t>(selectedBand)].frequency;
    };
    
    frequencyBand.onValueChanged =
        [this](float amount)
    {
        if (selectedBand < 0)
            return;

        handleFrequencyGesture(amount);
    };
    
    //
//    eqDrawingArea.onDrawingStarted =
//        [this]()
//        {
//            setInteractionMode(
//                EQDrawingArea::InteractionMode::drawing);
//        };
//    
    // Reset EQ
    resetButton.onClick =
        [this]()
        {
            resetEQ();
        };
    
    shelfPassButton.onClick =
        [this]()
        {
            
            // simply toggle button
            toggleShelfPass();
            
            // Update the drawing/UI state
            eqDrawingArea.setEQState(eqState);

            // Update the DSP state
            audioProcessor.setEQState(eqState);
            
            // Update button text
            updateShelfPassButton();
        };
}

//==============================================================================
// Selected band
//==============================================================================

void ParaNormalEQ_testsAudioProcessorEditor::setSelectedBand(int band)
{
    if (band < 0 || band >= numBands)
        return;

    selectedBand = band;
    
//    bandSelector.setSelectedBand(selectedBand);
    eqDrawingArea.setSelectedBand(selectedBand);
    
    updateShelfPassButton();
}

//==============================================================================
// Smoothness
//==============================================================================

void ParaNormalEQ_testsAudioProcessorEditor::handleQGesture(float amount)
{
    if (selectedBand < 0)
        return;

    constexpr float minQ = 0.1f;
    constexpr float maxQ = 10.0f;

    const float q =
        qAtGestureStart
        + amount * (maxQ - minQ);

    eqState.bands[
        static_cast<size_t>(selectedBand)].q =
        juce::jlimit(
            minQ,
            maxQ,
            q);

    eqDrawingArea.setEQState(eqState);

    audioProcessor.setEQState(eqState);

//    eqDrawingArea.setGains(gains);
//    updateDSP();
}

//==============================================================================
// Gain
//==============================================================================

void ParaNormalEQ_testsAudioProcessorEditor::handleGainGesture(float amount)
{
    if (selectedBand < 0)
        return;

    constexpr float gainRange = 12.0f;

    const float gain =
        gainAtGestureStart
        + amount * gainRange;

    eqState.bands[
        static_cast<size_t>(selectedBand)].gain =
        juce::jlimit(
            -12.0f,
            12.0f,
            gain);

    eqDrawingArea.setEQState(eqState);
    
    audioProcessor.setEQState(eqState);

//    eqDrawingArea.setGains(gains);
//    updateDSP();
}

void ParaNormalEQ_testsAudioProcessorEditor::handleFrequencyGesture(float amount)
{
    constexpr float minFrequency = 20.0f;
    constexpr float maxFrequency = 20000.0f;
    
    constexpr float octaveRange = 2.0f;
    
    const float newLogFrequency =
    std::log2(frequencyAtGestureStart)
    + amount * octaveRange;
    
    float frequency =
    std::pow(2.0f, newLogFrequency);
    
    frequency =
    juce::jlimit(
                 minFrequency,
                 maxFrequency,
                 frequency);
    
    // -------------------------------------------------------------
    // Don't cross neighbouring bands
    // -------------------------------------------------------------
    
    if (selectedBand > 0)
    {
        const float previous =
        eqState.bands[
            static_cast<size_t>(selectedBand - 1)].frequency;
        
        frequency =
        juce::jmax(
                   frequency,
                   previous + 1.0f);
    }
    
    if (selectedBand < 2)
    {
        const float next =
        eqState.bands[
            static_cast<size_t>(selectedBand + 1)].frequency;
        
        frequency =
        juce::jmin(
                   frequency,
                   next - 1.0f);
    }
    
    eqState.bands[
        static_cast<size_t>(selectedBand)].frequency =
    frequency;
    
    eqDrawingArea.setEQState(eqState);
    
    audioProcessor.setEQState(eqState);
    
//    updateDSP();
}

//==============================================================================
// DSP
//==============================================================================

//void ParaNormalEQ_testsAudioProcessorEditor::updateDSP()
//{
////    audioProcessor.setGains(gains);
//}

// Listen to audio processor state changes
void ParaNormalEQ_testsAudioProcessorEditor::changeListenerCallback(
    juce::ChangeBroadcaster* source)
{
    if (source != &audioProcessor)
        return;

    const auto state =
        audioProcessor.getEQState();

//    gains = state.gains;
//    gainsAtGestureStart = gains;
//
    eqDrawingArea.setEQState(state);
}

//==============================================================================
// Reset
//==============================================================================

void ParaNormalEQ_testsAudioProcessorEditor::resetEQ()
{
    // Reset the EQ curve to flat.
//    gains.fill(0.0f);

    // Any subsequent smoothness gesture should start
    // from the newly reset curve.
//    gainsAtGestureStart = gains;
    
    for (int i=0; i<3; i++){
        eqState.bands[i].gain = 0.0f;
        // TODO: do we need to change Q?
        eqState.bands[i].q = 0.707f;
    }
    eqState.bands[0].type = EQBandType::lowShelf;
    eqState.bands[1].type = EQBandType::bell;
    eqState.bands[2].type = EQBandType::highShelf;
    updateShelfPassButton();

    // Reset the starting point used by the gain gesture.
    qAtGestureStart = 0.707f;
    gainAtGestureStart = 0.0f;
    frequencyAtGestureStart = eqState.bands[selectedBand].frequency;

    // Update the visual representation.
    eqDrawingArea.setEQState(eqState);

    audioProcessor.setEQState(eqState);
//    // Update the DSP.
//    updateDSP();
}

void ParaNormalEQ_testsAudioProcessorEditor::configure_resetButton(){
    // configure reset button
    resetButton.setButtonText("---");
    resetButton.setTooltip("Reset EQ");
    // set colors
    const auto borderColour =
        VisualStyle::getStateColor(
                                     colourSet,
                                     false,
                                   true).withAlpha(0.35f);
    // make visuals of button
    resetButton.setColour(
        juce::TextButton::buttonColourId,
        VisualStyle::panelBackground);
    resetButton.setColour(
        juce::TextButton::textColourOffId,
                          borderColour);
    resetButton.setColour(
        juce::TextButton::textColourOnId,
                          borderColour);
    resetButton.setColour(juce::ComboBox::outlineColourId, borderColour);
}

void ParaNormalEQ_testsAudioProcessorEditor::configure_shelfPassButton()
{
    shelfPassButton.setTooltip("Switch between shelf and pass filter");

    const auto borderColour =
        VisualStyle::getStateColor(
            colourSet,
            false,
            true).withAlpha(0.35f);

    shelfPassButton.setColour(
        juce::TextButton::buttonColourId,
        VisualStyle::panelBackground);

    shelfPassButton.setColour(
        juce::TextButton::textColourOffId,
        borderColour);

    shelfPassButton.setColour(
        juce::TextButton::textColourOnId,
        borderColour);

    shelfPassButton.setColour(
        juce::ComboBox::outlineColourId,
        borderColour);

    shelfPassButton.setVisible(false);
}

void ParaNormalEQ_testsAudioProcessorEditor::updateShelfPassButton()
{
    if (selectedBand < 0 || selectedBand >= numBands)
    {
        shelfPassButton.setVisible(false);
        return;
    }

    const auto& band =
        eqState.bands[
            static_cast<size_t>(selectedBand)];

    // Only the outer bands have a shelf/pass choice.
    if (band.role == EQBandRole::left)
    {
        shelfPassButton.setButtonText(
            band.type == EQBandType::lowShelf ? "Shelf" : "Pass");

        shelfPassButton.setVisible(
            band.type == EQBandType::lowShelf ||
            band.type == EQBandType::highPass);
    }
    else if (band.role == EQBandRole::right)
    {
        shelfPassButton.setButtonText(
            band.type == EQBandType::highShelf ? "Shelf" : "Pass");

        shelfPassButton.setVisible(
            band.type == EQBandType::highShelf ||
            band.type == EQBandType::lowPass);
    }
    else
    {
        shelfPassButton.setVisible(false);
    }

    shelfPassButton.toFront(false);
}

void ParaNormalEQ_testsAudioProcessorEditor::toggleShelfPass()
{
    auto& band =
        eqState.bands[
            static_cast<size_t>(selectedBand)];

    switch (band.type)
    {
        case EQBandType::lowShelf:
            band.type = EQBandType::highPass;
            break;

        case EQBandType::lowPass:
            band.type = EQBandType::highShelf;
            break;

        case EQBandType::highShelf:
            band.type = EQBandType::lowPass;
            break;

        case EQBandType::highPass:
            band.type = EQBandType::lowShelf;
            break;

        case EQBandType::bell:
            break;
    }
}
