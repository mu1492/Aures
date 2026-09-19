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
MicArrayConfigThread.cpp

This file contains the sources for the microphone array configuration thread.
*/

#include "MicArrayConfigThread.h"

#include "MultiSourceHandler.h"

#include <vector>


//!************************************************************************
//! Constructor
//!************************************************************************
MicArrayConfigThread::MicArrayConfigThread
    (
    QObject* aParent    //!< parent object
    )
    : QThread( aParent )
    , mNrOfSpeakers( 1 )
    , mFirstSpeakerAngle( 0 )
    , mFrequencyRange( AcousticsHandler::FREQUENCY_RANGE_VALUES.at( AcousticsHandler::FREQUENCY_RANGE_SPEECH_BAND_NOMINAL ) )
    , mIsRestarting( false )
    , mIsAborting( false )
{
}


//!************************************************************************
//! Destructor
//!************************************************************************
MicArrayConfigThread::~MicArrayConfigThread()
{
    mMutex.lock();
        mIsAborting = true;
        mWaitCondition.wakeOne();
    mMutex.unlock();

    wait();
}


//!************************************************************************
//! Compute data loader
//! It provides input data for configuring the microphone array.
//!
//! @returns nothing
//!************************************************************************
void MicArrayConfigThread::compute
    (
    uint8_t                             aNrOfSpeakers,  //!< number of speakers
    double                              aAngle,         //!< angle of first speeker
    AcousticsHandler::FrequencyRange    aFrequencyRange //!< frequency range
    )
{
    QMutexLocker locker( &mMutex );
    mNrOfSpeakers = aNrOfSpeakers;
    mFirstSpeakerAngle = aAngle;
    mFrequencyRange = aFrequencyRange;

    if( !isRunning() )
    {
        start();
    }
    else
    {
        mIsRestarting = true;
        mWaitCondition.wakeOne();
    }
}


//!************************************************************************
//! Microphone array config thread main function
//!
//! @returns nothing
//!************************************************************************
/* virtual */ void MicArrayConfigThread::run()
{
    forever
    {
        mMutex.lock();
            const uint8_t NR_OF_SPEAKERS = mNrOfSpeakers;
            const double FIRST_SPEAKER_ANGLE = mFirstSpeakerAngle;
            const AcousticsHandler::FrequencyRange FREQUENCY_RANGE = mFrequencyRange;
        mMutex.unlock();

        if( mIsAborting )
        {
            return;
        }

        MicArray fullMicArray;
        fullMicArray.createFullGeometry();
        MultiSourceHandler multiSrcHndl;
        std::vector<int> bestConfigMicVec = multiSrcHndl.getBestConfigMicrophones( FREQUENCY_RANGE,
                                                                                   NR_OF_SPEAKERS,
                                                                                   FIRST_SPEAKER_ANGLE,
                                                                                   fullMicArray );
        if( !bestConfigMicVec.empty() )
        {
            MicArray optimumMicArray;

            for( size_t i = 0; i < bestConfigMicVec.size(); i++ )
            {
                optimumMicArray.addSingleMic( bestConfigMicVec.at( i ) );
            }

            if( !mIsRestarting )
            {
                emit micArrayConfigComputeDone( optimumMicArray );
            }
        }

        mMutex.lock();
            if( !mIsRestarting )
            {
                mWaitCondition.wait( &mMutex );
            }

            mIsRestarting = false;
        mMutex.unlock();
    }
}
