/*
  ==============================================================================

    ParametricEQ.h
    Created: 15 Sep 2026 6:42:30am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cassert>
#include "../UI/EQBand.h"


class ParametricEQ
{
public:

    // ================================================================
    // Configuration
    // ================================================================

    static constexpr std::size_t NumBands = 3;

    struct Band
    {
        float frequency = 1000.0f;
        float gain      = 0.0f;
        float q         = 0.707f;
        bool enabled    = true;
        EQBandType type = EQBandType::bell;
    };


    // ================================================================
    // Construction
    // ================================================================

    ParametricEQ() = default;


    // ================================================================
    // Preparation
    // ================================================================

    void prepare(double sampleRate)
    {
        assert(sampleRate > 0.0);

        sampleRate_ = sampleRate;
        
        constexpr float smoothingTimeSeconds = 0.020f; // 20 ms

        smoothingCoeff_ =
            1.0f - std::exp(
                -1.0f /
                (static_cast<float>(sampleRate_) *
                 smoothingTimeSeconds));

        for (std::size_t i = 0; i < NumBands; ++i)
        {
            currentFrequency_[i] =
                targetFrequency_[i].load(std::memory_order_relaxed);

            currentGain_[i] =
                targetGain_[i].load(std::memory_order_relaxed);

            currentQ_[i] =
                targetQ_[i].load(std::memory_order_relaxed);
            

            filters_[i].reset();

            filters_[i].updateCoefficients(
                targetType_[i].load(std::memory_order_relaxed),
                currentFrequency_[i],
                currentQ_[i],
                currentGain_[i],
                sampleRate_);
        }

        prepared_ = true;
    }


    bool isPrepared() const noexcept
    {
        return prepared_;
    }


    // ================================================================
    // Parameters
    //
    // These functions are safe to call from the GUI thread while the
    // audio thread is running.
    // ================================================================

    void setFrequency(std::size_t band, float frequency)
    {
        if (band >= NumBands)
            return;

        targetFrequency_[band].store(
            clampFrequency(frequency),
            std::memory_order_relaxed);
    }


    void setGain(std::size_t band, float gainDb)
    {
        if (band >= NumBands)
            return;
        
        targetGain_[band].store(
            clampGain(gainDb),
            std::memory_order_relaxed);
    }


    void setQ(std::size_t band, float q)
    {
        if (band >= NumBands)
            return;

        targetQ_[band].store(
            clampQ(q),
            std::memory_order_relaxed);
    }
    
    void setType(std::size_t band, EQBandType type)
    {
        if (band >= NumBands)
            return;

        targetType_[band].store(
            type,
            std::memory_order_relaxed);
    }


    void setBand(std::size_t band, const Band& parameters)
    {
        if (band >= NumBands)
            return;

        setFrequency(band, parameters.frequency);
        setGain(band, parameters.gain);
        setQ(band, parameters.q);
        setType(band, parameters.type);
    }


    float getFrequency(std::size_t band) const
    {
        if (band >= NumBands)
            return 0.0f;

        return targetFrequency_[band].load(
            std::memory_order_relaxed);
    }


    float getGain(std::size_t band) const
    {
        if (band >= NumBands)
            return 0.0f;

        return targetGain_[band].load(
            std::memory_order_relaxed);
    }


    float getQ(std::size_t band) const
    {
        if (band >= NumBands)
            return 0.0f;

        return targetQ_[band].load(
            std::memory_order_relaxed);
    }


    // ================================================================
    // Processing
    // ================================================================

    void process(float* samples, std::size_t numSamples)
    {
        if (!prepared_ ||
            samples == nullptr ||
            numSamples == 0)
        {
            return;
        }


        // ------------------------------------------------------------
        // Smooth parameters over the block.
        //
        // We use the same exponential smoothing idea as the previous
        // GraphicEQ implementation, but now for frequency, gain and Q.
        // ------------------------------------------------------------

        const float blockSmoothing =
            1.0f -
            std::pow(
                1.0f - smoothingCoeff_,
                static_cast<float>(numSamples));

        for (std::size_t band = 0;
             band < NumBands;
             ++band)
        {
            const float targetFrequency =
                targetFrequency_[band].load(
                    std::memory_order_relaxed);

            const float targetGain =
                targetGain_[band].load(
                    std::memory_order_relaxed);

            const float targetQ =
                targetQ_[band].load(
                    std::memory_order_relaxed);


            currentFrequency_[band] +=
                blockSmoothing *
                (targetFrequency -
                 currentFrequency_[band]);


            currentGain_[band] +=
                blockSmoothing *
                (targetGain -
                 currentGain_[band]);


            currentQ_[band] +=
                blockSmoothing *
                (targetQ -
                 currentQ_[band]);


            filters_[band].updateCoefficients(
                targetType_[band].load(std::memory_order_relaxed),
                currentFrequency_[band],
                currentQ_[band],
                currentGain_[band],
                sampleRate_);
        }


        // ------------------------------------------------------------
        // Process audio.
        // ------------------------------------------------------------

        for (std::size_t n = 0;
             n < numSamples;
             ++n)
        {
            float x = samples[n];

            for (std::size_t band = 0;
                 band < NumBands;
                 ++band)
            {
                x = filters_[band].process(x);
            }

            samples[n] = x;
        }
    }


    // ================================================================
    // Reset
    // ================================================================

    void reset()
    {
        if (!prepared_)
            return;
        
//        setType(0, EQBandType::lowShelf);
//        setType(1, EQBandType::bell);
//        setType(2, EQBandType::highShelf);
        
        for (std::size_t i = 0;
             i < NumBands;
             ++i)
        {
            targetFrequency_[i].store(
                currentFrequency_[i],
                std::memory_order_relaxed);

            targetGain_[i].store(
                0.0f,
                std::memory_order_relaxed);

            targetQ_[i].store(
                currentQ_[i],
                std::memory_order_relaxed);

            currentGain_[i] = 0.0f;

            filters_[i].reset();

            filters_[i].updateCoefficients(
                targetType_[i].load(std::memory_order_relaxed),
                currentFrequency_[i],
                currentQ_[i],
                0.0f,
                sampleRate_);
        }
    }


private:

    // ================================================================
    // Biquad
    // ================================================================

    class Biquad
    {
    public:

        void updateCoefficients(
            EQBandType type,
            float frequency,
            float Q,
            float gainDb,
            double sampleRate)
        {
            constexpr float pi =
                3.14159265358979323846f;

            const float omega =
                2.0f *
                pi *
                frequency /
                static_cast<float>(sampleRate);

            const float sinOmega =
                std::sin(omega);

            const float cosOmega =
                std::cos(omega);


            // ================================================================
            // Coefficients before normalization by a0
            // ================================================================

            float b0 = 0.0f;
            float b1 = 0.0f;
            float b2 = 0.0f;

            float a0 = 1.0f;
            float a1 = 0.0f;
            float a2 = 0.0f;


            // ================================================================
            // Select filter type
            // ================================================================

            switch (type)
            {
                // ------------------------------------------------------------
                // Bell / peaking EQ
                // ------------------------------------------------------------

                case EQBandType::bell:
                {
                    const float A =
                        std::pow(
                            10.0f,
                            gainDb / 40.0f);

                    const float alpha =
                        sinOmega /
                        (2.0f * Q);

                    b0 =
                        1.0f +
                        alpha * A;

                    b1 =
                        -2.0f *
                        cosOmega;

                    b2 =
                        1.0f -
                        alpha * A;

                    a0 =
                        1.0f +
                        alpha / A;

                    a1 =
                        -2.0f *
                        cosOmega;

                    a2 =
                        1.0f -
                        alpha / A;

                    break;
                }


                // ------------------------------------------------------------
                // Low-pass
                // ------------------------------------------------------------

                case EQBandType::lowPass:
                {
                    const float alpha =
                        sinOmega /
                        (2.0f * Q);

                    b0 =
                        (1.0f - cosOmega) /
                        2.0f;

                    b1 =
                        1.0f - cosOmega;

                    b2 =
                        (1.0f - cosOmega) /
                        2.0f;

                    a0 =
                        1.0f + alpha;

                    a1 =
                        -2.0f *
                        cosOmega;

                    a2 =
                        1.0f - alpha;

                    break;
                }


                // ------------------------------------------------------------
                // High-pass
                // ------------------------------------------------------------

                case EQBandType::highPass:
                {
                    const float alpha =
                        sinOmega /
                        (2.0f * Q);

                    b0 =
                        (1.0f + cosOmega) /
                        2.0f;

                    b1 =
                        -(1.0f + cosOmega);

                    b2 =
                        (1.0f + cosOmega) /
                        2.0f;

                    a0 =
                        1.0f + alpha;

                    a1 =
                        -2.0f *
                        cosOmega;

                    a2 =
                        1.0f - alpha;

                    break;
                }


                // ------------------------------------------------------------
                // Low shelf
                // ------------------------------------------------------------

                case EQBandType::lowShelf:
                {
                    const float A =
                        std::pow(
                            10.0f,
                            gainDb / 40.0f);

                    const float sqrtA =
                        std::sqrt(A);

                    const float alpha =
                        sinOmega /
                        (2.0f * Q);

                    const float beta =
                        2.0f *
                        sqrtA *
                        alpha;

                    b0 =
                        A *
                        ((A + 1.0f)
                         - (A - 1.0f) * cosOmega
                         + beta);

                    b1 =
                        2.0f *
                        A *
                        ((A - 1.0f)
                         - (A + 1.0f) * cosOmega);

                    b2 =
                        A *
                        ((A + 1.0f)
                         - (A - 1.0f) * cosOmega
                         - beta);

                    a0 =
                        (A + 1.0f)
                        + (A - 1.0f) * cosOmega
                        + beta;

                    a1 =
                        -2.0f *
                        ((A - 1.0f)
                         + (A + 1.0f) * cosOmega);

                    a2 =
                        (A + 1.0f)
                        + (A - 1.0f) * cosOmega
                        - beta;

                    break;
                }


                // ------------------------------------------------------------
                // High shelf
                // ------------------------------------------------------------

                case EQBandType::highShelf:
                {
                    const float A =
                        std::pow(
                            10.0f,
                            gainDb / 40.0f);

                    const float sqrtA =
                        std::sqrt(A);

                    const float alpha =
                        sinOmega /
                        (2.0f * Q);

                    const float beta =
                        2.0f *
                        sqrtA *
                        alpha;

                    b0 =
                        A *
                        ((A + 1.0f)
                         + (A - 1.0f) * cosOmega
                         + beta);

                    b1 =
                        -2.0f *
                        A *
                        ((A - 1.0f)
                         + (A + 1.0f) * cosOmega);

                    b2 =
                        A *
                        ((A + 1.0f)
                         + (A - 1.0f) * cosOmega
                         - beta);

                    a0 =
                        (A + 1.0f)
                        - (A - 1.0f) * cosOmega
                        + beta;

                    a1 =
                        2.0f *
                        ((A - 1.0f)
                         - (A + 1.0f) * cosOmega);

                    a2 =
                        (A + 1.0f)
                        - (A - 1.0f) * cosOmega
                        - beta;

                    break;
                }
            }


            // ================================================================
            // Normalize by a0
            // ================================================================

            const float invA0 =
                1.0f / a0;

            b0_ = b0 * invA0;
            b1_ = b1 * invA0;
            b2_ = b2 * invA0;

            a1_ = a1 * invA0;
            a2_ = a2 * invA0;
        }


        float process(float x)
        {
            // Direct Form II Transposed

            const float y =
                b0_ * x + z1_;


            z1_ =
                b1_ * x
                - a1_ * y
                + z2_;


            z2_ =
                b2_ * x
                - a2_ * y;


            return y;
        }


        void reset()
        {
            z1_ = 0.0f;
            z2_ = 0.0f;
        }


    private:

        float b0_ = 1.0f;
        float b1_ = 0.0f;
        float b2_ = 0.0f;

        float a1_ = 0.0f;
        float a2_ = 0.0f;

        float z1_ = 0.0f;
        float z2_ = 0.0f;
    };


    // ================================================================
    // Helpers
    // ================================================================

    static float clampFrequency(float frequency)
    {
        return std::clamp(
            frequency,
            20.0f,
            20000.0f);
    }


    static float clampGain(float gainDb)
    {
        return std::clamp(
            gainDb,
            -12.0f,
            12.0f);
    }


    static float clampQ(float q)
    {
        return std::clamp(
            q,
            0.1f,
            10.0f);
    }


    // ================================================================
    // Members
    // ================================================================

    double sampleRate_ = 0.0;

    std::array<std::atomic<float>, NumBands>
        targetFrequency_
        {
            100.0f,
            1000.0f,
            8000.0f
        };

    std::array<std::atomic<float>, NumBands>
        targetGain_
        {
            0.0f,
            0.0f,
            0.0f
        };

    std::array<std::atomic<float>, NumBands>
        targetQ_
        {
            0.707f,
            0.707f,
            0.707f
        };


    std::array<float, NumBands>
        currentFrequency_{};

    std::array<float, NumBands>
        currentGain_{};

    std::array<float, NumBands>
        currentQ_{};
    
    std::array<std::atomic<EQBandType>, NumBands>
        targetType_
        {
            EQBandType::lowShelf,
            EQBandType::bell,
            EQBandType::highShelf
        };


    std::array<Biquad, NumBands>
        filters_;


    float smoothingCoeff_ = 0.001f;

    bool prepared_ = false;
};
