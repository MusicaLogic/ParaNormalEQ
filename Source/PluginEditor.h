/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/EQDrawingArea.h"
#include "UI/TouchBand.h"
#include "UI/BandSelector.h"
#include "State/EQState.h"

//==============================================================================
/**
*/
class ParaNormalEQ_testsAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                                public juce::ChangeListener
{
public:
    static constexpr int numBands = 3;
    
    ParaNormalEQ_testsAudioProcessorEditor (ParaNormalEQ_testsAudioProcessor&);
    ~ParaNormalEQ_testsAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
//    void setInteractionMode(
//        EQDrawingArea::InteractionMode newMode);
//
//    EQDrawingArea::InteractionMode getInteractionMode() const noexcept
//    {
//        return interactionMode;
//    }

//    const std::array<float, numBands>& getGains() const noexcept
//    {
//        return gains;
//    }
    
    // listen to processor for loading saved states
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ParaNormalEQ_testsAudioProcessor& audioProcessor;
    
    //==============================================================
    // UI components
    //==============================================================

    EQDrawingArea eqDrawingArea;

    TouchBand qBand {
        TouchBand::Direction::UpDown
    };

    TouchBand gainBand {
        TouchBand::Direction::UpDown
    };
    
    TouchBand frequencyBand {
        TouchBand::Direction::LeftRight
    };

//    BandSelector bandSelector;
    
    juce::TextButton resetButton;
    void configure_resetButton();
    
    juce::TextButton shelfPassButton;
    void configure_shelfPassButton();
    void updateShelfPassButton();
    void toggleShelfPass();
    
    const VisualStyle::ColorSet& colourSet = VisualStyle::Palette::green;
    


    //==============================================================
    // EQ state
    //==============================================================

    int selectedBand = 1;
//
//    std::array<float, numBands> gains {};
//    std::array<float, numBands> gainsAtGestureStart {};
//    float gainAtGestureStart = 0.0f;
    
//    EQDrawingArea::InteractionMode interactionMode =
//            EQDrawingArea::InteractionMode::drawing;
    
    EQState eqState;
    
    float qAtGestureStart = 0.707f;
    float gainAtGestureStart = 0.0f;
    float frequencyAtGestureStart = 1000.0f;
    
    // UI communication
    void setupCallbacks();

    void setSelectedBand(int band);

    //==============================================================
    // Interaction
    //==============================================================

    void handleQGesture(float amount);
    void handleGainGesture(float amount);
    void handleFrequencyGesture(float amount);
    
    void resetEQ();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParaNormalEQ_testsAudioProcessorEditor)
};
