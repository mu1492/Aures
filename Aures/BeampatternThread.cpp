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
BeampatternThread.cpp

This file contains the sources for the beampattern compute thread.
*/

#include "BeampatternThread.h"

#include "RawSignalHandler.h"

#include <cstring>


//!************************************************************************
//! Constructor
//!************************************************************************
BeampatternThread::BeampatternThread
    (
    QObject* aParent    //!< parent object
    )
    : QThread( aParent )
    , mIsRestarting( false )
    , mIsAborting( false )
{
    memset( &mBeampatternConfig, 0, sizeof( mBeampatternConfig ) );
}


//!************************************************************************
//! Destructor
//!************************************************************************
BeampatternThread::~BeampatternThread()
{
    mMutex.lock();
        mIsAborting = true;
        mWaitCondition.wakeOne();
    mMutex.unlock();

    wait();
}


//!************************************************************************
//! Compute data loader
//! It provides input data for computing the beampattern.
//!
//! @returns nothing
//!************************************************************************
void BeampatternThread::compute
    (
    Beamforming::BeampatternConfig  aBeampatternConfig, //!< configuration data
    MicArray                        aMicArray           //!< microphone array
    )
{
    QMutexLocker locker( &mMutex );
    mBeampatternConfig = aBeampatternConfig;
    mMicArray = aMicArray;

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
//! Beampattern compute thread main function
//!
//! @returns nothing
//!************************************************************************
/* virtual */ void BeampatternThread::run()
{
    forever
    {
        mMutex.lock();
            const Beamforming::BeampatternConfig BEAMPATTERN_CONFIG = mBeampatternConfig;
            const MicArray MIC_ARRAY = mMicArray;
            const std::vector<size_t> MICS_VEC = MIC_ARRAY.getMicIndexes();
            Cx3Matrix rnnMatrix;
            RawSignalHandler* rawSignalHandler = RawSignalHandler::getInstance();

            if( rawSignalHandler )
            {
                rnnMatrix = rawSignalHandler->getRnnMatrix( MICS_VEC );
            }
        mMutex.unlock();

        if( mIsAborting )
        {
            return;
        }

        CxVector beampatternCxValues;
        Numeric* numericInstance = Numeric::getInstance();
        Beamforming bf;
        const double DOA_TO_MIC5_RAD = numericInstance->deg2Rad( BEAMPATTERN_CONFIG.doaDeg );
        const double DOA_TO_MIC3_RAD = DOA_TO_MIC5_RAD - 0.5 * Numeric::PI;

        switch( BEAMPATTERN_CONFIG.type )
        {
            case Beamforming::BEAMPATTERN_TYPE_DAS:
                {
                    for( int thetaToMic5Deg = -180; thetaToMic5Deg < 180; thetaToMic5Deg++ )
                    {
                        double thetaToMic5Rad = numericInstance->deg2Rad( thetaToMic5Deg );
                        double thetaToMic3Rad = thetaToMic5Rad - 0.5 * Numeric::PI;

                        cdouble bpv = bf.calculateDasBeampattern( BEAMPATTERN_CONFIG.frequency,
                                                                  DOA_TO_MIC3_RAD,
                                                                  thetaToMic3Rad,
                                                                  MIC_ARRAY,
                                                                  BEAMPATTERN_CONFIG.radialApodization );
                        beampatternCxValues.push_back( bpv );
                    }
                }
                break;

            case Beamforming::BEAMPATTERN_TYPE_MVDR:
                if( rawSignalHandler )
                {
                    for( int thetaToMic5Deg = -180; thetaToMic5Deg < 180; thetaToMic5Deg++ )
                    {
                        double thetaToMic5Rad = numericInstance->deg2Rad( thetaToMic5Deg );
                        double thetaToMic3Rad = thetaToMic5Rad - 0.5 * Numeric::PI;

                        cdouble bpv = bf.calculateMvdrBeampattern( BEAMPATTERN_CONFIG.frequency,
                                                                   DOA_TO_MIC3_RAD,
                                                                   thetaToMic3Rad,
                                                                   MIC_ARRAY,
                                                                   rnnMatrix );
                        beampatternCxValues.push_back( bpv );
                    }
                }
                break;

            case Beamforming::BEAMPATTERN_TYPE_ADAPTIVE:
                if( rawSignalHandler )
                {
                    for( int thetaToMic5Deg = -180; thetaToMic5Deg < 180; thetaToMic5Deg++ )
                    {
                        double thetaToMic5Rad = numericInstance->deg2Rad( thetaToMic5Deg );
                        double thetaToMic3Rad = thetaToMic5Rad - 0.5 * Numeric::PI;

                        cdouble bpv = bf.calculateAdaptiveBeampattern( BEAMPATTERN_CONFIG.frequency,
                                                                       DOA_TO_MIC3_RAD,
                                                                       thetaToMic3Rad,
                                                                       MIC_ARRAY,
                                                                       rnnMatrix,
                                                                       BEAMPATTERN_CONFIG.frequencyCenter );
                        beampatternCxValues.push_back( bpv );
                    }
                }
                break;

            default:
                break;
        }

        if( !mIsRestarting )
        {
            emit beampatternComputeDone( beampatternCxValues );
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
