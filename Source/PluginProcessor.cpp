/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParaNormalEQ_testsAudioProcessor::ParaNormalEQ_testsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

ParaNormalEQ_testsAudioProcessor::~ParaNormalEQ_testsAudioProcessor()
{
}

//==============================================================================
const juce::String ParaNormalEQ_testsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ParaNormalEQ_testsAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ParaNormalEQ_testsAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ParaNormalEQ_testsAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ParaNormalEQ_testsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ParaNormalEQ_testsAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ParaNormalEQ_testsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ParaNormalEQ_testsAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ParaNormalEQ_testsAudioProcessor::getProgramName (int index)
{
    return {};
}

void ParaNormalEQ_testsAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ParaNormalEQ_testsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    for (auto& eq : parametricEQ_)
        eq.prepare(sampleRate);
    
    inputSpectrumAnalyzer.setSampleRate(sampleRate);
    outputSpectrumAnalyzer.setSampleRate(sampleRate);

    inputSpectrumAnalyzer.reset();
    outputSpectrumAnalyzer.reset();
}

void ParaNormalEQ_testsAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ParaNormalEQ_testsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ParaNormalEQ_testsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    const auto numSamples =
            static_cast<std::size_t>(buffer.getNumSamples());
    const auto numChannels =
            std::min(
                static_cast<std::size_t>(buffer.getNumChannels()),
                NumChannels);
    
    // TODO: analyze sum of left and right channels
    // TODO: both in input and output
    
    // analyze input spectrum before processing
    inputSpectrumAnalyzer.processBlock(
        buffer.getReadPointer(0),
        buffer.getNumSamples());
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        parametricEQ_[channel].process(
                    buffer.getWritePointer(
                        static_cast<int>(channel)),
                    numSamples);
    }
    
    // analyze output spectrum after processing
    outputSpectrumAnalyzer.processBlock(
        buffer.getReadPointer(0),
        buffer.getNumSamples());
    
    // ============================================================
    // MAKE LATEST DATA AVAILABLE TO GUI
    // ============================================================

    spectrumData.publishInput(
        inputSpectrumAnalyzer.getSpectrum());

    spectrumData.publishOutput(
        outputSpectrumAnalyzer.getSpectrum());
}

//==============================================================================
bool ParaNormalEQ_testsAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ParaNormalEQ_testsAudioProcessor::createEditor()
{
    return new ParaNormalEQ_testsAudioProcessorEditor (*this);
}

//==============================================================================
void ParaNormalEQ_testsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ValueTree state("EQState");

    state.setProperty("version", 1, nullptr);

    for (std::size_t i = 0; i < eqState.bands.size(); ++i)
    {
        const auto& band = eqState.bands[i];

        auto bandNode = juce::ValueTree("Band");

        bandNode.setProperty(
            "role",
            static_cast<int>(band.role),
            nullptr);

        bandNode.setProperty(
            "type",
            static_cast<int>(band.type),
            nullptr);

        bandNode.setProperty(
            "frequency",
            band.frequency,
            nullptr);

        bandNode.setProperty(
            "gain",
            band.gain,
            nullptr);

        bandNode.setProperty(
            "q",
            band.q,
            nullptr);

        bandNode.setProperty(
            "enabled",
            band.enabled,
            nullptr);

        state.addChild(bandNode, -1, nullptr);
    }

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void ParaNormalEQ_testsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml == nullptr)
        return;

    if (!xml->hasTagName("EQState"))
        return;

    const int version =
        xml->getIntAttribute("version", 1);

    if (version != 1)
        return;

    EQState newState;

    int bandIndex = 0;

    for (const auto* bandXml : xml->getChildIterator())
    {
        if (!bandXml->hasTagName("Band"))
            continue;

        if (bandIndex >= static_cast<int>(newState.bands.size()))
            break;

        auto& band = newState.bands[
            static_cast<std::size_t>(bandIndex)];

        band.role =
            static_cast<EQBandRole>(
                bandXml->getIntAttribute(
                    "role",
                    static_cast<int>(band.role)));

        band.type =
            static_cast<EQBandType>(
                bandXml->getIntAttribute(
                    "type",
                    static_cast<int>(band.type)));

        band.frequency =
            static_cast<float>(
                bandXml->getDoubleAttribute(
                    "frequency",
                    band.frequency));

        band.gain =
            static_cast<float>(
                bandXml->getDoubleAttribute(
                    "gain",
                    band.gain));

        band.q =
            static_cast<float>(
                bandXml->getDoubleAttribute(
                    "q",
                    band.q));

        band.enabled =
            bandXml->getBoolAttribute(
                "enabled",
                band.enabled);

        ++bandIndex;
    }

    setEQState(newState);
}

void ParaNormalEQ_testsAudioProcessor::reset()
{
    for (auto& eq : parametricEQ_)
        eq.reset();
}

//// DSP-related functions
//void ParaNormalEQ_testsAudioProcessor::setGain(std::size_t band, float gainDb){
//    for (auto& eq : graphicEQ_)
//        eq.setGain(band, gainDb);
//}
//void ParaNormalEQ_testsAudioProcessor::setGains(const GraphicEQ::Gains& gains){
////    // ParaNormal
////    for (std::size_t i = 0;
////         i < EQState::NumBands;
////         ++i)
////    {
////        eqState.gains[i] = gains[i];
////    }
////    for (auto& eq : graphicEQ_)
////        eq.setGains(gains);
//}
//float ParaNormalEQ_testsAudioProcessor::getGain(std::size_t band) const{
//    if (band >= GraphicEQ::NumBands)
//        return 0.0f;
//
//    return graphicEQ_[0].getGain(band);
//}
//
//GraphicEQ::Gains ParaNormalEQ_testsAudioProcessor::getGains() const
//{
//    GraphicEQ::Gains gains{};
//    
////    // ParaNormal
////    for (std::size_t i = 0;
////         i < GraphicEQ::NumBands;
////         ++i)
////    {
////        gains[i] = eqState.gains[i];
////    }
//
//    return gains;
//}

EQState ParaNormalEQ_testsAudioProcessor::getEQState() const
{
    return eqState;
}

void ParaNormalEQ_testsAudioProcessor::setEQState(
    const EQState& state)
{
    eqState = state;
    
    for (std::size_t band = 0;
         band < ParametricEQ::NumBands;
         ++band)
    {
        const auto& b = eqState.bands[band];

        for (auto& eq : parametricEQ_)
        {
            eq.setFrequency(
                band,
                b.frequency);

            eq.setGain(
                band,
                b.enabled ? b.gain : 0.0f);

            eq.setQ(
                band,
                b.q);
            
            eq.setType(
                band,
                b.type);
        }
    }

    sendChangeMessage();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParaNormalEQ_testsAudioProcessor();
}
