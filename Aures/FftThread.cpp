///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023, 2026 Mihai Ursu                                           //
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
FftThread.cpp

This file contains the sources for the FFT thread.
*/

#include "FftThread.h"

#if BUILD_CUDA
    #include <algorithm>
    #include <cstring>
#endif


//!************************************************************************
//! Constructor
//!************************************************************************
FftThread::FftThread
    (
    QObject* aParent    //!< parent object
    )
    : QThread( aParent )
    , mIndex( 0 )
    , mFftSize( 0 )
    , mFftSizeChanged( false )
    , mIsRestarting( false )
    , mIsAborting( false )
#if BUILD_CUDA
    , mForceUseCpu( true )
    , mCuFftHandle( 0 )
    , mCuFftStream( nullptr )
    , mCuFftDirectDataInput( nullptr )
    , mCuFftDirectDataOutput( nullptr )
    , mCuFftInverseDataInput( nullptr )
    , mCuFftInverseDataOutput( nullptr )
#endif
{
#if BUILD_CUDA
    bool status = ( CUFFT_SUCCESS == cufftCreate( &mCuFftHandle ) );

    if( status )
    {
        status = ( cudaSuccess == cudaStreamCreateWithFlags( &mCuFftStream, cudaStreamNonBlocking ) );
    }

    if( status )
    {
        status = ( CUFFT_SUCCESS == cufftSetStream( mCuFftHandle, mCuFftStream ) );
    }
#endif
}


//!************************************************************************
//! Destructor
//!************************************************************************
FftThread::~FftThread()
{
    mMutex.lock();
        mIsAborting = true;
        mWaitCondition.wakeOne();
    mMutex.unlock();

    wait();

#if BUILD_CUDA
    if( mCuFftDirectDataOutput )
    {
        cudaFree( mCuFftDirectDataOutput );
    }
    if( mCuFftDirectDataInput )
    {
        cudaFree( mCuFftDirectDataInput );
    }

    if( mCuFftInverseDataOutput )
    {
        cudaFree( mCuFftInverseDataOutput );
    }
    if( mCuFftInverseDataInput )
    {
        cudaFree( mCuFftInverseDataInput );
    }

    cufftDestroy( mCuFftHandle );

    if( mCuFftStream )
    {
        cudaStreamDestroy( mCuFftStream );
    }
#endif
}


