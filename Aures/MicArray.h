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
MicArray.h

This file contains the definitions for the microphone array.
*/

#ifndef MicArray_h
#define MicArray_h

#include "Ics52000.h"

#include <cmath>
#include <cstdint>
#include <vector>


//************************************************************************
// Class for handling the microphone array
//************************************************************************
class MicArray
{
    //************************************************************************
    // constants and types
    //************************************************************************
    private:
        static constexpr double PI = 4.0 * atan( 1.0 );

        // EVAL-MICCANVASZ geometry
        // https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-miccanvasz.html
        //
        //            ^  y
        //            |
        //            |
        //
        //            5
        //      6           4
        //          13  12
        //                              x
        //   7   14   15   11   3  ----->
        //
        //          9   10
        //      8           2
        //            1
        //
        static constexpr double OUTER_RADIUS = 0.02;    // [m]
        static constexpr double INNER_RADIUS = 0.01;    // [m]

        static const uint8_t TOTAL_MICS_COUNT = 15;
        static const uint8_t OUTER_MICS_COUNT = 8;
        static const uint8_t INNER_MICS_COUNT = 6;

        static constexpr double OUTER_START_ANGLE = -PI / 2.0;       // [rad]
        static constexpr double INNER_START_ANGLE = -PI * 2.0 / 3.0; // [rad]


    //************************************************************************
    // functions
    //************************************************************************
    public:
        MicArray();

        static MicArray* getInstance();

        static void destroyInstance();

        std::vector<Ics52000> getArray() const;

    private:
        void createGeometry();


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static MicArray*        sInstance;      //!< singleton

        std::vector<Ics52000>   mMicArray;      //!< mic array
};

#endif // MicArray_h
