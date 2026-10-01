/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Frequency constraints
namespace
{
    constexpr float minFrequency = 20.0f;
    constexpr float maxFrequency = 20000.0f;

    // Minimum separation between adjacent EQ bands, in Hz.
    //
    // This is deliberately a processor-level constraint, independent
    // of the UI's pixel-based button separation.
    constexpr float minimumFrequencySeparation = 10.0f;
}

//==============================================================================
ParaNormalEQ_testsAudioProcessor::ParaNormalEQ_testsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#else
     : AudioProcessor()
#endif
     , apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    const auto parameterIDs = {
        "leftFrequency", "leftGain", "leftQ", "leftType",
        "middleFrequency", "middleGain", "middleQ",
        "rightFrequency", "rightGain", "rightQ", "rightType"
    };

    for (const auto* id : parameterIDs)
        apvts.addParameterListener(id, this);
}

ParaNormalEQ_testsAudioProcessor::~ParaNormalEQ_testsAudioProcessor()
{
    const auto parameterIDs = {
        "leftFrequency", "leftGain", "leftQ", "leftType",
        "middleFrequency", "middleGain", "middleQ",
        "rightFrequency", "rightGain", "rightQ", "rightType"
    };

    for (const auto* id : parameterIDs)
        apvts.removeParameterListener(id, this);
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
    
    updateDSPFromParameters();
    parametersChanged.store(false, std::memory_order_release);
    
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
    
    if (parametersChanged.exchange(false, std::memory_order_acq_rel))
    {
        updateDSPFromParameters();
        sendChangeMessage();
    }
    
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
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
    
//    juce::ValueTree state("EQState");
//
//    state.setProperty("version", 1, nullptr);
//
//    for (std::size_t i = 0; i < eqState.bands.size(); ++i)
//    {
//        const auto& band = eqState.bands[i];
//
//        auto bandNode = juce::ValueTree("Band");
//
//        bandNode.setProperty(
//            "role",
//            static_cast<int>(band.role),
//            nullptr);
//
//        bandNode.setProperty(
//            "type",
//            static_cast<int>(band.type),
//            nullptr);
//
//        bandNode.setProperty(
//            "frequency",
//            band.frequency,
//            nullptr);
//
//        bandNode.setProperty(
//            "gain",
//            band.gain,
//            nullptr);
//
//        bandNode.setProperty(
//            "q",
//            band.q,
//            nullptr);
//
//        bandNode.setProperty(
//            "enabled",
//            band.enabled,
//            nullptr);
//
//        state.addChild(bandNode, -1, nullptr);
//    }
//
//    if (auto xml = state.createXml())
//        copyXmlToBinary(*xml, destData);
}

void ParaNormalEQ_testsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr)
        return;

    // Current format: APVTS ValueTree.
    if (xml->hasTagName(apvts.state.getType()))
    {
        const auto state = juce::ValueTree::fromXml(*xml);

        if (!state.isValid())
            return;

        apvts.replaceState(state);
        parametersChanged.store(true, std::memory_order_release);
        sendChangeMessage();
        return;
    }

    // Legacy format: the previous EQState XML. Keep this so existing presets
    // made by the old implementation can still be opened once and migrated.
    if (!xml->hasTagName("EQState"))
        return;

    const int version = xml->getIntAttribute("version", 1);
    if (version != 1)
        return;

    EQState legacyState;
    int bandIndex = 0;

    for (const auto* bandXml : xml->getChildIterator())
    {
        if (!bandXml->hasTagName("Band"))
            continue;

        if (bandIndex >= static_cast<int>(legacyState.bands.size()))
            break;

        auto& band = legacyState.bands[static_cast<std::size_t>(bandIndex)];

        band.role = static_cast<EQBandRole>(
            bandXml->getIntAttribute("role", static_cast<int>(band.role)));
        band.type = static_cast<EQBandType>(
            bandXml->getIntAttribute("type", static_cast<int>(band.type)));
        band.frequency = static_cast<float>(
            bandXml->getDoubleAttribute("frequency", band.frequency));
        band.gain = static_cast<float>(
            bandXml->getDoubleAttribute("gain", band.gain));
        band.q = static_cast<float>(
            bandXml->getDoubleAttribute("q", band.q));
        // enabled was part of the old format, but is intentionally ignored:
        // all three bands are now always enabled.

        ++bandIndex;
    }

    setEQState(legacyState);
    sendChangeMessage();
    
