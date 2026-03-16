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
    : mThd( 0 )
    , mSensitivity( -100 )
    , mSnr( 0 )
    , mAop( 0 )
{
    memset( &mLocation, 0, sizeof( mLocation ) );

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

    mEin = REF_SPL - mSnr;
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
//! Get the 3D location
//!
//! @returns The 3D mic location
//!************************************************************************
Mic::Location Mic::getLocation() const
{
    return mLocation;
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
//! Set the 3D location
//!
//! @returns nothing
//!************************************************************************
void Mic::setLocation
    (
    const Location  aLocation      //!< xyz location
    )
{
    memcpy( &mLocation, &aLocation, sizeof( mLocation ) );
}
