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
RnnComputeThread.cpp

This file contains the sources for the Rnn matrix compute thread.
*/

#include "RnnComputeThread.h"

#include "MicArray.h"
#include "RawSignalHandler.h"


//!************************************************************************
//! Constructor
//!************************************************************************
RnnComputeThread::RnnComputeThread
    (
    QObject* aParent    //!< parent object
    )
    : QThread( aParent )
    , mFftSize( 0 )
    , mIsComputing( false )
    , mIsAborting( false )
{
    start();
}


//!************************************************************************
//! Destructor
//!************************************************************************
RnnComputeThread::~RnnComputeThread()
{
    mMutex.lock();
        mIsAborting = true;
        mWaitCondition.wakeOne();
    mMutex.unlock();

    wait();
}


//!************************************************************************
//! Compute data loader
//! It provides input data and information for calculating the Rnn matrix.
//!
//! @returns nothing
//!************************************************************************
void RnnComputeThread::compute
    (
    std::vector<std::vector<std::deque<cdouble>>>   aNoiseDataMatrix,   //!< noise data matrix
    uint32_t                                        aFftSize            //!< FFT size
    )
{
    QMutexLocker locker( &mMutex );

    if( !mIsComputing )
    {
        mNoiseDataMatrix = std::move( aNoiseDataMatrix );
        mFftSize = aFftSize;

        mWaitCondition.wakeOne();
    }
}


//!************************************************************************
//! Rnn compute thread main function
//!
//! @returns nothing
//!************************************************************************
/* virtual */ void RnnComputeThread::run()
{
    forever
    {        
        std::vector<std::vector<std::deque<cdouble>>> noiseData;
        uint32_t fftSize = 0;

        {
            QMutexLocker locker( &mMutex );

            while( mNoiseDataMatrix.empty() && !mIsAborting )
            {
                mWaitCondition.wait( &mMutex );
            }

            if( mIsAborting )
            {
                return;
            }

            noiseData = std::move( mNoiseDataMatrix );
            fftSize = mFftSize;

            mIsComputing = true;
        }

        Cx3Matrix rnnMatrix;
        rnnMatrix.resize( MicArray::TOTAL_MICS_COUNT );

        for( size_t micI = 0; micI < MicArray::TOTAL_MICS_COUNT; micI++ )
        {
            rnnMatrix.at( micI ).resize( MicArray::TOTAL_MICS_COUNT );

            for( size_t micJ = 0; micJ < MicArray::TOTAL_MICS_COUNT; micJ++ )
            {
                rnnMatrix.at( micI ).at( micJ ).resize( fftSize );
            }
        }

        for( size_t micI = 0; micI < MicArray::TOTAL_MICS_COUNT; micI++ )
        {
            for( size_t micJ = 0; micJ < MicArray::TOTAL_MICS_COUNT; micJ++ )
            {
                for( size_t crtBin = 0; crtBin < fftSize; crtBin++ )
                {
                    cdouble rIJ;

                    for( size_t crtNoiseFrame = 0; crtNoiseFrame < RawSignalHandler::NOISE_ONLY_FRAMES; crtNoiseFrame++ )
                    {
                        cdouble xI = noiseData.at( micI ).at( crtBin ).at( crtNoiseFrame );
                        cdouble xJ = noiseData.at( micJ ).at( crtBin ).at( crtNoiseFrame );
                        rIJ += xI * std::conj( xJ );
                    }

                    rIJ /= static_cast<double>( RawSignalHandler::NOISE_ONLY_FRAMES );
                    rnnMatrix.at( micI ).at( micJ ).at( crtBin ) = rIJ;
                }
            }
        }

        {
            QMutexLocker locker( &mMutex );

            mIsComputing = false;

            if( mIsAborting )
            {
                return;
            }
        }

        emit rnnComputeDone( rnnMatrix );
    }
}