//    auto xml = getXmlFromBinary(data, sizeInBytes);
//
//    if (xml == nullptr)
//        return;
//
//    if (!xml->hasTagName("EQState"))
//        return;
//
//    const int version =
//        xml->getIntAttribute("version", 1);
//
//    if (version != 1)
//        return;
//
//    EQState newState;
//
//    int bandIndex = 0;
//
//    for (const auto* bandXml : xml->getChildIterator())
//    {
//        if (!bandXml->hasTagName("Band"))
//            continue;
//
//        if (bandIndex >= static_cast<int>(newState.bands.size()))
//            break;
//
//        auto& band = newState.bands[
//            static_cast<std::size_t>(bandIndex)];
//
//        band.role =
//            static_cast<EQBandRole>(
//                bandXml->getIntAttribute(
//                    "role",
//                    static_cast<int>(band.role)));
//
//        band.type =
//            static_cast<EQBandType>(
//                bandXml->getIntAttribute(
//                    "type",
//                    static_cast<int>(band.type)));
//
//        band.frequency =
//            static_cast<float>(
//                bandXml->getDoubleAttribute(
//                    "frequency",
//                    band.frequency));
//
//        band.gain =
//            static_cast<float>(
//                bandXml->getDoubleAttribute(
//                    "gain",
//                    band.gain));
//
//        band.q =
//            static_cast<float>(
//                bandXml->getDoubleAttribute(
//                    "q",
//                    band.q));
//
//        band.enabled =
//            bandXml->getBoolAttribute(
//                "enabled",
//                band.enabled);
//
//        ++bandIndex;
//    }
//
//    setEQState(newState);
}

void ParaNormalEQ_testsAudioProcessor::reset()
{
    for (auto& eq : parametricEQ_)
        eq.reset();
    
    parametersChanged.store(true, std::memory_order_release);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ParaNormalEQ_testsAudioProcessor::createParameterLayout()
{
    using Parameter = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    constexpr float minFrequency = 20.0f;
    constexpr float maxFrequency = 20000.0f;
    constexpr float minGain = -12.0f;
    constexpr float maxGain = 12.0f;
    constexpr float minQ = 0.1f;
    constexpr float maxQ = 10.0f;
    constexpr float frequencySkew = 0.25f;

    auto frequencyAttributes = []
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction(
                [] (float value, int) { return juce::String(value, 1) + " Hz"; });
    };

    auto gainAttributes = []
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction(
                [] (float value, int) { return juce::String(value, 2) + " dB"; });
    };

    auto qAttributes = []
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel("Q")
            .withStringFromValueFunction(
                [] (float value, int) { return juce::String(value, 3); });
    };

    layout.add(std::make_unique<Parameter>("leftFrequency", "Left Frequency",
        juce::NormalisableRange<float>(minFrequency, maxFrequency, 0.01f, frequencySkew),
        100.0f, frequencyAttributes()));
    layout.add(std::make_unique<Parameter>("leftGain", "Left Gain",
        juce::NormalisableRange<float>(minGain, maxGain, 0.01f),
        0.0f, gainAttributes()));
    layout.add(std::make_unique<Parameter>("leftQ", "Left Q",
        juce::NormalisableRange<float>(minQ, maxQ, 0.001f),
        0.707f, qAttributes()));
    layout.add(std::make_unique<Choice>("leftType", "Left Type",
        juce::StringArray { "Low Shelf", "High Pass" }, 0));

    layout.add(std::make_unique<Parameter>("middleFrequency", "Middle Frequency",
        juce::NormalisableRange<float>(minFrequency, maxFrequency, 0.01f, frequencySkew),
        1000.0f, frequencyAttributes()));
    layout.add(std::make_unique<Parameter>("middleGain", "Middle Gain",
        juce::NormalisableRange<float>(minGain, maxGain, 0.01f),
        0.0f, gainAttributes()));
    layout.add(std::make_unique<Parameter>("middleQ", "Middle Q",
        juce::NormalisableRange<float>(minQ, maxQ, 0.001f),
        0.707f, qAttributes()));

    layout.add(std::make_unique<Parameter>("rightFrequency", "Right Frequency",
        juce::NormalisableRange<float>(minFrequency, maxFrequency, 0.01f, frequencySkew),
        8000.0f, frequencyAttributes()));
    layout.add(std::make_unique<Parameter>("rightGain", "Right Gain",
        juce::NormalisableRange<float>(minGain, maxGain, 0.01f),
        0.0f, gainAttributes()));
    layout.add(std::make_unique<Parameter>("rightQ", "Right Q",
        juce::NormalisableRange<float>(minQ, maxQ, 0.001f),
        0.707f, qAttributes()));
    layout.add(std::make_unique<Choice>("rightType", "Right Type",
        juce::StringArray { "High Shelf", "Low Pass" }, 0));

    return layout;
}

