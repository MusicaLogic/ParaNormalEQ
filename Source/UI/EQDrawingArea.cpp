/*
  ==============================================================================

    EQDrawingArea.cpp
    Created: 3 Sep 2026 7:25:02am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#include "EQDrawingArea.h"

#include <cmath>
#include <algorithm>


//==============================================================================
// Constructor
//==============================================================================

EQDrawingArea::EQDrawingArea()
{
    setOpaque(true);

    inputSpectrum.fill(spectrumMinDB);
    outputSpectrum.fill(spectrumMinDB);

    startTimerHz(15);
}


//==============================================================================
// JUCE
//==============================================================================

void EQDrawingArea::paint(juce::Graphics& g)
{
    g.fillAll(VisualStyle::panelBackground);

    drawFrame(g);

    drawAxisGuides(g);

    // Real-time spectrum first.
    drawSpectrum(g);

    // EQ response on top.
    drawPlotFill(g);
    drawPlot(g);

    // Three interactive filter handles.
    drawBandButtons(g);
}


//==============================================================================
// Frame
//==============================================================================

void EQDrawingArea::drawFrame(juce::Graphics& g)
{
    const auto bounds =
        getLocalBounds()
            .toFloat()
            .reduced(1.0f);

//    const bool active =
//        selectedBand >= 0;

    const auto colour =
        VisualStyle::getStateColor(
            colourSet,
            true,
            true);

    g.setColour(colour);

    g.drawRoundedRectangle(
        bounds,
        VisualStyle::Geometry::componentCornerRadius,
        VisualStyle::Geometry::componentBorderThickness);
}


//==============================================================================
// EQ response curve
//==============================================================================

void EQDrawingArea::drawPlot(juce::Graphics& g)
{
    const auto bounds =
        getPlotBounds();

    juce::Path path;

    constexpr int numPoints = 256;

    for (int i = 0; i < numPoints; ++i)
    {
        const float normalized =
            static_cast<float>(i)
            / static_cast<float>(numPoints - 1);

        const double frequency =
            minFrequency *
            std::pow(
                maxFrequency / minFrequency,
                normalized);

        const float magnitude =
            calculateMagnitudeDB(frequency);

        const float x =
            frequencyToX(frequency);

        const float y =
            gainToY(magnitude);

        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    const bool active =
        selectedBand >= 0;

    const auto colour =
        VisualStyle::getStateColor(
            colourSet,
            active,
            true);

    g.setColour(colour);

    g.strokePath(
        path,
        juce::PathStrokeType(
            2.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));
}


//==============================================================================
// EQ response fill
//==============================================================================

void EQDrawingArea::drawPlotFill(juce::Graphics& g)
{
    const auto bounds =
        getPlotBounds();

    juce::Path fillPath;

    constexpr int numPoints = 256;

    for (int i = 0; i < numPoints; ++i)
    {
        const float normalized =
            static_cast<float>(i)
            / static_cast<float>(numPoints - 1);

        const double frequency =
            minFrequency *
            std::pow(
                maxFrequency / minFrequency,
                normalized);

        const float magnitude =
            calculateMagnitudeDB(frequency);

        const float x =
            frequencyToX(frequency);

        const float y =
            gainToY(magnitude);

        if (i == 0)
            fillPath.startNewSubPath(x, y);
        else
            fillPath.lineTo(x, y);
    }

    fillPath.lineTo(
        frequencyToX(maxFrequency),
        bounds.getBottom());

    fillPath.lineTo(
        frequencyToX(minFrequency),
        bounds.getBottom());

    fillPath.closeSubPath();

    const bool active =
        selectedBand >= 0;

    const auto colour =
        VisualStyle::getStateColor(
            colourSet,
            active,
            true);

    g.setColour(
        colour.withAlpha(0.04f));

    g.fillPath(fillPath);
}


//==============================================================================
// Axis guides
//==============================================================================

void EQDrawingArea::drawAxisGuides(
    juce::Graphics& g)
{
    const auto bounds =
        getPlotBounds();

    const auto guideColour =
        colourSet.dim.withAlpha(0.25f);

    const auto textColour =
        colourSet.dim.withAlpha(0.65f);


    // ============================================================
    // Gain
    // ============================================================

    constexpr float guideGains[] =
    {
        6.0f,
        0.0f,
        -6.0f
    };

    constexpr const char* guideLabels[] =
    {
        "+6dB",
        "0dB",
        "-6dB"
    };

    for (int i = 0; i < 3; ++i)
    {
        const float y =
            gainToY(guideGains[i]);

        g.setColour(guideColour);

        g.drawHorizontalLine(
            juce::roundToInt(y),
            bounds.getX(),
            bounds.getRight());

        g.setColour(textColour);

        g.setFont(
            VisualStyle::getDefaultFont(
                VisualStyle::FontSize::small));

        g.drawText(
            guideLabels[i],
            juce::roundToInt(bounds.getX() + 8.0f),
            juce::roundToInt(y - 7.0f),
            45,
            14,
            juce::Justification::left,
            false);
    }


    // ============================================================
    // Frequency
    // ============================================================

    constexpr double guideFrequencies[] =
    {
        100.0,
        1000.0,
        10000.0
    };

    constexpr const char* frequencyLabels[] =
    {
        "100Hz",
        "1kHz",
        "10kHz"
    };

    for (int i = 0; i < 3; ++i)
    {
        const float x =
            frequencyToX(
                guideFrequencies[i]);

        g.setColour(guideColour);

        g.drawVerticalLine(
            juce::roundToInt(x),
            bounds.getY(),
            bounds.getBottom());

        g.setColour(textColour);

        g.setFont(
            VisualStyle::getDefaultFont(
                VisualStyle::FontSize::small));

        g.drawText(
            frequencyLabels[i],
            juce::roundToInt(x - 25.0f),
            juce::roundToInt(
                bounds.getBottom() - 20.0f),
            50,
            14,
            juce::Justification::centred,
            false);
    }
}


//==============================================================================
// Three filter handles
//==============================================================================

void EQDrawingArea::drawBandButtons(juce::Graphics& g)
{
    const auto colour =
        VisualStyle::Palette::green;

    for (int i = 0; i < numBands; ++i)
    {
        const auto& band =
            state.bands[static_cast<size_t>(i)];

        const float x =
            frequencyToX(band.frequency);

        const float y =
            gainToY(band.gain);

        const bool selected =
            selectedBand == i;

        g.setColour(
            selected
                ? colour.highlight
                : colour.dim);

        g.fillEllipse(
            x - buttonRadius,
            y - buttonRadius,
            buttonRadius * 2.0f,
            buttonRadius * 2.0f);

        g.setColour(
            selected
                ? VisualStyle::background
                : VisualStyle::panelBackground);

        g.fillEllipse(
            x - buttonRadius + 3.0f,
            y - buttonRadius + 3.0f,
            (buttonRadius - 3.0f) * 2.0f,
            (buttonRadius - 3.0f) * 2.0f);
    }
}

//void EQDrawingArea::drawBandButtons(
//    juce::Graphics& g)
//{
//    const auto colour =
//        VisualStyle::Palette::green;
//
//    for (int i = 0; i < numBands; ++i)
//    {
//        const auto& band =
//            state.bands[
//                static_cast<size_t>(i)];
//
//        const float x =
//            frequencyToX(
//                band.frequency);
//
//        const float y =
//            gainToY(
//                calculateMagnitudeDB(
//                    band.frequency));
//
//        const bool selected =
//            selectedBand == i;
//
//        // Outer circle
//        g.setColour(
//            selected
//                ? colour.highlight
//                : colour.dim);
//
//        g.fillEllipse(
//            x - buttonRadius,
//            y - buttonRadius,
//            buttonRadius * 2.0f,
//            buttonRadius * 2.0f);
//
//        // Inner circle
//        g.setColour(
//            selected
//                ? VisualStyle::background
//                : VisualStyle::panelBackground);
//
//        g.fillEllipse(
//            x - buttonRadius + 3.0f,
//            y - buttonRadius + 3.0f,
//            (buttonRadius - 3.0f) * 2.0f,
//            (buttonRadius - 3.0f) * 2.0f);
//    }
//}


//==============================================================================
// EQ response calculation
//==============================================================================

float EQDrawingArea::calculateMagnitudeDB(
    double frequency) const
{
    double totalDB = 0.0;

    for (const auto& band : state.bands)
    {
        if (!band.enabled)
            continue;

        totalDB +=
            calculateBandMagnitudeDB(
                band,
                frequency);
    }

    return juce::jlimit(
        minGain,
        maxGain,
        static_cast<float>(totalDB));
}


float EQDrawingArea::calculateBandMagnitudeDB(
    const EQBand& band,
    double frequency) const
{
//    if (std::abs(band.gain) < 0.0001f &&
//        band.type != EQBandType::lowPass &&
//        band.type != EQBandType::highPass)
//    {
//        return 0.0f;
//    }

    const auto coefficients =
        makeCoefficients(band);

    const double magnitude =
        evaluateBiquadMagnitude(
            coefficients,
            frequency);

    if (magnitude <= 1.0e-12)
        return -120.0f;

    return static_cast<float>(
        20.0 * std::log10(magnitude));
}


//==============================================================================
// Biquad coefficient generation
//
// Standard RBJ-style equations.
//
// This is currently ONLY for drawing the response.
// The real-time DSP will be implemented separately.
//==============================================================================

EQDrawingArea::BiquadCoefficients
EQDrawingArea::makeCoefficients(
    const EQBand& band) const
{
    BiquadCoefficients c;

    constexpr double sampleRate = 48000.0;

    const double w0 =
        2.0
        * juce::MathConstants<double>::pi
        * band.frequency
        / sampleRate;

    const double cosW0 =
        std::cos(w0);

    const double sinW0 =
        std::sin(w0);

    const double A =
        std::pow(
            10.0,
            band.gain / 40.0);

    const double alpha =
        sinW0 / (2.0 * band.q);

    switch (band.type)
    {
        // ========================================================
        // Bell
        // ========================================================

        case EQBandType::bell:
        {
            c.b0 = 1.0 + alpha * A;
            c.b1 = -2.0 * cosW0;
            c.b2 = 1.0 - alpha * A;

            c.a0 = 1.0 + alpha / A;
            c.a1 = -2.0 * cosW0;
            c.a2 = 1.0 - alpha / A;

            break;
        }


        // ========================================================
        // Low-pass
        // ========================================================

        case EQBandType::lowPass:
        {
            c.b0 =
                (1.0 - cosW0) / 2.0;

            c.b1 =
                1.0 - cosW0;

            c.b2 =
                (1.0 - cosW0) / 2.0;

            c.a0 =
                1.0 + alpha;

            c.a1 =
                -2.0 * cosW0;

            c.a2 =
                1.0 - alpha;

            break;
        }


        // ========================================================
        // High-pass
        // ========================================================

        case EQBandType::highPass:
        {
            c.b0 =
                (1.0 + cosW0) / 2.0;

            c.b1 =
                -(1.0 + cosW0);

            c.b2 =
                (1.0 + cosW0) / 2.0;

            c.a0 =
                1.0 + alpha;

            c.a1 =
                -2.0 * cosW0;

            c.a2 =
                1.0 - alpha;

            break;
        }


        // ========================================================
        // Low shelf
        // ========================================================

        case EQBandType::lowShelf:
        {
            const double beta =
                2.0 * std::sqrt(A) * alpha;

            c.b0 =
                A *
                ((A + 1.0)
                - (A - 1.0) * cosW0
                + beta);

            c.b1 =
                2.0 * A *
                ((A - 1.0)
                - (A + 1.0) * cosW0);

            c.b2 =
                A *
                ((A + 1.0)
                - (A - 1.0) * cosW0
                - beta);

            c.a0 =
                (A + 1.0)
                + (A - 1.0) * cosW0
                + beta;

            c.a1 =
                -2.0 *
                ((A - 1.0)
                + (A + 1.0) * cosW0);

            c.a2 =
                (A + 1.0)
                + (A - 1.0) * cosW0
                - beta;

            break;
        }


        // ========================================================
        // High shelf
        // ========================================================

        case EQBandType::highShelf:
        {
            const double beta =
                2.0 * std::sqrt(A) * alpha;

            c.b0 =
                A *
                ((A + 1.0)
                + (A - 1.0) * cosW0
                + beta);

            c.b1 =
                -2.0 * A *
                ((A - 1.0)
                + (A + 1.0) * cosW0);

            c.b2 =
                A *
                ((A + 1.0)
                + (A - 1.0) * cosW0
                - beta);

            c.a0 =
                (A + 1.0)
                - (A - 1.0) * cosW0
                + beta;

            c.a1 =
                2.0 *
                ((A - 1.0)
                - (A + 1.0) * cosW0);

            c.a2 =
                (A + 1.0)
                - (A - 1.0) * cosW0
                - beta;

            break;
        }
    }

    return c;
}


//==============================================================================
// Evaluate biquad magnitude
//==============================================================================

double EQDrawingArea::evaluateBiquadMagnitude(
    const BiquadCoefficients& c,
    double frequency) const
{
    constexpr double sampleRate = 48000.0;

    const double omega =
        2.0
        * juce::MathConstants<double>::pi
        * frequency
        / sampleRate;

    const double cos1 =
        std::cos(omega);

    const double sin1 =
        -std::sin(omega);

    const double cos2 =
        std::cos(2.0 * omega);

    const double sin2 =
        -std::sin(2.0 * omega);


    const double numeratorR =
        c.b0
        + c.b1 * cos1
        + c.b2 * cos2;

    const double numeratorI =
        c.b1 * sin1
        + c.b2 * sin2;


    const double denominatorR =
        c.a0
        + c.a1 * cos1
        + c.a2 * cos2;

    const double denominatorI =
        c.a1 * sin1
        + c.a2 * sin2;


    const double numeratorMagnitude =
        std::sqrt(
            numeratorR * numeratorR
            + numeratorI * numeratorI);

    const double denominatorMagnitude =
        std::sqrt(
            denominatorR * denominatorR
            + denominatorI * denominatorI);

    if (denominatorMagnitude <= 1.0e-12)
        return 1.0;

    return numeratorMagnitude /
           denominatorMagnitude;
}


//==============================================================================
// Coordinate conversion
//==============================================================================

float EQDrawingArea::frequencyToX(
    double frequency) const
{
    const auto bounds =
        getPlotBounds();

    const double minLog =
        std::log10(minFrequency);

    const double maxLog =
        std::log10(maxFrequency);

    const double normalized =
        (std::log10(frequency) - minLog)
        / (maxLog - minLog);

    return bounds.getX()
         + static_cast<float>(
             normalized * bounds.getWidth());
}


double EQDrawingArea::xToFrequency(
    float x) const
{
    const auto bounds =
        getPlotBounds();

    const double normalized =
        juce::jlimit(
            0.0,
            1.0,
            static_cast<double>(
                (x - bounds.getX())
                / bounds.getWidth()));

    const double minLog =
        std::log10(minFrequency);

    const double maxLog =
        std::log10(maxFrequency);

    return std::pow(
        10.0,
        minLog
        + normalized * (maxLog - minLog));
}


float EQDrawingArea::gainToY(
    float gain) const
{
    const auto bounds =
        getPlotBounds();

    const float normalized =
        juce::jmap(
            juce::jlimit(
                minGain,
                maxGain,
                gain),
            minGain,
            maxGain,
            1.0f,
            0.0f);

    return bounds.getY()
         + normalized * bounds.getHeight();
}


float EQDrawingArea::yToGain(
    float y) const
{
    const auto bounds =
        getPlotBounds();

    const float normalized =
        juce::jlimit(
            0.0f,
            1.0f,
            (y - bounds.getY())
            / bounds.getHeight());

    return juce::jmap(
        normalized,
        1.0f,
        0.0f,
        minGain,
        maxGain);
}


//==============================================================================
// Geometry
//==============================================================================

juce::Rectangle<float>
EQDrawingArea::getPlotBounds() const
{
    return getLocalBounds()
        .toFloat()
        .reduced(plotPadding);
}


//==============================================================================
// Button hit testing
//==============================================================================

int EQDrawingArea::findButtonAt(
    const juce::Point<float>& position) const
{
    for (int i = 0; i < numBands; ++i)
    {
        const auto& band =
            state.bands[
                static_cast<size_t>(i)];

        const float x =
            frequencyToX(
                band.frequency);
        
        const float y =
            gainToY(band.gain);
        
//        const float y =
//            gainToY(
//                calculateMagnitudeDB(
//                    band.frequency));

        const float dx =
            position.x - x;

        const float dy =
            position.y - y;

        const float distanceSquared =
            dx * dx + dy * dy;

        if (distanceSquared
            <= buttonRadius * buttonRadius * 2.5f)
        {
            return i;
        }
    }

    return -1;
}


//==============================================================================
// Frequency separation
//==============================================================================

bool EQDrawingArea::isFrequencyAllowed(
    int band,
    float frequency) const
{
    const float x =
        frequencyToX(frequency);

    for (int i = 0; i < numBands; ++i)
    {
        if (i == band)
            continue;

        const auto& other =
            state.bands[
                static_cast<size_t>(i)];

        const float otherX =
            frequencyToX(other.frequency);

        if (std::abs(x - otherX)
            < minimumButtonSeparation)
        {
            return false;
        }
    }

    return true;
}


//==============================================================================
// Mouse interaction
//==============================================================================

void EQDrawingArea::mouseDown(
    const juce::MouseEvent& event)
{
    const int band =
        findButtonAt(event.position);

    if (band < 0)
        return;

    setSelectedBand(band);

    const auto& selected =
        state.bands[static_cast<size_t>(band)];

    dragOffset =
    {
        event.position.x -
            frequencyToX(selected.frequency),

        event.position.y -
            gainToY(selected.gain)
    };

    dragging = true;
}

//void EQDrawingArea::mouseDown(
//    const juce::MouseEvent& event)
//{
//    const int band =
//        findButtonAt(event.position);
//
//    if (band < 0)
//        return;
//
//    setSelectedBand(band);
//
//    dragging = true;
//}

void EQDrawingArea::mouseDrag(
    const juce::MouseEvent& event)
{
    if (!dragging || selectedBand < 0)
        return;

    const auto adjustedPosition =
        event.position - dragOffset;

    updateSelectedBandFromMouse(
        adjustedPosition);
}

//void EQDrawingArea::mouseDrag(
//    const juce::MouseEvent& event)
//{
//    if (!dragging ||
//        selectedBand < 0)
//    {
//        return;
//    }
//
//    updateSelectedBandFromMouse(
//        event.position);
//}


void EQDrawingArea::mouseUp(
    const juce::MouseEvent&)
{
    dragging = false;
}


void EQDrawingArea::mouseMove(
    const juce::MouseEvent&)
{
    // Nothing required for now.
}


void EQDrawingArea::mouseExit(
    const juce::MouseEvent&)
{
    // Nothing required for now.
}


//==============================================================================
// Update selected band
//==============================================================================

void EQDrawingArea::updateSelectedBandFromMouse(
    const juce::Point<float>& position)
{
    if (selectedBand < 0)
        return;

    auto& band =
        state.bands[
            static_cast<size_t>(selectedBand)];


    // ============================================================
    // Vertical movement -> gain
    // ============================================================

    band.gain =
        juce::jlimit(
            minGain,
            maxGain,
            yToGain(position.y));


    // ============================================================
    // Horizontal movement -> frequency
    // ============================================================

    const float proposedFrequency =
        static_cast<float>(
            xToFrequency(position.x));

    if (isFrequencyAllowed(
            selectedBand,
            proposedFrequency))
    {
        band.frequency =
            juce::jlimit(
                minFrequency,
                maxFrequency,
                proposedFrequency);
    }


    notifyEQChanged();

    repaint();
}


//==============================================================================
// EQ state
//==============================================================================

void EQDrawingArea::setEQState(
    const EQState& newState)
{
    state = newState;

    repaint();
}


//==============================================================================
// Selected band
//==============================================================================

void EQDrawingArea::setSelectedBand(
    int newSelectedBand) noexcept
{
    if (newSelectedBand < -1 ||
        newSelectedBand >= numBands)
    {
        newSelectedBand = -1;
    }

    if (selectedBand == newSelectedBand)
        return;

    selectedBand = newSelectedBand;

    if (onSelectedBandChanged)
        onSelectedBandChanged(selectedBand);

    repaint();
}


//==============================================================================
// Change notification
//==============================================================================

void EQDrawingArea::notifyEQChanged()
{
    if (onEQChanged)
        onEQChanged(state);
}


//==============================================================================
// Spectrum plotting
//==============================================================================

void EQDrawingArea::setSpectrumBuffer(
    SpectrumBuffer& buffer) noexcept
{
    spectrumBuffer = &buffer;
}


void EQDrawingArea::timerCallback()
{
    updateSpectrumVisualization();

    repaint();
}


void EQDrawingArea::updateSpectrumVisualization()
{
    if (spectrumBuffer == nullptr)
        return;

    inputSpectrum =
        spectrumBuffer->getInput();

    outputSpectrum =
        spectrumBuffer->getOutput();
}


//==============================================================================
// Spectrum coordinate conversion
//==============================================================================

float EQDrawingArea::spectrumDBToY(
    float dB) const
{
    const auto bounds =
        getPlotBounds();

    const float normalized =
        juce::jmap(
            juce::jlimit(
                spectrumMinDB,
                spectrumMaxDB,
                dB),
            spectrumMinDB,
            spectrumMaxDB,
            1.0f,
            0.0f);

    return bounds.getY()
         + normalized * bounds.getHeight();
}


//==============================================================================
// Spectrum
//==============================================================================

void EQDrawingArea::drawSpectrum(
    juce::Graphics& g)
{
    drawSpectrumLines(
        g,
        inputSpectrum,
        VisualStyle::Palette::blue.highlight);

    drawSpectrumLines(
        g,
        outputSpectrum,
        VisualStyle::Palette::yellow.highlight);
}


void EQDrawingArea::drawSpectrumLines(
    juce::Graphics& g,
    const Spectrum& spectrum,
    const juce::Colour& colour)
{
    const auto bounds =
        getPlotBounds();

    g.setColour(
        colour.withAlpha(0.45f));

    for (int i = 0;
         i < numSpectrumBands;
         ++i)
    {
        const float lowFrequency =
            SpectrumAnalyzer::
                getBandLowFrequency(i);

        const float highFrequency =
            SpectrumAnalyzer::
                getBandHighFrequency(i);

        const float x1 =
            frequencyToX(lowFrequency);

        const float x2 =
            frequencyToX(highFrequency);

        const float y =
            spectrumDBToY(
                spectrum[
                    static_cast<size_t>(i)]);

        g.drawHorizontalLine(
            juce::roundToInt(y),
            x1,
            x2);
    }
}
