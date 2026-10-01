/*
  ==============================================================================

    EQState.h
    Created: 9 Sep 2026 7:00:51am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

//==============================================================================
// Lightweight EQ snapshot used by the existing UI.
//
// APVTS is now the authoritative/persistent state. EQState is retained as a
// convenient representation for the drawing/UI code and as an adapter at the
// processor/UI boundary.
//==============================================================================

#pragma once

#include <array>
#include "../UI/EQBand.h"

//==============================================================================
// Complete EQ state
//==============================================================================

struct EQState
{
    std::array<EQBand, 3> bands =
    {
        EQBand
        {
            EQBandRole::left,
            EQBandType::lowShelf,
            100.0f,
            0.0f,
            0.707f,
            true
        },

        EQBand
        {
            EQBandRole::middle,
            EQBandType::bell,
            1000.0f,
            0.0f,
            0.707f,
            true
        },

        EQBand
        {
            EQBandRole::right,
            EQBandType::highShelf,
            8000.0f,
            0.0f,
            0.707f,
            true
        }
    };
};