void ParaNormalEQ_testsAudioProcessor::enforceFrequencyOrdering()
{
    auto getFrequency = [this] (const char* id)
    {
        return apvts.getRawParameterValue(id)->load();
    };

    auto setFrequency = [this] (const char* id, float frequency)
    {
        if (auto* parameter = apvts.getParameter(id))
        {
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(frequency));
        }
    };

    float leftFrequency =
        getFrequency("leftFrequency");

    float middleFrequency =
        getFrequency("middleFrequency");

    float rightFrequency =
        getFrequency("rightFrequency");


    // ------------------------------------------------------------
    // Left must remain below middle.
    // ------------------------------------------------------------

    if (leftFrequency >
        middleFrequency - minimumFrequencySeparation)
    {
        leftFrequency =
            middleFrequency - minimumFrequencySeparation;

        leftFrequency =
            juce::jmax(leftFrequency, minFrequency);

        setFrequency(
            "leftFrequency",
            leftFrequency);
    }


    // ------------------------------------------------------------
    // Right must remain above middle.
    // ------------------------------------------------------------

    if (rightFrequency <
        middleFrequency + minimumFrequencySeparation)
    {
        rightFrequency =
            middleFrequency + minimumFrequencySeparation;

        rightFrequency =
            juce::jmin(rightFrequency, maxFrequency);

        setFrequency(
            "rightFrequency",
            rightFrequency);
    }


    // ------------------------------------------------------------
    // Middle must remain between left and right.
    //
    // This also protects us when loading a preset/state that
    // contains invalid ordering.
    // ------------------------------------------------------------

    const float minimumMiddle =
        leftFrequency + minimumFrequencySeparation;

    const float maximumMiddle =
        rightFrequency - minimumFrequencySeparation;

    if (minimumMiddle <= maximumMiddle)
    {
        if (middleFrequency < minimumMiddle)
        {
            middleFrequency = minimumMiddle;

            setFrequency(
                "middleFrequency",
                middleFrequency);
        }
        else if (middleFrequency > maximumMiddle)
        {
            middleFrequency = maximumMiddle;

            setFrequency(
                "middleFrequency",
                middleFrequency);
        }
    }
}

void ParaNormalEQ_testsAudioProcessor::parameterChanged(const juce::String& parameterID, float)
{
    parametersChanged.store(true, std::memory_order_release);
}

EQState ParaNormalEQ_testsAudioProcessor::makeEQStateFromParameters() const
{
    EQState state;

    const auto f = [this] (const char* id) -> float
    {
        return apvts.getRawParameterValue(id)->load();
    };

    const auto c = [this] (const char* id) -> int
    {
        return static_cast<int>(apvts.getRawParameterValue(id)->load());
    };

    state.bands[0] = { EQBandRole::left,
                       c("leftType") == 0 ? EQBandType::lowShelf : EQBandType::highPass,
                       f("leftFrequency"), f("leftGain"), f("leftQ"), true };

    state.bands[1] = { EQBandRole::middle, EQBandType::bell,
                       f("middleFrequency"), f("middleGain"), f("middleQ"), true };

    state.bands[2] = { EQBandRole::right,
                       c("rightType") == 0 ? EQBandType::highShelf : EQBandType::lowPass,
                       f("rightFrequency"), f("rightGain"), f("rightQ"), true };

    return state;
}

void ParaNormalEQ_testsAudioProcessor::updateDSPFromParameters()
{
    enforceFrequencyOrdering();
    
    const auto state = makeEQStateFromParameters();

    for (std::size_t band = 0; band < ParametricEQ::NumBands; ++band)
    {
        const auto& b = state.bands[band];

        for (auto& eq : parametricEQ_)
        {
            eq.setFrequency(band, b.frequency);
            eq.setGain(band, b.enabled ? b.gain : 0.0f);
            eq.setQ(band, b.q);
            eq.setType(band, b.type);
        }
    }
}

EQState ParaNormalEQ_testsAudioProcessor::getEQState() const
{
    return makeEQStateFromParameters();
}

void ParaNormalEQ_testsAudioProcessor::setEQState(const EQState& state)
{
    auto setFloat = [this] (const char* id, float value)
    {
        if (auto* parameter = apvts.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };

    auto setChoice = [this] (const char* id, int index)
    {
        if (auto* parameter = apvts.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(index)));
    };

    const auto& left = state.bands[0];
    setFloat("leftFrequency", left.frequency);
    setFloat("leftGain", left.gain);
    setFloat("leftQ", left.q);
    setChoice("leftType", left.type == EQBandType::lowShelf ? 0 : 1);

    const auto& middle = state.bands[1];
    setFloat("middleFrequency", middle.frequency);
    setFloat("middleGain", middle.gain);
    setFloat("middleQ", middle.q);

    const auto& right = state.bands[2];
    setFloat("rightFrequency", right.frequency);
    setFloat("rightGain", right.gain);
    setFloat("rightQ", right.q);
    setChoice("rightType", right.type == EQBandType::highShelf ? 0 : 1);
}

//EQState ParaNormalEQ_testsAudioProcessor::getEQState() const
//{
//    return eqState;
//}
//
//void ParaNormalEQ_testsAudioProcessor::setEQState(
//    const EQState& state)
//{
//    eqState = state;
//    
//    for (std::size_t band = 0;
//         band < ParametricEQ::NumBands;
//         ++band)
//    {
//        const auto& b = eqState.bands[band];
//
//        for (auto& eq : parametricEQ_)
//        {
//            eq.setFrequency(
//                band,
//                b.frequency);
//
//            eq.setGain(
//                band,
//                b.enabled ? b.gain : 0.0f);
//
//            eq.setQ(
//                band,
//                b.q);
//            
//            eq.setType(
//                band,
//                b.type);
//        }
//    }
//
//    sendChangeMessage();
//}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParaNormalEQ_testsAudioProcessor();
}
