///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 Mihai Ursu                                                 //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

/*
MultiSourceHandler.cpp

This file contains the sources for the acoustic multi source handler.
*/

#include "MultiSourceHandler.h"

#include <limits>


//!************************************************************************
//! Constructor
//!************************************************************************
MultiSourceHandler::MultiSourceHandler()
    : mNrOfSpeakers( MIN_NR_OF_SPEAKERS )
    , mFirstSpeakerAngleDeg( 0.0 )
{
}


//!************************************************************************
//! Recursive generation of microphone combinations
//! Used for getting the active mics for equidistant speakers.
//!
//! @returns nothing
//!************************************************************************
void MultiSourceHandler::generateMicrophoneCombinations
    (
    const int                       aOffset,        //!< offset index
    const int                       aActiveCount,   //!< nr of active mics
    std::vector<int>&               aCurrentList,   //!< current list of mics
    std::vector<std::vector<int>>&  aResultMatrix,  //!< result matrix of candidate mics
    const int                       aMicsCount      //!< total nr of mics
    )
{
    if( 0 == aActiveCount )
    {
        aResultMatrix.push_back( aCurrentList );
        return;
    }

    for( int i = aOffset; i < aMicsCount; i++ )
    {
        aCurrentList.push_back( i );
        generateMicrophoneCombinations( i + 1, aActiveCount - 1, aCurrentList, aResultMatrix, aMicsCount );
        aCurrentList.pop_back();
    }
}


//!************************************************************************
//! Get the best configuration of active microphones considering a number
//! of speakers placed at equal angles.
//!
//! The configuration is obtained for a frequency band (e.g. 300-3400 Hz),
//! and not just for a single value.
//!
//! The vector is filled with integers corresponding to the indexes from
//! MicArray::getArray().
//! Mic::getLabel() can be used if the EVAL-MICCANVASZ labels (1-15) are
//! needed.
//!
//! @returns the vector containing the best configuration of microphones
//!************************************************************************
std::vector<int> MultiSourceHandler::getBestConfigMicrophones
    (
    const AcousticsHandler::FrequencyRange  aFrequencyBand,     //!< frequency band
    const int                               aNumberOfSpeakers,  //!< number of speakers
    const double                            aThetaToMic5,       //!< angle [rad] of 1st speaker relative to mic 5
                                                                //!< in trigonometric sense
    MicArray                                aMicArray           //!< given mic array / starting point
    )
{
    std::vector<int> bestConfigMics;

    if( aNumberOfSpeakers >= 1 && aNumberOfSpeakers <= 6 )
    {
        const double D_MAX = 2 * aMicArray.getMaxRadius();
        const double PHI_MAX = Numeric::TWO_PI / 12.0;
        const double F_MIN = aFrequencyBand.min;
        const double F_MAX = aFrequencyBand.max;
        const double DELTA_F_MAX = PHI_MAX * AcousticsHandler::C / ( Numeric::TWO_PI * D_MAX );
        const int FREQUENCIES_COUNT = std::max( ceil( ( F_MAX - F_MIN ) / DELTA_F_MAX ), 3.0 );
        std::vector<double> fVec( FREQUENCIES_COUNT, 0 );

        for( int i = 0; i < FREQUENCIES_COUNT; i++ )
        {
            fVec[i] = F_MIN + i * ( F_MAX - F_MIN ) / ( FREQUENCIES_COUNT - 1 );
        }

        std::vector<double> thetasToMic3( aNumberOfSpeakers );

        for( int crtSpeaker = 0; crtSpeaker < aNumberOfSpeakers; crtSpeaker++ )
        {
            thetasToMic3[crtSpeaker] = Numeric::TWO_PI * crtSpeaker / aNumberOfSpeakers;
            thetasToMic3[crtSpeaker] += aThetaToMic5 + 0.5 * Numeric::PI;
        }

        int activeMicsCount = std::min( 2 * aNumberOfSpeakers + 1, static_cast<int>( aMicArray.getArray().size() ) );
        std::vector<std::vector<int>> candidateMics;
        std::vector<int> temp;
        generateMicrophoneCombinations( 0, activeMicsCount, temp, candidateMics, aMicArray.getArray().size() );
        double bestScore = -std::numeric_limits<double>::infinity();

        for( auto& comb : candidateMics )
        {
            double score = 0.0;

            for( double f : fVec )
            {
                CxMatrix A( aNumberOfSpeakers, CxVector( activeMicsCount ) );

                for( int s = 0; s < aNumberOfSpeakers; s++ )
                {
                    MicArray subMicArray;

                    for( int idx : comb )
                    {
                        subMicArray.addSingleMic( idx );
                    }

                    CxVector a;
                    Beamforming bf;
                    a = bf.calculateSteeringVector( f, thetasToMic3[s], subMicArray );

                    for( int m = 0; m < activeMicsCount; m++ )
                    {
                        A[s][m] = a[m];
                    }
                }

                Numeric* numericInstance = Numeric::getInstance();
                CxMatrix G;
                cdouble detG;

                if( numericInstance )
                {
                    G = numericInstance->calculateGramMatrix( A );
                    detG = numericInstance->calculateDeterminantGaussian( G );
                }

                score += log( abs( detG ) + 1e-12 );
            }

            score /= FREQUENCIES_COUNT;

            if( score > bestScore )
            {
                bestScore = score;
                bestConfigMics = comb;
            }
        }
    }
    else if( aNumberOfSpeakers >= 7 )
    {
        if( aMicArray.getArray().size() >= 15 )
        {
            for( size_t i = 0; i < aMicArray.getArray().size(); i++ )
            {
                bestConfigMics.push_back( i );
            }
        }
    }

    return bestConfigMics;
}


//!************************************************************************
//! Get the angle of first speaker having as reference mic 5
//! - speakers are placed at equal angles.
//! - the sense is trigonometric.
//!
//! @returns the value of the angle [deg]
//!************************************************************************
double MultiSourceHandler::getFirstSpeakerAngleDeg() const
{
    return mFirstSpeakerAngleDeg;
}


//!************************************************************************
//! Get the number of speakers
//!
//! @returns the number of speakers
//!************************************************************************
uint8_t MultiSourceHandler::getNrOfSpeakers() const
{
    return mNrOfSpeakers;
}


//!************************************************************************
//! Set the angle of first speaker having as reference mic 5
//! - see getFirstSpeakerAngleDeg() for more details.
//!
//! @returns true if the angle can be set
//!************************************************************************
bool MultiSourceHandler::setFirstSpeakerAngleDeg
    (
    const double aAngleDeg      //!< angle [deg]
    )
{
    bool status = aAngleDeg >= FIRST_SPEAKER_ANGLE_DEG_MIN
               && aAngleDeg <= FIRST_SPEAKER_ANGLE_DEG_MAX;

    if( status )
    {
        mFirstSpeakerAngleDeg = aAngleDeg;
    }

    return status;
}


//!************************************************************************
//! Set the number of speakers
//!
//! @returns true if the number can be set
//!************************************************************************
bool MultiSourceHandler::setNrOfSpeakers
    (
    const uint8_t aSpeakersNr   //!< nr of speakers
    )
{
    bool status = aSpeakersNr >= MIN_NR_OF_SPEAKERS
               && aSpeakersNr <= MAX_NR_OF_SPEAKERS;

    if( status )
    {
        mNrOfSpeakers = aSpeakersNr;
    }

    return status;
}
