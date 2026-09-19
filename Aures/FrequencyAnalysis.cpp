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
FrequencyAnalysis.cpp

This file contains the sources for the frequency analysis.
*/

#include "FrequencyAnalysis.h"

#include "AudioCaptureThread.h"
#include "Numeric.h"
#include "RawSignalHandler.h"


FrequencyAnalysis* FrequencyAnalysis::sInstance = nullptr;

const std::map<FrequencyAnalysis::FftSizeOption, uint32_t> FrequencyAnalysis::FFT_SIZE_VALUES =
{
    { FrequencyAnalysis::FFT_SIZE_256,         256 },
    { FrequencyAnalysis::FFT_SIZE_512,         512 },
    { FrequencyAnalysis::FFT_SIZE_1024,       1024 },
    { FrequencyAnalysis::FFT_SIZE_2048,       2048 }
};


//!************************************************************************
//! Constructor
//!************************************************************************
FrequencyAnalysis::FrequencyAnalysis()
    : mSps( AudioCaptureThread::SAMPLE_RATE )
    , mFftFreqMax( mSps / 2 )
    , mFftSizeIndex( FFT_SIZE_512 )
    , mFftSizeValue( FFT_SIZE_VALUES.at( mFftSizeIndex ) )
    , mFftBinWidth( mSps / mFftSizeValue )
    , mFftTimeGate( 1 / mFftBinWidth )
{    
    mNumericInstance = Numeric::getInstance();

    for( FftSizeOption idxOption = FFT_SIZE_256; idxOption < FFT_SIZE_MAX; idxOption = static_cast<FftSizeOption>( idxOption + 1 ) )
    {
        if( RawSignalHandler::STFT_WINDOW_TOTAL_FRAMES == FFT_SIZE_VALUES.at( idxOption ) )
        {
            mFftSizeIndex = idxOption;
            mFftSizeValue = FFT_SIZE_VALUES.at( mFftSizeIndex );
            break;
        }
    }

    mWindowFunction.setFftSize( mFftSizeValue );
    setFftSizeOptionIndex( mFftSizeIndex );
}