//!************************************************************************
//! Compute data loader
//! It provides input data and information for calculating the FFT.
//!
//! @returns nothing
//!************************************************************************
void FftThread::compute
    (
    CxVector                    aDataVec,   //!< input data for FFT computing
    FrequencyAnalysis::FftSense aSense,     //!< FFT compute sense - direct or inverse
    int                         aIndex,     //!< compute index
    bool                        aCpuOnly    //!< true if selecting only CPU/FPU and no GPU
    )
{
    QMutexLocker locker( &mMutex );
    mDataVec = aDataVec;
    mSense = aSense;
    mIndex = aIndex;
#if BUILD_CUDA
    mForceUseCpu = aCpuOnly;
#else
    Q_UNUSED( aCpuOnly );
#endif
    mFftSizeChanged = ( mFftSize != mDataVec.size() );
    mFftSize = mDataVec.size();

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
//! FFT thread main function
//!
//! @returns nothing
//!************************************************************************
/* virtual */ void FftThread::run()
{
    forever
    {
        mMutex.lock();
            double dataArray[1 + mFftSize * 2] = { 0 };

            for( size_t i = 0; i < mDataVec.size(); i++ )
            {
                dataArray[ 2 * i + 1 ] = mDataVec.at( i ).real();
                dataArray[ 2 * i + 2 ] = mDataVec.at( i ).imag();
            }

            const int DATA_LEN = 1 + mFftSize * 2;
            const int FFT_SIZE = mFftSize;
            const FrequencyAnalysis::FftSense SENSE = mSense;
            const int INDEX = mIndex;

            #if BUILD_CUDA
                // input for direct FFT, output for inverse FFT
                CxVector cuIoCxVec1( FFT_SIZE );
                // output for direct FFT, input for inverse FFT
                CxVector cuIoCxVec2( FFT_SIZE );

                if( !mForceUseCpu )
                {
                    if( SENSE == FrequencyAnalysis::FFT_SENSE_DIRECT )
                    {
                        cuIoCxVec1 = mDataVec;
                    }
                    else if( SENSE == FrequencyAnalysis::FFT_SENSE_INVERSE )
                    {                       
                        cuIoCxVec2 = mDataVec;
                    }
                }
            #endif
        mMutex.unlock();

        if( mIsAborting )
        {
            return;
        }

        FrequencyAnalysis* faInstance = FrequencyAnalysis::getInstance();

        #if !BUILD_CUDA
            faInstance->calculateFourierTransform( dataArray, FFT_SIZE, SENSE );
        #else
            bool status = true;

            if( mForceUseCpu )
            {               
                faInstance->calculateFourierTransform( dataArray, FFT_SIZE, SENSE );
            }
            else
            {
                if( SENSE == FrequencyAnalysis::FFT_SENSE_DIRECT )
                {
                    if( mFftSizeChanged )
                    {
                        mFftSizeChanged = false;

                        if( status && mCuFftDirectDataOutput )
                        {
                            status = ( cudaSuccess == cudaFree( mCuFftDirectDataOutput ) );
                        }
                        if( status && mCuFftDirectDataInput )
                        {
                            status = ( cudaSuccess == cudaFree( mCuFftDirectDataInput ) );
                        }

                        if( status )
                        {
                            status = ( CUFFT_SUCCESS == cufftPlan1d( &mCuFftHandle,
                                                                     FFT_SIZE,
                                                                     CUFFT_Z2Z,
                                                                     1 ) );
                        }

                        if( status )
                        {
                            status = ( cudaSuccess == cudaMalloc( static_cast<cufftDoubleComplex**>( &mCuFftDirectDataInput ),
                                                                  sizeof( cdouble ) * FFT_SIZE ) );
                        }
                        if( status )
                        {
                            status = ( cudaSuccess == cudaMalloc( static_cast<cufftDoubleComplex**>( &mCuFftDirectDataOutput ),
                                                                  sizeof( cdouble ) * FFT_SIZE ) );
                        }
                    }

                    if( status )
                    {
                        status = ( cudaSuccess == cudaMemcpyAsync( mCuFftDirectDataInput,
                                                                   cuIoCxVec1.data(),
                                                                   sizeof( cdouble ) * FFT_SIZE,
                                                                   cudaMemcpyHostToDevice,
                                                                   mCuFftStream ) );
                    }

                    if( status )
                    {
                        status = ( CUFFT_SUCCESS == cufftExecZ2Z( mCuFftHandle,
                                                                  mCuFftDirectDataInput,
                                                                  mCuFftDirectDataOutput,
                                                                  CUFFT_FORWARD ) );
                    }

                    if( status )
                    {
                        status = ( cudaSuccess == cudaMemcpyAsync( cuIoCxVec2.data(),
                                                                   mCuFftDirectDataOutput,
                                                                   sizeof( cdouble ) * FFT_SIZE,
                                                                   cudaMemcpyDeviceToHost,
                                                                   mCuFftStream ) );
                    }

                    if( status )
                    {
                        status = ( cudaSuccess == cudaStreamSynchronize( mCuFftStream ) );
                    }

                    if( status )
                    {
                        for( int i = 1; i <= DATA_LEN - 1; i++ )
                        {
                            if( i % 2 )
                            {
                                dataArray[i] = cuIoCxVec2.at( ( i - 1 ) / 2 ).real();
                            }
                            else
                            {
                                dataArray[i] = -cuIoCxVec2.at( ( i - 2 ) / 2 ).imag();
                            }
                        }
                    }
                }
                else if( SENSE == FrequencyAnalysis::FFT_SENSE_INVERSE )
                {
                    if( mFftSizeChanged )
                    {
                        mFftSizeChanged = false;

                        if( status && mCuFftInverseDataOutput )
                        {
                            status = ( cudaSuccess == cudaFree( mCuFftInverseDataOutput ) );
                        }
                        if( status && mCuFftInverseDataInput )
                        {
                            status = ( cudaSuccess == cudaFree( mCuFftInverseDataInput ) );
                        }

                        if( status )
                        {
                            status = ( CUFFT_SUCCESS == cufftPlan1d( &mCuFftHandle,
                                                                     FFT_SIZE,
                                                                     CUFFT_Z2Z,
                                                                     1 ) );
                        }

                        if( status )
                        {
                            status = ( cudaSuccess == cudaMalloc( static_cast<cufftDoubleComplex**>( &mCuFftInverseDataInput ),
                                                                  sizeof( cdouble ) * FFT_SIZE ) );
                        }
                        if( status )
                        {
                            status = ( cudaSuccess == cudaMalloc( static_cast<cufftDoubleComplex**>( &mCuFftInverseDataOutput ),
                                                                  sizeof( cdouble ) * FFT_SIZE ) );
                        }
                    }

                    if( status )
                    {
                        status = ( cudaSuccess == cudaMemcpyAsync( mCuFftInverseDataInput,
                                                                   cuIoCxVec2.data(),
                                                                   sizeof( cdouble ) * FFT_SIZE,
                                                                   cudaMemcpyHostToDevice,
                                                                   mCuFftStream ) );
                    }

                    if( status )
                    {
                        status = ( CUFFT_SUCCESS == cufftExecZ2Z( mCuFftHandle,
                                                                  mCuFftInverseDataInput,
                                                                  mCuFftInverseDataOutput,
                                                                  CUFFT_INVERSE) );
                    }

                    if( status )
                    {
                        status = ( cudaSuccess == cudaMemcpyAsync( cuIoCxVec1.data(),
                                                                   mCuFftInverseDataOutput,
                                                                   sizeof( cdouble ) * FFT_SIZE,
                                                                   cudaMemcpyDeviceToHost,
                                                                   mCuFftStream ) );
                    }

                    if( status )
                    {
                        status = ( cudaSuccess == cudaStreamSynchronize( mCuFftStream ) );
                    }

                    if( status )
                    {
                        for( int i = 1; i <= DATA_LEN - 1; i++ )
                        {
                            if( i % 2 )
                            {
                                dataArray[i] = -cuIoCxVec1.at( ( i - 1 ) / 2 ).real();
                            }
                            else
                            {
                                dataArray[i] = cuIoCxVec1.at( ( i - 2 ) / 2 ).imag();
                            }
                        }
                    }
                }
            }
        #endif

        if( !mIsRestarting )
        {
            #if !BUILD_CUDA
                emit fftComputeDone( dataArray, DATA_LEN, INDEX );
            #else
                if( mForceUseCpu )
                {
                    emit fftComputeDone( dataArray, DATA_LEN, INDEX );
                }
                else
                {
                    if( status )
                    {
                        emit fftComputeDone( dataArray, DATA_LEN, INDEX );
                    }
                }
            #endif
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
