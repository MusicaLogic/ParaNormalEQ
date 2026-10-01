/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "State/EQState.h"
#include "DSP/EQConstants.h"
#include "DSP/ParametricEQ.h"
#include "DSP/SpectrumAnalyzer.h"
#include "DSP/SpectrumDataBuffer.h"
#include <array>
#include <atomic>

//==============================================================================
/**
*/
class ParaNormalEQ_testsAudioProcessor  : public juce::AudioProcessor,
                                            public juce::ChangeBroadcaster,
                                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    ParaNormalEQ_testsAudioProcessor();
    ~ParaNormalEQ_testsAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void reset() override;
    
    //==============================================================================
    // APVTS
    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts; }
    
    //==============================================================================
    // EQ state adapter
    //
    // EQState is not the authoritative persistent state. These functions
    // provide a convenient snapshot for the existing editor/UI code and write
    // changes back into APVTS.
    //==============================================================================
    EQState getEQState() const;
    void setEQState(const EQState& state);
    
    //==============================================================================
    SpectrumDataBuffer<
        SpectrumAnalyzer::numSpectrumBands>&
    getSpectrumData() noexcept
    {
        return spectrumData;
    }

private:
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void enforceFrequencyOrdering();
    void updateDSPFromParameters();
    EQState makeEQStateFromParameters() const;
    
    static constexpr std::size_t NumChannels = 2;
    
    // APVTS is the single source of truth for all host-controllable EQ values.
    juce::AudioProcessorValueTreeState apvts;
    
    std::array<ParametricEQ, NumChannels> parametricEQ_;
    // Set by parameterChanged() and consumed at the start of processBlock().
    // This keeps DSP changes out of the parameter callback itself.
    std::atomic<bool> parametersChanged { true };
    
//    EQState eqState;
    
    SpectrumAnalyzer inputSpectrumAnalyzer;
    SpectrumAnalyzer outputSpectrumAnalyzer;
    
    SpectrumDataBuffer<
        SpectrumAnalyzer::numSpectrumBands>
        spectrumData;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParaNormalEQ_testsAudioProcessor)
};