//!************************************************************************
//! Destructor
//!************************************************************************
FrequencyAnalysis::~FrequencyAnalysis()
{
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
FrequencyAnalysis* FrequencyAnalysis::getInstance()
{
    if( !sInstance )
    {
        sInstance = new FrequencyAnalysis;
    }

    return sInstance;
}


//!************************************************************************
//! Calculate the 1D Fourier transform for real values
//! The data table can be used for either real or complex values.
//! For complex values, the real and imaginary parts must alternate:
//! Re[0], Im[0], Re[1], Im[1], ..., Re[N-1], Im[N-1].
//!
//! @returns nothing
//!************************************************************************
void FrequencyAnalysis::calculateFourierTransform
    (
    double          aData[],		//!< data, dimension is right shifted FFT size
    const uint32_t	aShrFftSize,	//!< right shifted FFT size, must be a power of two
    const FftSense	aTransformSense	//!< direct or inverse transform
    )
{
    if( aData )
    {
        if( mNumericInstance->isPowerOfTwo( aShrFftSize ) )
        {
            const uint32_t FFT_SIZE = aShrFftSize << 1;
            uint32_t i = 0;
            uint32_t j = 1;
            uint32_t m = 0;

            for( i = 1; i < FFT_SIZE; i += 2 )
            {
                if( j > i )
                {
                    mNumericInstance->swap( aData[j], aData[i] );
                    mNumericInstance->swap( aData[j + 1], aData[i + 1] );
                }

                m = FFT_SIZE >> 1;

                while( m >= 2 && j > m )
                {
                    j -= m;
                    m >>= 1;
                }

                j += m;
            }

            uint32_t mmax = 2;
            uint32_t istep = 0;

            double theta = 0;
            double tempr = 0;
            double tempi = 0;

            while( FFT_SIZE > mmax )
            {
                istep = mmax << 1;
                theta = aTransformSense * ( Numeric::TWO_PI / mmax );
                double wtemp = sin( 0.5 * theta );
                double wpr = -2.0 * wtemp * wtemp;
                double wpi = sin( theta );
                double wr = 1.0;
                double wi = 0.0;

                for( m = 1; m < mmax; m += 2 )
                {
                    for( i = m; i <= FFT_SIZE; i += istep )
                    {
                        j = i + mmax;

                        tempr = wr * aData[j] - wi * aData[j + 1];
                        tempi = wr * aData[j + 1] + wi * aData[j];

                        aData[j] = aData[i] - tempr;
                        aData[j + 1] = aData[i + 1] - tempi;

                        aData[i] += tempr;
                        aData[i + 1] += tempi;
                    }

                    wtemp = wr;
                    wr = wtemp * wpr - wi * wpi + wr;
                    wi = wi * wpr + wtemp * wpi + wi;
                }

                mmax = istep;
            }
        }
    }
}


//!************************************************************************
//! Get the FFT bin width (frequency resolution) [Hz/bin]
//!
//! @returns The FFT bin width
//!************************************************************************
double FrequencyAnalysis::getFftBinWidth() const
{
    return mFftBinWidth;
}


//!************************************************************************
//! Get the FFT maximum frequency [Hz]
//!
//! @returns The FFT maximum frequency
//!************************************************************************
double FrequencyAnalysis::getFftFreqMax() const
{
    return mFftFreqMax;
}


//!************************************************************************
//! Get the index of the FFT size option
//!
//! @returns The FFT size index
//!************************************************************************
FrequencyAnalysis::FftSizeOption FrequencyAnalysis::getFftSizeOptionIndex() const
{
    return mFftSizeIndex;
}


//!************************************************************************
//! Get the value of the FFT size
//!
//! @returns The FFT size
//!************************************************************************
uint32_t FrequencyAnalysis::getFftSizeValue() const
{
    return mFftSizeValue;
}


//!************************************************************************
//! Get the FFT sampling rate [Hz]
//!
//! @returns The FFT SPS
//!************************************************************************
double FrequencyAnalysis::getFftSps() const
{
    return mSps;
}


//!************************************************************************
//! Get the FFT time gate [s]
//! The time gate [s] is equal to the inverse of the bin width [Hz/bin].
//!
//! @returns The FFT time gate
//!************************************************************************
double FrequencyAnalysis::getFftTimeGate() const
{
    return mFftTimeGate;
}


//!************************************************************************
//! Get the address of FFT window function
//!
//! @returns The FFT window function address
//!************************************************************************
WindowFunction& FrequencyAnalysis::getWindowFunction()
{
    return mWindowFunction;
}


//!************************************************************************
//! Set a new index in the FFT size options
//!
//! @returns true if the value could be set
//!************************************************************************
bool FrequencyAnalysis::setFftSizeOptionIndex
    (
    const FftSizeOption aFftSizeIndex          //!< FFT size index
    )
{
    bool status = aFftSizeIndex < FFT_SIZE_MAX;

    if( status && ( aFftSizeIndex != mFftSizeIndex ) )
    {
        mFftSizeIndex = aFftSizeIndex;
        mFftSizeValue = FFT_SIZE_VALUES.at( mFftSizeIndex );

        mFftBinWidth = mSps / mFftSizeValue;
        mFftTimeGate = 1.0 /  mFftBinWidth;

        getWindowFunction().setFftSize( mFftSizeValue );
    }

    return status;
}


//!************************************************************************
//! Set a new FFT sampling rate
//!
//! @returns true if the value could be set
//!************************************************************************
bool FrequencyAnalysis::setFftSps
    (
    const double aSps   //!< SPS [Hz]
    )
{
    bool status = aSps > 0;

    if( status && ( aSps != mSps ) )
    {
        mSps = aSps;

        mFftFreqMax = mSps / 2.0;
        mFftBinWidth = mSps / mFftSizeValue;
        mFftTimeGate = 1.0 /  mFftBinWidth;
    }

    return status;
}
