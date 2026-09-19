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


//!************************************************************************
//! Constructor
//!************************************************************************
MicArray::MicArray()
{
}


//!************************************************************************
//! Add a single microphone to the array geometry
//!
//! @returns nothing
//!************************************************************************
bool MicArray::addSingleMic
    (
    const size_t aIndex         //!< mic index [0 .. TOTAL_MICS_COUNT-1]
    )
{
    bool status = aIndex < TOTAL_MICS_COUNT;

    if( status )
    {
        const int MIC_LABEL = aIndex + 1;
        bool alreadyAdded = false;

        // make sure this mic was not already added
        for( size_t i = 0; i < mMicArray.size(); i++ )
        {
            if( MIC_LABEL == mMicArray.at( i ).getLabel() )
            {
                alreadyAdded = true;
                break;
            }
        }

        if( !alreadyAdded )
        {
            Ics52000 crtMic;
            crtMic.setLabel( MIC_LABEL );

            Mic::XyzLocation micXyzLocation;
            memset( &micXyzLocation, 0, sizeof( micXyzLocation ) );

            const double OUTER_ANGLE_STEP = Numeric::TWO_PI / OUTER_CIRCLE.micsCount;
            const double INNER_ANGLE_STEP = Numeric::TWO_PI / INNER_CIRCLE.micsCount;
            double crtAngle = 0;
            const double EPS = 1.e-5;

            if( aIndex < OUTER_CIRCLE.micsCount )
            {
                crtAngle = OUTER_START_ANGLE + aIndex * OUTER_ANGLE_STEP;

                micXyzLocation.x = OUTER_CIRCLE.radius * cos( crtAngle );
                micXyzLocation.y = OUTER_CIRCLE.radius * sin( crtAngle );

                if( abs( micXyzLocation.x ) < EPS )
                {
                    micXyzLocation.x = 0;
                }

                if( abs( micXyzLocation.y ) < EPS )
                {
                    micXyzLocation.y = 0;
                }

                crtMic.setXyzLocation( micXyzLocation );

                crtMic.setCcaLocation( Mic::CCA_LOCATION_OUTER_CIRCLE );
            }
            else if( aIndex < OUTER_CIRCLE.micsCount + INNER_CIRCLE.micsCount )
            {
                crtAngle = INNER_START_ANGLE + aIndex * INNER_ANGLE_STEP;

                micXyzLocation.x = INNER_CIRCLE.radius * cos( crtAngle );
                micXyzLocation.y = INNER_CIRCLE.radius * sin( crtAngle );

                if( abs( micXyzLocation.x ) < EPS )
                {
                    micXyzLocation.x = 0;
                }

                if( abs( micXyzLocation.y ) < EPS )
                {
                    micXyzLocation.y = 0;
                }

                crtMic.setXyzLocation( micXyzLocation );

                crtMic.setCcaLocation( Mic::CCA_LOCATION_INNER_CIRCLE );
            }
            else if( OUTER_CIRCLE.micsCount + INNER_CIRCLE.micsCount == aIndex )
            {
                crtMic.setXyzLocation( micXyzLocation );
                crtMic.setCcaLocation( Mic::CCA_LOCATION_CENTER );
            }

            mMicArray.push_back( crtMic );
        }
    }

    return status;
}


//!************************************************************************
//! Clear the mic array
//!
//! @returns nothing
//!************************************************************************
void MicArray::clearArray()
{
    mMicArray.clear();
}


