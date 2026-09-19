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
VoiceActivityDetection.cpp

This file contains the sources for the VAD (voice activity detection).
*/

#include "VoiceActivityDetection.h"

#include <iostream>


//!************************************************************************
//! Constructor
//!************************************************************************
VoiceActivityDetection::VoiceActivityDetection
    (
    QObject* aParent    //!< parent object
    )
    : QObject( aParent )
    , mSource( SOURCE_UNKNOWN )
    , mIndex( 0 )
    , mIsVoice( false )
{
}


//!************************************************************************
//! Get the VAD object index
//!
//! @returns the index
//!************************************************************************
int VoiceActivityDetection::getIndex() const
{
    return mIndex;
}


//!************************************************************************
//! Get the voice presence status
//!
//! @returns true if voice is detected
//!************************************************************************
bool VoiceActivityDetection::getIsVoice() const
{
    return mIsVoice;
}


//!************************************************************************
//! Get the VAD object source type
//!
//! @returns the source type
//!************************************************************************
VoiceActivityDetection::Source VoiceActivityDetection::getSource() const
{
    return mSource;
}


//!************************************************************************
//! Set the VAD object index
//! Multiple VAD objects may have the same index and different source
//! types.  Proper identification should be done using both parameters.
//!
//! @returns nothing
//!************************************************************************
void VoiceActivityDetection::setIndex
    (
    int aIndex        //!< index
    )
{
    mIndex = aIndex;
}


//!************************************************************************
//! Set whether voice is detected or not
//!
//! @returns nothing
//!************************************************************************
void VoiceActivityDetection::setIsVoice
    (
    bool aIsVoice     //!< true if voice is detected
    )
{
    if( aIsVoice != mIsVoice )
    {
        mIsVoice = aIsVoice;
        emit haveVoiceDetectionChanged( mIsVoice, mIndex, mSource );
    }
}


//!************************************************************************
//! Set the VAD object source type
//! Multiple VAD objects may have the same source type and different
//! indexes.  Proper identification should be done using both parameters.
//!
//! @returns nothing
//!************************************************************************
void VoiceActivityDetection::setSource
    (
    Source aSource    //!< source
    )
{
    if( aSource < SOURCE_MAX_COUNT )
    {
        mSource = aSource;
    }
}
