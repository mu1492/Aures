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
#include "Numeric.h"

#include <cmath>
#include <cstdint>
#include <vector>

#include <QMetaType>


//************************************************************************
// Class for handling the microphone array
//************************************************************************
class MicArray
{
    //************************************************************************
    // constants and types
    //************************************************************************    
    public:
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
        //   7   14   15   11   3  ----->     DOA = 0 [rad]
        //                                    (Direction Of Arrival)
        //          9   10
        //      8           2
        //            1
        //

        // UCA - Uniform Circular Array
        typedef struct
        {
            double radius;      //!< radius [m]
            uint8_t micsCount;  //!< number of microphones
        }Uca;

        static constexpr Uca OUTER_CIRCLE = { 0.02, 8 };
        static constexpr Uca INNER_CIRCLE = { 0.01, 6 };

        static const uint8_t TOTAL_MICS_COUNT = OUTER_CIRCLE.micsCount
                                              + INNER_CIRCLE.micsCount
                                              + 1;

    private:
        static constexpr double OUTER_START_ANGLE = -Numeric::PI / 2.0;       // [rad]
        static constexpr double INNER_START_ANGLE = -Numeric::PI * 2.0 / 3.0; // [rad]


    //************************************************************************
    // functions
    //************************************************************************
    public:
        MicArray();

        bool addSingleMic
            (
            const size_t aIndex         //!< mic index [0 .. TOTAL_MICS_COUNT-1]
            );

        void clearArray();

        void createFullGeometry();

        std::vector<Ics52000> getArray() const;

        std::vector<Mic::XyzLocation> getGeometry() const;

        double getMaxRadius() const;

        std::vector<size_t> getMicIndexes() const;

        double getRadius
            (
            const size_t aIndex         //!< mic index
            ) const;

        bool isInUse
            (
            const size_t aIndex         //!< mic index
            );

        bool isOnOuterCircle
            (
            const size_t aIndex         //!< mic index
            ) const;

        bool isOnInnerCircle
            (
            const size_t aIndex         //!< mic index
            ) const;

        bool isOnCenter
            (
            const size_t aIndex         //!< mic index
            ) const;

        bool setInUse
            (
            const size_t    aIndex,     //!< mic index
            const bool      aStatus     //!< in use status
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        std::vector<Ics52000>   mMicArray;      //!< mic array
};

Q_DECLARE_METATYPE( MicArray )

#endif // MicArray_h
