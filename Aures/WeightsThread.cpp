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
WeightsThread.cpp

This file contains the sources for the weights compute thread.
*/

#include "WeightsThread.h"

#include "RawSignalHandler.h"

#include <cstring>
#include <iostream>


//!************************************************************************
//! Constructor
//!************************************************************************
WeightsThread::WeightsThread
    (
    QObject* aParent    //!< parent object
    )
    : QThread( aParent )
    , mIndex( -1 )
    , mIsRestarting( false )
    , mIsAborting( false )
{
    memset( &mBeampatternConfig, 0, sizeof( mBeampatternConfig ) );
}


//!************************************************************************
//! Destructor
//!************************************************************************
WeightsThread::~WeightsThread()
{
    mMutex.lock();
        mIsAborting = true;
        mWaitCondition.wakeOne();
    mMutex.unlock();

    wait();
}


//!************************************************************************
//! Compute data loader
//! It provides input data for computing the weights.
//!
//! @returns nothing
//!************************************************************************
void WeightsThread::compute
    (
    Beamforming::BeampatternConfig  aBeampatternConfig, //!< configuration data
    MicArray                        aMicArray,          //!< microphone array
    int                             aIndex              //!< index
    )
{
    QMutexLocker locker( &mMutex );
    mBeampatternConfig = aBeampatternConfig;
    mMicArray = aMicArray;
    mIndex = aIndex;

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
/* virtual */ void WeightsThread::run()
{
    forever
    {
        mMutex.lock();
            const Beamforming::BeampatternConfig BEAMPATTERN_CONFIG = mBeampatternConfig;
            const MicArray MIC_ARRAY = mMicArray;
            const int INDEX = mIndex;

            const std::vector<size_t> MICS_VEC = MIC_ARRAY.getMicIndexes();
            const size_t NR_OF_SPEAKERS = ( MICS_VEC.size() - 1 ) / 2;

            RawSignalHandler* rawSignalHandler = RawSignalHandler::getInstance();
            const Cx3Matrix RNN_MATRIX = rawSignalHandler->getRnnMatrix( MICS_VEC );

            FrequencyAnalysis* faInstance = FrequencyAnalysis::getInstance();
            const double BIN_WIDTH = faInstance->getFftBinWidth();
            const size_t FFT_SIZE = faInstance->getFftSizeValue();
        mMutex.unlock();

        if( mIsAborting )
        {
            return;
        }

        CxMatrix weightsMatrix;
        weightsMatrix.resize( MICS_VEC.size() );

        for( size_t crtActiveMic = 0; crtActiveMic < MICS_VEC.size(); crtActiveMic++ )
        {
            weightsMatrix.at( crtActiveMic ).resize( FFT_SIZE );
        }

        Numeric* numericInstance = Numeric::getInstance();
        Beamforming bf;
        CxVector weightsCxValues;
        const double DOA_TO_MIC5_RAD = numericInstance->deg2Rad( BEAMPATTERN_CONFIG.doaDeg );
        const double DOA_TO_MIC3_RAD = DOA_TO_MIC5_RAD - 0.5 * Numeric::PI;

        switch( BEAMPATTERN_CONFIG.type )
        {
            case Beamforming::BEAMPATTERN_TYPE_DAS:
                if( !BEAMPATTERN_CONFIG.radialApodization )
                {
                    for( size_t crtBin = 0; crtBin <= FFT_SIZE / 2; crtBin++ )
                    {
                        weightsCxValues = bf.calculateDasWeights( BIN_WIDTH * crtBin,
                                                                  DOA_TO_MIC3_RAD,
                                                                  MIC_ARRAY );

                        for( size_t crtActiveMic = 0; crtActiveMic < weightsCxValues.size(); crtActiveMic++ )
                        {
                            weightsMatrix.at( crtActiveMic ).at( crtBin ) = weightsCxValues.at( crtActiveMic );

                            if( ( 0 != crtBin ) && ( FFT_SIZE / 2 != crtBin ) )
                            {
                                weightsMatrix.at( crtActiveMic ).at( FFT_SIZE - crtBin ) = std::conj( weightsCxValues.at( crtActiveMic ) );
                            }
                        }
                    }
                }
                else
                {
                    for( size_t crtBin = 0; crtBin <= FFT_SIZE / 2; crtBin++ )
                    {
                        weightsCxValues = bf.calculateDasWeightsRadialApodization( BIN_WIDTH * crtBin,
                                                                                   DOA_TO_MIC3_RAD,
                                                                                   MIC_ARRAY );

                        for( auto& z : weightsCxValues )
                        {
                            z /= RAD_APOD_FACTOR_VEC.at( NR_OF_SPEAKERS - 1 );
                        }

                        for( size_t crtActiveMic = 0; crtActiveMic < weightsCxValues.size(); crtActiveMic++ )
                        {
                            weightsMatrix.at( crtActiveMic ).at( crtBin ) = weightsCxValues.at( crtActiveMic );

                            if( ( 0 != crtBin ) && ( FFT_SIZE / 2 != crtBin ) )
                            {
                                weightsMatrix.at( crtActiveMic ).at( FFT_SIZE - crtBin ) = std::conj( weightsCxValues.at( crtActiveMic ) );
                            }
                        }
                    }
                }
                break;

            case Beamforming::BEAMPATTERN_TYPE_MVDR:
                {
                    for( size_t crtBin = 0; crtBin <= FFT_SIZE / 2; crtBin++ )
                    {
                        weightsCxValues = bf.calculateMvdrWeights( BIN_WIDTH * crtBin,
                                                                   DOA_TO_MIC3_RAD,
                                                                   MIC_ARRAY,
                                                                   RNN_MATRIX );

                        for( size_t crtActiveMic = 0; crtActiveMic < weightsCxValues.size(); crtActiveMic++ )
                        {
                            weightsMatrix.at( crtActiveMic ).at( crtBin ) = weightsCxValues.at( crtActiveMic );

                            if( ( 0 != crtBin ) && ( FFT_SIZE / 2 != crtBin ) )
                            {
                                weightsMatrix.at( crtActiveMic ).at( FFT_SIZE - crtBin ) = std::conj( weightsCxValues.at( crtActiveMic ) );
                            }
                        }
                    }

                    double maxValue = 0;

                    for( size_t i = 0; i < weightsMatrix.size(); i++ )
                    {
                        for( size_t j = 0; j < weightsMatrix.at( 0 ).size(); j++ )
                        {
                            maxValue = std::max( maxValue, std::abs( weightsMatrix.at( i ).at( j ) ) );
                        }
                    }

                    if( maxValue )
                    {
                        for( size_t i = 0; i < weightsMatrix.size(); i++ )
                        {
                            for( size_t j = 0; j < weightsMatrix.at( 0 ).size(); j++ )
                            {
                                weightsMatrix.at( i ).at( j ) /= maxValue * ( 2.0 * NR_OF_SPEAKERS + 1.0 );
                            }
                        }
                    }
                }
                break;

            case Beamforming::BEAMPATTERN_TYPE_ADAPTIVE:
                {
                    for( size_t crtBin = 0; crtBin <= FFT_SIZE / 2; crtBin++ )
                    {
                        weightsCxValues = bf.calculateMixedWeights( BIN_WIDTH * crtBin,
                                                                    DOA_TO_MIC3_RAD,
                                                                    MIC_ARRAY,
                                                                    RNN_MATRIX,
                                                                    BEAMPATTERN_CONFIG.frequencyCenter );

                        for( size_t crtActiveMic = 0; crtActiveMic < weightsCxValues.size(); crtActiveMic++ )
                        {
                            weightsMatrix.at( crtActiveMic ).at( crtBin ) = weightsCxValues.at( crtActiveMic );

                            if( ( 0 != crtBin ) && ( FFT_SIZE / 2 != crtBin ) )
                            {
                                weightsMatrix.at( crtActiveMic ).at( FFT_SIZE - crtBin ) = std::conj( weightsCxValues.at( crtActiveMic ) );
                            }
                        }
                    }

                    double maxValue = 0;

                    for( size_t i = 0; i < weightsMatrix.size(); i++ )
                    {
                        for( size_t j = 0; j < weightsMatrix.at( 0 ).size(); j++ )
                        {
                            maxValue = std::max( maxValue, std::abs( weightsMatrix.at( i ).at( j ) ) );
                        }
                    }

                    if( maxValue )
                    {
                        for( size_t i = 0; i < weightsMatrix.size(); i++ )
                        {
                            for( size_t j = 0; j < weightsMatrix.at( 0 ).size(); j++ )
                            {
                                weightsMatrix.at( i ).at( j ) /= maxValue * ( 2.0 * NR_OF_SPEAKERS + 1.0 );
                            }
                        }
                    }
                }
                break;

            default:
                break;
        }

        if( !mIsRestarting )
        {
            emit weightsComputeDone( weightsMatrix, INDEX );
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
