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
Mic.cpp

This file contains the sources for a microphone.
*/

#include "Mic.h"
#include "AcousticsHandler.h"

#include <cstring>


//!************************************************************************
//! Constructor
//!************************************************************************
Mic::Mic
    (
    FreqRange3db    aFrequencyRange,    //!< frequency range @3dB [Hz]
    double          aThd,               //!< THD [-]
    double          aSensitivity,       //!< sensitivity [dB FS]
    double          aSnr,               //!< SNR [dBA]
    double          aAop                //!< AOP [dB SPL]
    )
    : mLabel( 0 )
    , mInUse( true )
    , mCcaLocation( CCA_LOCATION_UNKNOWN )
    , mThd( 0 )
    , mSensitivity( -100 )
    , mSnr( 0 )
    , mAop( 0 )
{
    memset( &mXyzLocation, 0, sizeof( mXyzLocation ) );

    if( aFrequencyRange.fmin > 0
     && aFrequencyRange.fmax > aFrequencyRange.fmin )
    {
        memcpy( &mFrequencyRange, &aFrequencyRange, sizeof( mFrequencyRange ) );
    }
    else
    {
        memset( &mFrequencyRange, 0, sizeof( mFrequencyRange ) );
    }

    if( aThd > 0 && aThd < 1 )
    {
        aThd = aThd;
    }

    if( aSensitivity > -100 && aSensitivity < 0 )
    {
        mSensitivity = aSensitivity;
    }

    if( aSnr > 0 )
    {
        mSnr = aSnr;
    }

    if( aAop > 0 )
    {
        mAop = aAop;
    }

    mEin = AcousticsHandler::REF_SPL - mSnr;
    mNf = mSensitivity - mSnr;
    mDynRng = mAop - mEin;
}


//!************************************************************************
//! Get the AOP (Acoustic Overload Point) [dB SPL]
//!
//! @returns The AOP
//!************************************************************************
double Mic::getAop() const
{
    return mAop;
}


//!************************************************************************
//! Get the Circular Concentric Array location
//!
//! @returns The CCA mic location
//!************************************************************************
Mic::CcaLocation Mic::getCcaLocation() const
{
    return mCcaLocation;
}


//!************************************************************************
//! Get the dynamic range [dB SPL]
//!
//! @returns The dynamic range
//!************************************************************************
double Mic::getDynRng() const
{
    return mDynRng;
}


//!************************************************************************
//! Get the EIN (Equivalent Input Noise) [dB SPL]
//!
//! @returns The EIN
//!************************************************************************
double Mic::getEin() const
{
    return mEin;
}


//!************************************************************************
//! Get the integer label that matches EVAL-MICCANVASZ numbering
//!
//! @returns nothing
//!************************************************************************
int Mic::getLabel() const
{
    return mLabel;
}


//!************************************************************************
//! Get the noise floor [dB FS]
//!
//! @returns The noise floor
//!************************************************************************
double Mic::getNf() const
{
    return mNf;
}


//!************************************************************************
//! Get the sensitivity [dB FS]
//!
//! @returns The sensitivity
//!************************************************************************
double Mic::getSensitivity() const
{
    return mSensitivity;
}


//!************************************************************************
//! Get the SNR (Signal to Noise Ratio) [dBA]
//!
//! @returns The SNR
//!************************************************************************
double Mic::getSnr() const
{
    return mSnr;
}


//!************************************************************************
//! Get the 3D location
//!
//! @returns The 3D mic location
//!************************************************************************
Mic::XyzLocation Mic::getXyzLocation() const
{
    return mXyzLocation;
}


//!************************************************************************
//! Get the in use status
//!
//! @returns true if the mic is actively used in the array
//!************************************************************************
bool Mic::isInUse() const
{
    return mInUse;
}


//!************************************************************************
//! Set the CCA location
//!
//! @returns nothing
//!************************************************************************
void Mic::setCcaLocation
    (
    const CcaLocation  aCcaLocation      //!< CCA location
    )
{
    if( aCcaLocation < CCA_LOCATION_MAX_KNOWN )
    {
        mCcaLocation = aCcaLocation;
    }
}


//!************************************************************************
//! Set the in use status
//!
//! @returns nothing
//!************************************************************************
void Mic::setInUse
    (
    const bool aStatus                  //!< in use status
    )
{
    mInUse = aStatus;
}


//!************************************************************************
//! Set the integer label that matches EVAL-MICCANVASZ numbering
//!
//! @returns nothing
//!************************************************************************
void Mic::setLabel
    (
    const int aLabel                    //!< label
    )
{
    mLabel = aLabel;
}


//!************************************************************************
//! Set the 3D location
//!
//! @returns nothing
//!************************************************************************
void Mic::setXyzLocation
    (
    const XyzLocation  aXyzLocation      //!< xyz location
    )
{
    memcpy( &mXyzLocation, &aXyzLocation, sizeof( mXyzLocation ) );
}
