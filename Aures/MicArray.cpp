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
MicArray.cpp

This file contains the sources for the microphone array.
*/

#include "MicArray.h"

#include <cstring>


MicArray* MicArray::sInstance = nullptr;

//!************************************************************************
//! Constructor
//!************************************************************************
MicArray::MicArray()
{
    createGeometry();
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
MicArray* MicArray::getInstance()
{
    if( !sInstance )
    {
        sInstance = new MicArray;
    }

    return sInstance;
}


//!************************************************************************
//! Instance destroyer
//!
//! @returns nothing
//!************************************************************************
void MicArray::destroyInstance()
{
    delete sInstance;
    sInstance = nullptr;
}


//!************************************************************************
//! Create the mic array geometry
//!
//! @returns nothing
//!************************************************************************
void MicArray::createGeometry()
{
    Mic::Location micLocation;
    memset( &micLocation, 0, sizeof( micLocation ) );

    const double OUTER_ANGLE_STEP = 2.0 * PI / OUTER_MICS_COUNT;
    const double INNER_ANGLE_STEP = 2.0 * PI / INNER_MICS_COUNT;
    double crtAngle = 0;

    mMicArray.clear();
    Ics52000 crtMic;

    // outer radius => microphones "1" -> "8"
    for( size_t i = 0; i < OUTER_MICS_COUNT; i++ )
    {
        crtAngle = OUTER_START_ANGLE + i * OUTER_ANGLE_STEP;

        micLocation.x = OUTER_RADIUS * cos( crtAngle );
        micLocation.y = OUTER_RADIUS * sin( crtAngle );
        crtMic.setLocation( micLocation );

        mMicArray.push_back( crtMic );
    }

    // inner radius => microphones "9" -> "14"
    for( size_t i = 0; i < INNER_MICS_COUNT; i++ )
    {
        crtAngle = INNER_START_ANGLE + i * INNER_ANGLE_STEP;

        micLocation.x = INNER_RADIUS * cos( crtAngle );
        micLocation.y = INNER_RADIUS * sin( crtAngle );
        crtMic.setLocation( micLocation );

        mMicArray.push_back( crtMic );
    }

    // center => microphone "15"
    memset( &micLocation, 0, sizeof( micLocation ) );
    crtMic.setLocation( micLocation );
    mMicArray.push_back( crtMic );
}


//!************************************************************************
//! Get the mic array
//!
//! @returns the mic array
//!************************************************************************
std::vector<Ics52000> MicArray::getArray() const
{
    return mMicArray;
}