//!************************************************************************
//! Create from scratch the entire array geometry for all 15 mics
//! It will delete all existing mics.
//!
//! @returns nothing
//!************************************************************************
void MicArray::createFullGeometry()
{
    Mic::XyzLocation micXyzLocation;
    memset( &micXyzLocation, 0, sizeof( micXyzLocation ) );

    const double OUTER_ANGLE_STEP = Numeric::TWO_PI / OUTER_CIRCLE.micsCount;
    const double INNER_ANGLE_STEP = Numeric::TWO_PI / INNER_CIRCLE.micsCount;
    double crtAngle = 0;
    const double EPS = 1.e-5;

    mMicArray.clear();
    Ics52000 crtMic;
    int crtLabel = 0;    

    // outer radius => microphones "1" -> "8"
    for( size_t i = 0; i < OUTER_CIRCLE.micsCount; i++ )
    {
        crtMic.setLabel( ++crtLabel );

        crtAngle = OUTER_START_ANGLE + i * OUTER_ANGLE_STEP;

        micXyzLocation.x = OUTER_CIRCLE.radius * cos( crtAngle );
        micXyzLocation.y = OUTER_CIRCLE.radius * sin( crtAngle );

        if( abs( micXyzLocation.x ) < EPS )
        {
            micXyzLocation.x = 0;
        }

        if( abs( micXyzLocation.y ) < EPS )
        {
            micXyzLocation.y = 0;
        }

        crtMic.setXyzLocation( micXyzLocation );

        crtMic.setCcaLocation( Mic::CCA_LOCATION_OUTER_CIRCLE );

        mMicArray.push_back( crtMic );
    }

    // inner radius => microphones "9" -> "14"
    for( size_t i = 0; i < INNER_CIRCLE.micsCount; i++ )
    {
        crtMic.setLabel( ++crtLabel );

        crtAngle = INNER_START_ANGLE + i * INNER_ANGLE_STEP;

        micXyzLocation.x = INNER_CIRCLE.radius * cos( crtAngle );
        micXyzLocation.y = INNER_CIRCLE.radius * sin( crtAngle );

        if( abs( micXyzLocation.x ) < EPS )
        {
            micXyzLocation.x = 0;
        }

        if( abs( micXyzLocation.y ) < EPS )
        {
            micXyzLocation.y = 0;
        }

        crtMic.setXyzLocation( micXyzLocation );

        crtMic.setCcaLocation( Mic::CCA_LOCATION_INNER_CIRCLE );

        mMicArray.push_back( crtMic );
    }

    // center => microphone "15"
    crtMic.setLabel( ++crtLabel );
    memset( &micXyzLocation, 0, sizeof( micXyzLocation ) );
    crtMic.setXyzLocation( micXyzLocation );
    crtMic.setCcaLocation( Mic::CCA_LOCATION_CENTER );
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


//!************************************************************************
//! Get the vector of indexes for microphones in the array
//!
//! @returns the vector of indexes
//!************************************************************************
std::vector<size_t> MicArray::getMicIndexes() const
{
    std::vector<size_t> idxVec;

    for( size_t i = 0; i < mMicArray.size(); i++ )
    {
        idxVec.push_back( mMicArray.at( i ).getLabel() - 1 );
    }

    return idxVec;
}


//!************************************************************************
//! Get the mic array geometry
//!
//! @returns the vector with coordinates for all mics in the array
//!************************************************************************
std::vector<Mic::XyzLocation> MicArray::getGeometry() const
{
    std::vector<Mic::XyzLocation> geomVec;

    for( size_t i = 0; i < mMicArray.size(); i++ )
    {
        geomVec.push_back( mMicArray.at( i ).getXyzLocation() );
    }

    return geomVec;
}


//!************************************************************************
//! Get the radius of the most outer circle where mics exist [m]
//!
//! @returns The radius of the most outer circle
//!************************************************************************
double MicArray::getMaxRadius() const
{
    double rMax = 0;
    size_t i = 0;

    for( i = 0; i < mMicArray.size(); i++ )
    {
        if( Mic::CCA_LOCATION_OUTER_CIRCLE == mMicArray.at( i ).getCcaLocation() )
        {
            rMax = OUTER_CIRCLE.radius;
            break;
        }
    }

    if( !rMax )
    {
        for( i = 0; i < mMicArray.size(); i++ )
        {
            if( Mic::CCA_LOCATION_INNER_CIRCLE == mMicArray.at( i ).getCcaLocation() )
            {
                rMax = INNER_CIRCLE.radius;
                break;
            }
        }
    }

    return rMax;
}


//!************************************************************************
//! Get the radius of the circle on which the mic is placed [m]
//!
//! @returns The radius of the circle
//!************************************************************************
double MicArray::getRadius
    (
    const size_t aIndex   //!< mic index
    ) const
{
    double r = 0;

    if( isOnOuterCircle( aIndex ) )
    {
        r = OUTER_CIRCLE.radius;
    }
    else if( isOnInnerCircle( aIndex ) )
    {
        r = INNER_CIRCLE.radius;
    }

    return r;
}


//!************************************************************************
//! Get the in use status of a microphone
//!
//! @returns true if the mic is actively used in the array
//!************************************************************************
bool MicArray::isInUse
    (
    const size_t aIndex     //!< mic index
    )
{
    bool status = aIndex < mMicArray.size();

    if( status )
    {
        status = mMicArray.at( aIndex ).isInUse();
    }

    return status;
}


//!************************************************************************
//! Check if a microphone is on the outer circle
//!
//! @returns true if the mic is on the outer circle
//!************************************************************************
bool MicArray::isOnOuterCircle
    (
    const size_t aIndex   //!< mic index
    ) const
{
    bool status = aIndex < mMicArray.size();

    if( status )
    {
        status = ( Mic::CCA_LOCATION_OUTER_CIRCLE == mMicArray.at( aIndex ).getCcaLocation() );
    }

    return status;
}


//!************************************************************************
//! Check if a microphone is on the inner circle
//!
//! @returns true if the mic is on the inner circle
//!************************************************************************
bool MicArray::isOnInnerCircle
    (
    const size_t aIndex   //!< mic index
    ) const
{
    bool status = aIndex < mMicArray.size();

    if( status )
    {
        status = ( Mic::CCA_LOCATION_INNER_CIRCLE == mMicArray.at( aIndex ).getCcaLocation() );
    }

    return status;
}


//!************************************************************************
//! Check if a microphone is on the center
//!
//! @returns true if the mic is on the center
//!************************************************************************
bool MicArray::isOnCenter
    (
    const size_t aIndex   //!< mic index
    ) const
{
    bool status = aIndex < mMicArray.size();

    if( status )
    {
        status = ( Mic::CCA_LOCATION_CENTER == mMicArray.at( aIndex ).getCcaLocation() );
    }

    return status;
}


//!************************************************************************
//! Set the in use status of a microphone
//!
//! @returns true if the in use status can be changed
//!************************************************************************
bool MicArray::setInUse
    (
    const size_t aIndex,    //!< mic index
    const bool   aStatus    //!< in use status
    )
{
    bool status = aIndex < mMicArray.size();

    if( status )
    {
        mMicArray.at( aIndex ).setInUse( aStatus );
    }

    return status;
}
