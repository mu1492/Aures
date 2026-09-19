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
MultiSourceHandler.h

This file contains the definitions for the acoustic multi source handler.
*/

#ifndef MultiSourceHandler_h
#define MultiSourceHandler_h

#include "AcousticsHandler.h"
#include "Beamforming.h"

#include <cstdint>
#include <vector>


//************************************************************************
// Class for handling the acoustic multiple sources
//************************************************************************
class MultiSourceHandler
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        static const uint8_t MIN_NR_OF_SPEAKERS = 1;
        static const uint8_t MAX_NR_OF_SPEAKERS = 7;

        static constexpr double FIRST_SPEAKER_ANGLE_DEG_MIN = -180;
        static constexpr double FIRST_SPEAKER_ANGLE_DEG_MAX =  180;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        MultiSourceHandler();

        std::vector<int> getBestConfigMicrophones
            (
            const AcousticsHandler::FrequencyRange  aFrequencyBand,     //!< frequency band
            const int                               aNumberOfSpeakers,  //!< number of speakers
            const double                            aThetaToMic5,       //!< angle [rad] of 1st speaker relative to mic 5
                                                                        //!< in trigonometric sense
            MicArray                                aMicArray           //!< given mic array / starting point
            );

        double getFirstSpeakerAngleDeg() const;

        uint8_t getNrOfSpeakers() const;

        bool setFirstSpeakerAngleDeg
            (
            const double aAngleDeg      //!< angle [deg]
            );

        bool setNrOfSpeakers
            (
            const uint8_t aSpeakersNr   //!< nr of speakers
            );

    private:
        void generateMicrophoneCombinations
            (
            const int                       aOffset,        //!< offset index
            const int                       aActiveCount,   //!< nr of active mics
            std::vector<int>&               aCurrentList,   //!< current list of mics
            std::vector<std::vector<int>>&  aResultMatrix,  //!< result matrix of candidate mics
            const int                       aMicsCount      //!< total nr of mics
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        AcousticsHandler*           mAcousticsInstance;         //!< acoustics hndl

        uint8_t                     mNrOfSpeakers;              //!< number of speakers
        double                      mFirstSpeakerAngleDeg;      //!< angle [deg] of the 1st speaker related to mic 5, trigonometric sense
};

#endif // MultiSourceHandler_h
