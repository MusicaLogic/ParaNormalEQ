/*
  ==============================================================================

    EQDrawingArea.h
    Created: 3 Sep 2026 7:25:02am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Style/VisualStyle.h"
#include "EQBand.h"
#include "../State/EQState.h"

#include "../DSP/SpectrumAnalyzer.h"
#include "../DSP/SpectrumDataBuffer.h"


class EQDrawingArea : public juce::Component,
                      private juce::Timer
{
public:

    // ============================================================
    // Construction
    // ============================================================

    EQDrawingArea();
    ~EQDrawingArea() override = default;


    // ============================================================
    // JUCE
    // ============================================================

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;


    // ============================================================
    // EQ state
    // ============================================================

    void setEQState(const EQState& newState);

    const EQState& getEQState() const noexcept
    {
        return state;
    }


    // ============================================================
    // Selected band
    // ============================================================

    int getSelectedBand() const noexcept
    {
        return selectedBand;
    }

    void setSelectedBand(int newSelectedBand) noexcept;


    // ============================================================
    // Change notification
    // ============================================================

    std::function<void(const EQState&)> onEQChanged;

    std::function<void(int)> onSelectedBandChanged;


    // ============================================================
    // Spectrum plotting
    // ============================================================

    static constexpr int numSpectrumBands =
        SpectrumAnalyzer::numSpectrumBands;

    using Spectrum =
        SpectrumAnalyzer::Spectrum;

    using SpectrumBuffer =
        SpectrumDataBuffer<numSpectrumBands>;

    void setSpectrumBuffer(
        SpectrumBuffer& buffer) noexcept;


private:

    // ============================================================
    // Drawing
    // ============================================================

    void drawFrame(juce::Graphics& g);

    void drawPlot(juce::Graphics& g);

    void drawPlotFill(juce::Graphics& g);

    void drawAxisGuides(juce::Graphics& g);

    void drawBandButtons(juce::Graphics& g);


    // ============================================================
    // EQ response
    // ============================================================

    float calculateMagnitudeDB(
        double frequency) const;

    float calculateBandMagnitudeDB(
        const EQBand& band,
        double frequency) const;


    // ============================================================
    // Biquad response
    //
    // Used only for visualisation at this stage.
    // The actual real-time DSP will be implemented separately.
    // ============================================================

    struct BiquadCoefficients
    {
        double b0 = 1.0;
        double b1 = 0.0;
        double b2 = 0.0;

        double a0 = 1.0;
        double a1 = 0.0;
        double a2 = 0.0;
    };

    BiquadCoefficients makeCoefficients(
        const EQBand& band) const;

    double evaluateBiquadMagnitude(
        const BiquadCoefficients& coefficients,
        double frequency) const;


    // ============================================================
    // Coordinate conversion
    // ============================================================

    float frequencyToX(double frequency) const;

    double xToFrequency(float x) const;

    float gainToY(float gain) const;

    float yToGain(float y) const;
    
    juce::Point<float> dragOffset;


    // ============================================================
    // Interaction
    // ============================================================

    int findButtonAt(
        const juce::Point<float>& position) const;

    void updateSelectedBandFromMouse(
        const juce::Point<float>& position);

    bool isFrequencyAllowed(
        int band,
        float frequency) const;

    void notifyEQChanged();


    // ============================================================
    // Geometry
    // ============================================================

    juce::Rectangle<float> getPlotBounds() const;


    // ============================================================
    // EQ parameters
    // ============================================================

    static constexpr int numBands = 3;

    static constexpr float minFrequency = 20.0f;
    static constexpr float maxFrequency = 20000.0f;

    static constexpr float minGain = -12.0f;
    static constexpr float maxGain = 12.0f;

    static constexpr float minQ = 0.1f;
    static constexpr float maxQ = 10.0f;

    // Minimum horizontal distance between handles.
    static constexpr float minimumButtonSeparation = 45.0f;

    static constexpr float buttonRadius = 11.0f;

    static constexpr float plotPadding = 10.0f;


    // ============================================================
    // EQ state
    // ============================================================

    EQState state;

    int selectedBand = -1;

    bool dragging = false;


    // ============================================================
    // Appearance
    // ============================================================

    const VisualStyle::ColorSet& colourSet =
        VisualStyle::Palette::green;


    // ============================================================
    // Spectrum visualization
    // ============================================================

    SpectrumBuffer* spectrumBuffer = nullptr;

    Spectrum inputSpectrum {};
    Spectrum outputSpectrum {};

    void timerCallback() override;

    void updateSpectrumVisualization();

    void drawSpectrum(
        juce::Graphics& g);

    void drawSpectrumLines(
        juce::Graphics& g,
        const Spectrum& spectrum,
        const juce::Colour& colour);

    static constexpr float spectrumMinDB = -80.0f;
    static constexpr float spectrumMaxDB = 0.0f;

    float spectrumDBToY(float dB) const;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQDrawingArea)
};
