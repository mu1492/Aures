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
RawSignalHandler.cpp

This file contains the sources for processing raw audio signals.
*/

#include "RawSignalHandler.h"

#include "AcousticsHandler.h"
#include "Aures.h"
#include "FrequencyAnalysis.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

#include <QElapsedTimer>
#include <QMutexLocker>
#include <QTimer>


RawSignalHandler* RawSignalHandler::sInstance = nullptr;

//!************************************************************************
//! Constructor
//!************************************************************************
RawSignalHandler::RawSignalHandler()
    : mRmsWindowShown( false )
    , mWindowType( WindowFunction::WINDOW_FUNCTION_TYPE_HANN )
    , mIsRedimensioning( false )
    , mFftSize( 0 )
    , mOverlapAddGain( 0.5 * ( OLA_GAIN.min + OLA_GAIN.max ) )
    , mNoiseThdDb( 3.0 )
    , mDenoisingGainType( DENOISING_GAIN_WIENER_DECISION_DIRECTED )
    , mSpeechThdDb( 6.0 )
    , mVadEnabled( true )
    , mRnnComputeThread( nullptr )
    , mNumberOfSpeakers( 1 )
{
    // Use the constant for the entire number of microphones (TOTAL_MICS_COUNT=15) because the worker
    // threads deal with realtime audio data coming from TDM16 -> I2S -> ALSA -> AudioCaptureThread.
    // This is hardware setup dependent, and not related to finding a specific configuration of active
    // microphones.

    mRawSignalWorkersVec.resize( MicArray::TOTAL_MICS_COUNT );
    mRawSignalThreadsVec.resize( MicArray::TOTAL_MICS_COUNT );

    for( size_t i = 0; i < MicArray::TOTAL_MICS_COUNT; i++ )
    {
        RawSignalWorker* worker = new RawSignalWorker( i );

        if( worker )
        {
            mRawSignalWorkersVec.at( i ) = worker;
        }

        QThread* thread = new QThread();

        if( thread )
        {
            mRawSignalThreadsVec.at( i ) = thread;
        }
    }

    for( size_t i = 0; i < MicArray::TOTAL_MICS_COUNT; i++ )
    {
        if( mRawSignalWorkersVec.at( i ) && mRawSignalThreadsVec.at( i ) )
        {
            mRawSignalWorkersVec.at( i )->moveToThread( mRawSignalThreadsVec.at( i ) );

            connect( this, &RawSignalHandler::haveNewRawAudio, mRawSignalWorkersVec.at( i ), &RawSignalWorker::processNewAudio, Qt::QueuedConnection );
            connect( mRawSignalWorkersVec.at( i ), &RawSignalWorker::computeRmsDone, this, &RawSignalHandler::handleRms, Qt::QueuedConnection );
            connect( mRawSignalThreadsVec.at( i ), &QThread::finished, mRawSignalWorkersVec.at( i ), &QObject::deleteLater );

            mRawSignalThreadsVec.at( i )->start();
        }
    }

    mSumRmsBaseTimerVec.resize( MicArray::TOTAL_MICS_COUNT, 0 );
    mCountRmsBaseTimerVec.resize( MicArray::TOTAL_MICS_COUNT, 0 );

    mRmsBaseTimerVec.resize( MicArray::TOTAL_MICS_COUNT, 0 );
    mRmsMultiplierTimerVec.resize( MicArray::TOTAL_MICS_COUNT, 0 );

    connect( &mBaseTimer, &QTimer::timeout, this, &RawSignalHandler::handleBaseTimer );
    mBaseTimer.setTimerType( Qt::PreciseTimer );
    mBaseTimer.start( BASE_TIMER_MS );

    ///////////////////////////////
    // STFT
    ///////////////////////////////
    FrequencyAnalysis* faInstance = FrequencyAnalysis::getInstance();

    if( faInstance )
    {
        mFftSize = faInstance->getFftSizeValue();

        mWindowFunctionVec.resize( mFftSize, 0 );

        mAudioDataMatrix.resize( MicArray::TOTAL_MICS_COUNT, std::vector<double>( mFftSize, 0 ) );

        mAudioReconstructedMatrix.resize( MultiSourceHandler::MAX_NR_OF_SPEAKERS, std::vector<double>( mFftSize, 0 ) );

        mAudioHistoryReconstructedMatrix.resize( MultiSourceHandler::MAX_NR_OF_SPEAKERS );

        for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++  )
        {
            mAudioHistoryReconstructedMatrix.at( crtSpeaker ).resize( STFT_REQUIRED_PREVIOUS_FRAMES );

            for( size_t histFrame = 0; histFrame < STFT_REQUIRED_PREVIOUS_FRAMES; histFrame++ )
            {
                mAudioHistoryReconstructedMatrix.at( crtSpeaker ).at( histFrame ).resize( mFftSize, 0 );
            }
        }

        mAudioReconstructIndexCountVec.resize( MultiSourceHandler::MAX_NR_OF_SPEAKERS, 0 );

        mOverlapAddNormalizationVec.resize( mFftSize, 0 );

        mFftIndexCountVec.resize( MicArray::TOTAL_MICS_COUNT, 0 );

        mFftFeedMatrix.resize( MicArray::TOTAL_MICS_COUNT, std::vector<double>( mFftSize, 0 ) );

        mFftDirectValuesMatrix.resize( MicArray::TOTAL_MICS_COUNT, CxVector( mFftSize ) );

        mNoisePwrValuesMatrix.resize( MicArray::TOTAL_MICS_COUNT, std::vector<double>( mFftSize, 0 ) );
        mNoisePwrCounterVec.resize( MicArray::TOTAL_MICS_COUNT, 0 );
        mSigProcSettingNoisePwrVec.resize( MicArray::TOTAL_MICS_COUNT, false );

        mFftThreadsVec = std::vector<FftThread>( MicArray::TOTAL_MICS_COUNT );

        for( size_t crtMic = 0; crtMic < MicArray::TOTAL_MICS_COUNT; crtMic++ )
        {
            connect( &mFftThreadsVec.at( crtMic ), &FftThread::fftComputeDone, this, &RawSignalHandler::receiveNewFft, Qt::QueuedConnection );
        }

        mSignalProcessingWorkersVec.resize( MicArray::TOTAL_MICS_COUNT );
        mSignalProcessingThreadsVec.resize( MicArray::TOTAL_MICS_COUNT );

        for( size_t crtMic = 0; crtMic < MicArray::TOTAL_MICS_COUNT; crtMic++ )
        {
            SignalProcessingWorker* worker = new SignalProcessingWorker( crtMic );

            if( worker )
            {
                mSignalProcessingWorkersVec.at( crtMic ) = worker;
            }

            QThread* thread = new QThread();

            if( thread )
            {
                mSignalProcessingThreadsVec.at( crtMic ) = thread;
            }
        }

        for( size_t crtMic = 0; crtMic < MicArray::TOTAL_MICS_COUNT; crtMic++ )
        {
            if( mSignalProcessingWorkersVec.at( crtMic ) && mSignalProcessingThreadsVec.at( crtMic ) )
            {
                mSignalProcessingWorkersVec.at( crtMic )->moveToThread( mSignalProcessingThreadsVec.at( crtMic ) );
                connect( mSignalProcessingThreadsVec.at( crtMic ), &QThread::finished, mSignalProcessingWorkersVec.at( crtMic ), &QObject::deleteLater );
                mSignalProcessingThreadsVec.at( crtMic )->start();
            }
        }

        mInvFftThreadsVec = std::vector<FftThread>( MultiSourceHandler::MAX_NR_OF_SPEAKERS );

        for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
        {
            connect( &mInvFftThreadsVec.at( crtSpeaker ), &FftThread::fftComputeDone, this, &RawSignalHandler::receiveNewInvFft, Qt::QueuedConnection );
        }

        mPrevDenoisedPwrMatrix.resize( MicArray::TOTAL_MICS_COUNT, std::vector<double>( mFftSize, 0 ) );
        mSmoothedGainDenoisingMatrix.resize( MicArray::TOTAL_MICS_COUNT, std::vector<double>( mFftSize, 0 ) );

        mVadSpeakersVec.resize( MultiSourceHandler::MAX_NR_OF_SPEAKERS );

        for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
        {
            mVadSpeakersVec.at( crtSpeaker ) = new VoiceActivityDetection( this );
        }

        for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
        {
            mVadSpeakersVec.at( crtSpeaker )->setIndex( crtSpeaker );
            mVadSpeakersVec.at( crtSpeaker )->setSource( VoiceActivityDetection::SOURCE_SPEAKER );
        }

        mVadSpeakersHangoverCounterVec.resize( MultiSourceHandler::MAX_NR_OF_SPEAKERS, 0 );


        mNoiseOnlyLogicFifoMatrix.resize( MicArray::TOTAL_MICS_COUNT );

        for( size_t crtMic = 0; crtMic < MicArray::TOTAL_MICS_COUNT; crtMic++ )
        {
            mNoiseOnlyLogicFifoMatrix.at( crtMic ).resize( NOISE_ONLY_FRAMES, false );
        }

        mNoiseOnlyDataFifoMatrix.resize( MicArray::TOTAL_MICS_COUNT );

        for( size_t crtMic = 0; crtMic < MicArray::TOTAL_MICS_COUNT; crtMic++ )
        {
            mNoiseOnlyDataFifoMatrix.at( crtMic ).resize( mFftSize );

            for( size_t crtBin = 0; crtBin < mFftSize; crtBin++ )
            {
                mNoiseOnlyDataFifoMatrix.at( crtMic ).at( crtBin ).resize( NOISE_ONLY_FRAMES );
            }
        }

        mRnnMatrix.resize( MicArray::TOTAL_MICS_COUNT );

        for( size_t micI = 0; micI < MicArray::TOTAL_MICS_COUNT; micI++  )
        {
            mRnnMatrix.at( micI ).resize( MicArray::TOTAL_MICS_COUNT );

            for( size_t micJ = 0; micJ < MicArray::TOTAL_MICS_COUNT; micJ++ )
            {
                mRnnMatrix.at( micI ).at( micJ ).resize( mFftSize );
            }
        }

        connect( &mRnnComputeThread, &RnnComputeThread::rnnComputeDone, this, &RawSignalHandler::receiveNewRnn, Qt::QueuedConnection );

        mSpeakersCxMatrix.resize( mNumberOfSpeakers );

        for( size_t crtSpeaker = 0; crtSpeaker < mNumberOfSpeakers; crtSpeaker++ )
        {
            mSpeakersCxMatrix.at( crtSpeaker ).resize( mFftSize );
        }
    }
}


//!************************************************************************
//! Destructor
//!************************************************************************
RawSignalHandler::~RawSignalHandler()
{
    mFftThreadsVec.clear();
    mInvFftThreadsVec.clear();

    for( size_t i = 0; i < mVadSpeakersVec.size(); i++ )
    {
        delete mVadSpeakersVec.at( i );
        mVadSpeakersVec.at( i ) = nullptr;
    }

    for( size_t i = 0; i < mSignalProcessingWorkersVec.size(); i++ )
    {
        delete mSignalProcessingWorkersVec.at( i );
        mSignalProcessingWorkersVec.at( i ) = nullptr;
    }

    for( size_t i = 0; i < mSignalProcessingThreadsVec.size(); i++ )
    {
        if( mSignalProcessingThreadsVec.at( i ) )
        {
            if( mSignalProcessingThreadsVec.at( i )->isRunning() )
            {
                mSignalProcessingThreadsVec.at( i )->quit();
                mSignalProcessingThreadsVec.at( i )->wait();
            }

            delete mSignalProcessingThreadsVec.at( i );
            mSignalProcessingThreadsVec.at( i ) = nullptr;
        }
    }

    for( size_t i = 0; i < mRawSignalWorkersVec.size(); i++ )
    {
        delete mRawSignalWorkersVec.at( i );
        mRawSignalWorkersVec.at( i ) = nullptr;
    }

    for( size_t i = 0; i < mRawSignalThreadsVec.size(); i++ )
    {
        if( mRawSignalThreadsVec.at( i ) )
        {
            if( mRawSignalThreadsVec.at( i )->isRunning() )
            {
                mRawSignalThreadsVec.at( i )->quit();
                mRawSignalThreadsVec.at( i )->wait();
            }

            delete mRawSignalThreadsVec.at( i );
            mRawSignalThreadsVec.at( i ) = nullptr;
        }
    }
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
RawSignalHandler* RawSignalHandler::getInstance()
{
    if( !sInstance )
    {
        sInstance = new RawSignalHandler;
    }

    return sInstance;
}


//!************************************************************************
//! Instance destroyer
//!
//! @returns nothing
//!************************************************************************
void RawSignalHandler::destroyInstance()
{
    delete sInstance;
    sInstance = nullptr;
}


//!************************************************************************
//! Convert a FFT bin index to frequency
//!
//! @returns the frequency [Hz]
//!************************************************************************
double RawSignalHandler::convertBinIndex2Frequency
    (
    const size_t aBinIndex      //!< bin index, in [0 .. FFT_SIZE-1]
    ) const
{
    double f = 0;
    const double SPS = AudioCaptureThread::SAMPLE_RATE;

    if( mFftSize
     && aBinIndex < mFftSize )
    {
        if( aBinIndex <= mFftSize / 2 )
        {
            f = aBinIndex * SPS / mFftSize;
        }
        else
        {
            f = -( mFftSize - static_cast<double>( aBinIndex ) ) * SPS / mFftSize;
        }
    }

    return f;
}


//!************************************************************************
//! Convert a frequency to FFT bin index
//!
//! @returns the frequency [Hz]
//!************************************************************************
size_t RawSignalHandler::convertFrequency2BinIndex
    (
    const double aFrequency     //!< frequency [Hz], in [-Fs/2 .. +Fs/2)
    ) const
{
    int32_t nearestBin = 0;
    const double SPS = AudioCaptureThread::SAMPLE_RATE;

    if( aFrequency < SPS / 2.0
     && aFrequency >= -SPS / 2.0 )
    {
        double binIdx = aFrequency * mFftSize / SPS;
        nearestBin = static_cast<int32_t>( std::lround( binIdx ) );
        nearestBin %= static_cast<int32_t>( mFftSize );

        if( nearestBin < 0 )
        {
            nearestBin += static_cast<int32_t>( mFftSize );
        }
    }

    return static_cast<size_t>( nearestBin );
}


//!************************************************************************
//! Apply denoising (clean the signal)
//!
//! @returns nothing
//!************************************************************************
void RawSignalHandler::denoise
    (
    CxVector&       aCxSpectrumVec, //!< spectrum data
    const size_t    aMic            //!< mic index
    )
{
    size_t fftSize = aCxSpectrumVec.size();

    if( fftSize
     && aMic < MicArray::TOTAL_MICS_COUNT )
    {
        mMutex.lock();
            double gain = 1;
            const double MIN_NOISE_PWR = 1.e-12;
            const size_t CRT_MIC = aMic;

            for( size_t k = 0; k < fftSize; k++ )
            {
                double binPwr = std::norm( aCxSpectrumVec.at( k ) );
                double noisePwr = fmax( mNoisePwrValuesMatrix.at( CRT_MIC ).at( k ), MIN_NOISE_PWR );

                switch( mDenoisingGainType )
                {
                    case DENOISING_GAIN_WIENER_SIMPLIFIED:
                        {
                            //***********************************************
                            // no tunable parameters
                            //***********************************************

                            // a posteriori (observed) SNR
                            // ===========================
                            // Px = Ps + Pn
                            // gamma = Px/Pn = (Ps+Pn)/Pn = 1 + Ps/Pn
                            // gain = 1 - 1/gamma
                            if( binPwr > 0 )
                            {
                                double gamma = binPwr / noisePwr;
                                gain = fmax( 0.0, 1.0 - 1.0 / gamma );
                                gain = fmin( gain, 1.0 );
                            }
                            else
                            {
                                gain = 0;
                            }
                        }
                        break;

                    case DENOISING_GAIN_WIENER_PARAMETERIZED:
                        {
                            //***********************************************
                            // tunable parameters
                            //***********************************************
                            double gamma = 0.7;         // 0.50 .. 1.50
                            double gainFloor = 0.05;    // 0.01 .. 0.10
                            //***********************************************

                            double noiseRatio = 1;

                            if( binPwr > MIN_NOISE_PWR )
                            {
                                noiseRatio = mNoisePwrValuesMatrix.at( CRT_MIC ).at( k ) / binPwr;
                            }

                            double signalRatio = std::clamp( 1 - noiseRatio, 0.0, 1.0 );
                            gain = fmax( gainFloor, pow( signalRatio, gamma ) );
                        }
                        break;

                    case DENOISING_GAIN_WIENER_DECISION_DIRECTED:
                        {
                            //***********************************************
                            // tunable parameters
                            //***********************************************
                            double gainFloor = 0.05;                    // 0.01 .. 0.10
                            double temporalSmoothingNoiseCoeff = 0.65;  // 0.60 .. 0.70
                            double temporalSmoothingSpeechCoeff = 0.90; // 0.85 .. 0.95
                            double denoisedRetentionRatio = 0.8;        // 0.70 .. 0.90
                            //***********************************************

                            if( binPwr > 0 )
                            {
                                double gamma = binPwr / noisePwr;
                                gamma = fmax( 0.0, gamma );

                                // a priori SNR - decision directed (1984)
                                // =======================================
                                double alpha = 0.98;
                                double xi = alpha * mPrevDenoisedPwrMatrix.at( CRT_MIC ).at( k ) / noisePwr +
                                            ( 1 - alpha ) * fmax( 0.0, gamma - 1 );
                                xi = fmax( 0.0, xi );
                                gain = fmax( gainFloor, xi / ( 1 + xi ) );

                                // temporal smoothing - faster attack, slower release
                                double smoothCoeff = 0;

                                if( gain < mSmoothedGainDenoisingMatrix.at( CRT_MIC ).at( k ) )
                                {
                                    // noise -> remove faster
                                    smoothCoeff = temporalSmoothingNoiseCoeff;
                                }
                                else
                                {
                                    // speech -> rise slower
                                    smoothCoeff = temporalSmoothingSpeechCoeff;
                                }

                                mSmoothedGainDenoisingMatrix.at( CRT_MIC ).at( k ) *= smoothCoeff;
                                mSmoothedGainDenoisingMatrix.at( CRT_MIC ).at( k ) += ( 1 - smoothCoeff ) * gain;

                                gain = mSmoothedGainDenoisingMatrix.at( CRT_MIC ).at( k );
                                gain = std::clamp( gain, gainFloor, 1.0 );
                                double estimatedDenoisedPwr = gain * gain * binPwr;

                                mPrevDenoisedPwrMatrix.at( CRT_MIC ).at( k ) *= denoisedRetentionRatio;
                                mPrevDenoisedPwrMatrix.at( CRT_MIC ).at( k ) += ( 1 - denoisedRetentionRatio ) * estimatedDenoisedPwr;
                            }
                            else
                            {
                                gain = 0;
                            }
                        }
                        break;

                    default:
                        break;
                }

                aCxSpectrumVec.at( k ) *= gain;
            }
        mMutex.unlock();
    }
}


//!************************************************************************
//! Fill the vector with Overlap-Add normalization values
//!
//! @returns nothing
//!************************************************************************
void RawSignalHandler::fillOverlapAddNormalizationVec()
{
    mOverlapAddNormalizationVec = mWindowFunctionVec;

    for( int histFrame = STFT_REQUIRED_PREVIOUS_FRAMES - 1; histFrame >= 0; histFrame-- )
    {
        int offsetInHistoricFrame = STFT_WINDOW_TOTAL_FRAMES - ( STFT_REQUIRED_PREVIOUS_FRAMES - histFrame ) * STFT_WINDOW_HOP_FRAMES;

        for( int k = offsetInHistoricFrame; k < STFT_WINDOW_TOTAL_FRAMES; k++ )
        {
            mOverlapAddNormalizationVec.at( k - offsetInHistoricFrame ) += mWindowFunctionVec.at( k );
        }
    }
}


//!************************************************************************
//! Band-pass filtering
//! ** tapered cosine **
//!
//! @returns nothing
//!************************************************************************
void RawSignalHandler::filterBandPass
    (
    CxVector&       aCxSpectrumVec, //!< spectrum data
    const double    aFmin,          //!< lower cutoff
    const double    aFmax,          //!< upper cutoff
    const uint32_t  aSamplingRate   //!< sampling rate
    )
{
    size_t fftSize = aCxSpectrumVec.size();

    if( fftSize
        && 0 < aFmin
        && aFmin < aFmax
        && aSamplingRate )
    {
        double binWidth = static_cast<double>( aSamplingRate ) / fftSize;
        double transitionWidth = std::min( 3 * binWidth, 0.45 * ( aFmax - aFmin ) );

        for( size_t k = 0; k < fftSize; k++ )
        {
            double f = 0;

            if( k <= fftSize / 2 )
            {
                f = binWidth * k;
            }
            else
            {
                f = -binWidth * ( fftSize - k );
            }

            double absF = fabs( f );
            double gain = 1;

            // 1             ----------
            //              /|        |\
            //             / |        | \
            //            /  |        |  \           absF
            // 0 --|-----/   |        |   \------|---->
            //     |     |   |        |   |      |
            //     |     |  fMin     fMax |      |
            //     |   fMin-tW          fMax+tW  |
            //    20                            8000
            //
            //
            // fftSize | binWidth
            // ==================
            //   256   |  62.5
            //   512   |  31.3
            //           ------ 20 Hz
            //  1024   |  15.6
            //  2048   |   7.8
            //  4096   |   3.9

            double fMinAbsolute = std::max( AcousticsHandler::FREQUENCY_RANGE_VALUES.at( AcousticsHandler::FREQUENCY_RANGE_AUDIO ).min, aFmin - transitionWidth );
            double fMaxAbsolute = std::min( AcousticsHandler::FREQUENCY_RANGE_VALUES.at( AcousticsHandler::FREQUENCY_RANGE_AUDIO ).max, aFmax + transitionWidth );

            if( absF >= aFmin && absF <= aFmax )
            {
                gain = 1;
            }
            else if( absF >= fMinAbsolute && absF < aFmin )
            {
                double incRatio = ( absF - fMinAbsolute ) / ( aFmin - fMinAbsolute );
                gain = 0.5 * ( 1 - cos( Numeric::PI * incRatio ) );
            }
            else if( absF > aFmax && absF <= fMaxAbsolute )
            {
                double decRatio = ( absF - aFmax ) / ( fMaxAbsolute - aFmax );
                gain = 0.5 * ( 1 + cos( Numeric::PI * decRatio ) );
            }
            else
            {
                gain = 0;
            }

            aCxSpectrumVec.at( k ) *= gain;
        }
    }
}


//!************************************************************************
//! Return a reference to the active microphone array
//!
//! @returns: the reference to the active micophone array
//!************************************************************************
const MicArray& RawSignalHandler::getActiveMicArray() const
{
    QMutexLocker locker( &mMutableMutex );
    return mActiveMicArray;
}


//!************************************************************************
//! Return a reference to the 3D matrix of calculated values for active weights
//!
//! @returns: the reference to the 3D matrix with active weights
//!************************************************************************
const Cx3Matrix& RawSignalHandler::getActiveWeightsMatrix3d() const
{
    QMutexLocker locker( &mMutableMutex );
    return mActiveWeightsMatrix3d;
}


//!************************************************************************
//! Return a reference to the calculated values from direct FFTs for all mics
//!
//! @returns: the reference to the calculated values from direct FFTs
//!************************************************************************
const CxMatrix& RawSignalHandler::getFftDirectValuesMatrix() const
{
    QMutexLocker locker( &mMutableMutex );
    return mFftDirectValuesMatrix;
}


//!************************************************************************
//! Return a reference to the states of having data from all active mics
//!
//! @returns: the reference to the vector of states
//!************************************************************************
std::vector<bool>& RawSignalHandler::getHaveAllActiveMicsVec()
{
    QMutexLocker locker( &mMutableMutex );
    return mHaveAllActiveMicsVec;
}


//!************************************************************************
//! Return a reference to the vector with inverse FFT threads
//!
//! @returns: the reference to the vector with inverse FFT threads
//!************************************************************************
std::vector<FftThread>& RawSignalHandler::getInvFftThreadsVec()
{
    QMutexLocker locker( &mMutableMutex );
    return mInvFftThreadsVec;
}


//!************************************************************************
//! Return a reference to the 3D matrix with most recent noise-only data
//!
//! @returns: the reference to the matrix with most recent noise-only data
//!************************************************************************
std::vector<std::vector<std::deque<cdouble>>>& RawSignalHandler::getNoiseOnlyDataFifoMatrix()
{
    QMutexLocker locker( &mMutableMutex );
    return mNoiseOnlyDataFifoMatrix;
}


//!************************************************************************
//! Return a reference to the matrix with most recent noise-only truth states
//!
//! @returns: the reference to the matrix with truth states
//!************************************************************************
std::vector<std::deque<bool>>& RawSignalHandler::getNoiseOnlyLogicFifoMatrix()
{
    QMutexLocker locker( &mMutableMutex );
    return mNoiseOnlyLogicFifoMatrix;
}


//!************************************************************************
//! Return a reference to the matrix with noise power values for all mics
//!
//! @returns: the reference to the matrix with noise power values
//!************************************************************************
const std::vector<std::vector<double>>& RawSignalHandler::getNoisePwrValuesMatrix() const
{
    QMutexLocker locker( &mMutableMutex );
    return mNoisePwrValuesMatrix;
}


//!************************************************************************
//! Return a reference to the number of speakers
//!
//! @returns: the reference to the number of speakers
//!************************************************************************
const size_t& RawSignalHandler::getNumberOfSpeakers() const
{
    QMutexLocker locker( &mMutableMutex );
    return mNumberOfSpeakers;
}


//!************************************************************************
//! Return a reference to the noise threshold [dB]
//!
//! @returns: the reference to the noise threshold
//!************************************************************************
const double& RawSignalHandler::getNoiseThdDb() const
{
    QMutexLocker locker( &mMutableMutex );
    return mNoiseThdDb;
}


//!************************************************************************
//! Return a reference to the Rnn compute thread
//!
//! @returns the reference to the thread
//!************************************************************************
RnnComputeThread& RawSignalHandler::getRnnComputeThread()
{
    QMutexLocker locker( &mMutableMutex );
    return mRnnComputeThread;
}


//!************************************************************************
//! Return a reference to the wignal processing workers for all mics
//!
//! @returns the reference to the vector of signal processing workers
//!************************************************************************
std::vector<SignalProcessingWorker*> RawSignalHandler::getSignalProcessingWorkersVec()
{
    QMutexLocker locker( &mMutableMutex );
    return mSignalProcessingWorkersVec;
}


//!************************************************************************
//! Return a reference to the complex matrix with summed data for all speakers
//!
//! @returns: the reference to the complex matrix with summmed data
//!************************************************************************
CxMatrix& RawSignalHandler::getSpeakersCxMatrix()
{
    QMutexLocker locker( &mMutableMutex );
    return mSpeakersCxMatrix;
}


//!************************************************************************
//! Return a reference to the speech threshold [dB]
//!
//! @returns: the reference to the speech threshold
//!************************************************************************
const double& RawSignalHandler::getSpeechThdDb() const
{
    QMutexLocker locker( &mMutableMutex );
    return mSpeechThdDb;
}


//!************************************************************************
//! Return a reference to the VAD enabled state
//!
//! @returns: the reference to the VAD enabled state
//!************************************************************************
const bool& RawSignalHandler::getVadEnabled() const
{
    QMutexLocker locker( &mMutableMutex );
    return mVadEnabled;
}


//!************************************************************************
//! Return a reference to the vector of VAD hangover counters for all speakers
//!
//! @returns: the reference to the VAD hangover counter vector
//!************************************************************************
std::vector<int>& RawSignalHandler::getVadSpeakersHangoverCounterVec()
{
    QMutexLocker locker( &mMutableMutex );
    return mVadSpeakersHangoverCounterVec;
}


//!************************************************************************
//! Return a reference to the vector of VADs for all speakers
//!
//! @returns: the reference to the vector of VADs
//!************************************************************************
std::vector<VoiceActivityDetection*>& RawSignalHandler::getVadSpeakersVec()
{
    QMutexLocker locker( &mMutableMutex );
    return mVadSpeakersVec;
}


//!************************************************************************
//! Get the noise spectral covariance matrix Rnn
//! - all mics from the selected set must be in-range and unique,
//! otherwise the full Rnn is returned.
//!
//! - maximum size is ( TOTAL_MICS_COUNT x TOTAL_MICS_COUNT x mFftSize )
//! - minimum size is ( 2 x 2 x mFftSize )
//!
//! - for the full set of mics (N=15) the dimension of Rnn is
//! ( 15 x 15 x mFftSize )
//!
//! - for a subset of mics, e.g. {0,2,4,7,9,11,12} (N=7) the dimension of
//! Rnn is ( 7 x 7 x mFftSize )
//!
//!       |--                                           --|
//!       |   0,0   0,2   0,4   0,7   0,9   0,11   0,12   |
//!       |                                               |
//!       |   2,0   2,2   2,4   2,7   2,9   2,11   2,12   |
//!       |                                               |
//!       |   4,0   4,2   4,4   4,7   4,9   4,11   4,12   |
//!       |                                               |
//! rnn = |   7,0   7,2   7,4   7,7   7,9   7,11   7,12   |
//!       |                                               |
//!       |   9,0   9,2   9,4   9,7   9,9   9,11   9,12   |
//!       |                                               |
//!       |  11,0  11,2  11,4  11,7  11,9  11,11  11,12   |
//!       |                                               |
//!       |  12,0  12,2  12,4  12,7  12,9  12,11  12,12   |
//!       |--                                           --|
//!
//!
//! @returns: the noise spectral covariance matrix
//!************************************************************************
Cx3Matrix RawSignalHandler::getRnnMatrix
    (
    std::vector<size_t> aSelectedMicsVec //!< vector with selected/active microphones
    )
{
    Cx3Matrix rnn;
    const size_t N = aSelectedMicsVec.size();
    bool returnFullMatrix = ( MicArray::TOTAL_MICS_COUNT == N || N <= 1 );
    bool selectionValid = true; // all in-range and no duplicates
    std::unordered_set<size_t> seen;

    for( size_t crtMic : aSelectedMicsVec )
    {
        if( crtMic >= MicArray::TOTAL_MICS_COUNT || !seen.insert( crtMic ).second )
        {
            selectionValid = false;
            break;
        }
    }

    if( !selectionValid )
    {
        returnFullMatrix = true;
    }

    if( !returnFullMatrix )
    {
        std::sort( aSelectedMicsVec.begin(), aSelectedMicsVec.end() );
        rnn.resize( N );

        for( size_t i = 0; i < N; i++ )
        {
            rnn.at( i ).resize( N );

            for( size_t j = 0; j < N; j++ )
            {
                rnn.at( i ).at( j ).resize( mFftSize );
            }
        }
    }

    mMutex.lock();
        if( returnFullMatrix )
        {
            rnn = mRnnMatrix;
        }
        else
        {
            for( size_t row = 0; row < N; row++ )
            {
                for( size_t col = 0; col < N; col++ )
                {
                    rnn.at( row ).at( col ) = mRnnMatrix.at( aSelectedMicsVec.at( row ) ).at( aSelectedMicsVec.at( col ) );
                }
            }
        }
    mMutex.unlock();

    return rnn;
}


//!************************************************************************
//! Handle the base timer
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::handleBaseTimer()
{
    const int RMS_COMPUTE_MIN_DURATION_MS = 500; // time [ms] between displaying RMS updates
    const int TIMER_MULTIPLIER = ceil( static_cast<double>( RMS_COMPUTE_MIN_DURATION_MS ) /
                                       static_cast<double>( BASE_TIMER_MS ) );
    static int timerMultiplierCounter = 0;

    static std::vector<double> sumRmsMultiplierVec( MicArray::TOTAL_MICS_COUNT, 0 );

    //*************
    // base timer
    //*************
    for( size_t micIndex = 0; micIndex < MicArray::TOTAL_MICS_COUNT; micIndex++ )
    {
        mRmsBaseTimerVec.at( micIndex ) = 0;

        if( mCountRmsBaseTimerVec.at( micIndex ) )
        {
            mRmsBaseTimerVec.at( micIndex ) = mSumRmsBaseTimerVec.at( micIndex ) / mCountRmsBaseTimerVec.at( micIndex );
        }

        mSumRmsBaseTimerVec.at( micIndex ) = 0;
        mCountRmsBaseTimerVec.at( micIndex ) = 0;
    }

    //*************
    // multiplier timer
    //*************
    timerMultiplierCounter++;

    for( size_t micIndex = 0; micIndex < MicArray::TOTAL_MICS_COUNT; micIndex++ )
    {
        sumRmsMultiplierVec.at( micIndex ) += mRmsBaseTimerVec.at( micIndex );
    }

    if( 0 == timerMultiplierCounter % TIMER_MULTIPLIER )
    {
        timerMultiplierCounter = 0;

        for( size_t micIndex = 0; micIndex < MicArray::TOTAL_MICS_COUNT; micIndex++ )
        {
            mRmsMultiplierTimerVec.at( micIndex ) = sumRmsMultiplierVec.at( micIndex ) / TIMER_MULTIPLIER;
            sumRmsMultiplierVec.at( micIndex ) = 0;

            if( mRmsWindowShown )
            {
                ChannelValueDouble rmsInfo = { static_cast<int>( micIndex + 1 ), mRmsMultiplierTimerVec.at( micIndex ) };
                emit sendRms( rmsInfo );
            }
        }
    }
}


//!************************************************************************
//! Handle the RMS [dBFS] value
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::handleRms
    (
    ChannelValueDouble aRmsInfo //!< RMS [dBFS] information
    )
{
    if( 0 != aRmsInfo.channel )
    {
        mCountRmsBaseTimerVec.at( aRmsInfo.channel - 1 )++;
        mSumRmsBaseTimerVec.at( aRmsInfo.channel - 1 ) += aRmsInfo.value;
    }
}


//!************************************************************************
//! Receive new data sent by the audio capture thread
//! Expected to occur every 0.004 [s]
//!                 = 64 [samples] / 16000 [samples/s]
//!                 = AudioCaptureThread::FRAMES_PER_PERIOD / AudioCaptureThread::SAMPLE_RATE
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::receiveNewCapturedAudio
    (
    AudioChannelData aData      //!< new data
    )
{
    // process only for  aData.channel=1..15
    if( 0 != aData.channel )
    {
        emit haveNewRawAudio( aData );

        const size_t CRT_MIC = aData.channel - 1;
        const size_t DATA_LEN = aData.data.size();

        mMutex.lock();
            shiftLeftByN( mAudioDataMatrix.at( CRT_MIC ), DATA_LEN );
            std::copy( aData.data.begin(), aData.data.end(),
                       mAudioDataMatrix.at( CRT_MIC ).end() - DATA_LEN );

            mFftIndexCountVec.at( CRT_MIC ) += DATA_LEN;  // += 64 frames

            if( STFT_WINDOW_HOP_FRAMES == mFftIndexCountVec.at( CRT_MIC ) ) // every 128 frames
            {
                std::copy( mAudioDataMatrix.at( CRT_MIC ).begin(), mAudioDataMatrix.at( CRT_MIC ).end(),
                           mFftFeedMatrix.at( CRT_MIC ).begin() );

                mFftIndexCountVec.at( CRT_MIC ) = 0;

                FrequencyAnalysis* faInstance = FrequencyAnalysis::getInstance();

                if( faInstance && mFftThreadsVec.size() )
                {
                    for( size_t i = 0; i < mFftFeedMatrix.at( CRT_MIC ).size(); i++ )
                    {                     
                        mFftFeedMatrix.at( CRT_MIC ).at( i ) *= mWindowFunctionVec.at( i );
                    }

                    CxVector crtMicCxDataVector( mFftSize );

                    for( size_t i = 0; i < crtMicCxDataVector.size(); i++ )
                    {
                        crtMicCxDataVector.at( i ) = { mFftFeedMatrix.at( CRT_MIC ).at( i ), 0 };
                    }

                    mFftThreadsVec.at( CRT_MIC ).compute( crtMicCxDataVector, FrequencyAnalysis::FFT_SENSE_DIRECT, CRT_MIC, true );
                }
            }
        mMutex.unlock();
    }
}


//!************************************************************************
//! Receive computed FFT values from dedicated thread
//! Expected to occur every ( STFT_WINDOW_HOP_FRAMES / AudioCaptureThread::SAMPLE_RATE )
//! seconds.
//! -> see receiveNewCapturedAudio()
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::receiveNewFft
    (
    double*     aDataArray,     //!< output data array with computed FFT
    int         aLength,        //!< array length (2N + 1)
    int         aIndex          //!< compute index
    )
{
    if( aIndex >= 0 && aIndex < MicArray::TOTAL_MICS_COUNT )
    {
        const int CRT_MIC = aIndex;
        CxVector fftVec( ( aLength - 1 ) / 2 );

        if( aDataArray )
        {
            for( size_t i = 0; i < fftVec.size(); i++ )
            {
                fftVec.at( i ) = { aDataArray[2 * i + 1], aDataArray[2 * i + 2] };
            }
        }

        mMutex.lock();
            mFftDirectValuesMatrix.at( CRT_MIC ) = fftVec;
        mMutex.unlock();

        if( mSigProcSettingNoisePwrVec.at( CRT_MIC ) )
        {
            setNoisePwr( CRT_MIC );
        }
        else
        {
            if( mIsRedimensioning && !mSignalProcessingWorkersVec.at( CRT_MIC )->isPaused() )
            {
                mSignalProcessingWorkersVec.at( CRT_MIC )->setPaused( true );
            }
            else
            {
                mSignalProcessingWorkersVec.at( CRT_MIC )->processNewSpectrum( CRT_MIC );
            }
        }
    }
}


//!************************************************************************
//! Receive computed inverse FFT values from dedicated thread
//! Expected to occur every STFT_WINDOW_HOP_FRAMES samples per speaker.
//! Depending on the active mics from mActiveMicArray.getMicIndexes(),
//! this slot is *NOT* guaranteed to be synchronous with any particular
//! microphone from the entire array, as individual microphone direct FFT
//! slots are asynchronous and are not received in a particular order.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::receiveNewInvFft
    (
    double*  aDataArray, //!< array with computed inverse FFT values
    int      aLength,    //!< array length (2N + 1)
    int      aIndex      //!< compute index
    )
{
    if( aIndex >= 0 && aIndex < MultiSourceHandler::MAX_NR_OF_SPEAKERS )
    {
        const int CRT_SPEAKER = aIndex;

        mMutex.lock();
            if( aDataArray )
            {
                size_t fftSize = ( aLength - 1 ) / 2;

                if( fftSize && ( fftSize == mAudioReconstructedMatrix.at( CRT_SPEAKER ).size() ) )
                {
                    // Overlap-Add
                    //    histFrame = STFT_REQUIRED_PREVIOUS_FRAMES-1 is the farthest from most-recent reconstructed audio frame
                    //    histFrame = 0 is closest to most-recent reconstructed audio frame
                    //
                    // Example for STFT_REQUIRED_PREVIOUS_FRAMES=3
                    // -------------------------------------------
                    //
                    //   I---I---I---I---I                  <- histFrame = 2  (oldest)
                    //       I---I---I---I---I              <- histFrame = 1
                    //           I---I---I---I---I          <- histFrame = 0
                    //               I---I---I---I---I      <- current reconstructed audio after IFFT  (newest)
                    //               |   |
                    //               |hop|
                    //            -->|---|<--

                    for( int histFrame = STFT_REQUIRED_PREVIOUS_FRAMES - 1; histFrame > 0; histFrame-- )
                    {
                        mAudioHistoryReconstructedMatrix.at( CRT_SPEAKER ).at( histFrame ) = mAudioHistoryReconstructedMatrix.at( CRT_SPEAKER ).at( histFrame - 1 );
                    }

                    mAudioHistoryReconstructedMatrix.at( CRT_SPEAKER ).at( 0 ) = mAudioReconstructedMatrix.at( CRT_SPEAKER );

                    // current frame
                    for( size_t k = 0; k < fftSize; k++ )
                    {
                        mAudioReconstructedMatrix.at( CRT_SPEAKER ).at( k ) = aDataArray[2 * k + 1] / fftSize;
                    }

                    mAudioReconstructIndexCountVec.at( CRT_SPEAKER ) += STFT_WINDOW_HOP_PERIODS;

                    if( STFT_WINDOW_TOTAL_PERIODS == mAudioReconstructIndexCountVec.at( CRT_SPEAKER ) )
                    {
                        mAudioReconstructIndexCountVec.at( CRT_SPEAKER ) = 0;

                        std::vector<double> overlapAddedAudio( mAudioReconstructedMatrix.at( CRT_SPEAKER ) );

                        for( int histFrame = STFT_REQUIRED_PREVIOUS_FRAMES - 1; histFrame >= 0; histFrame-- )
                        {
                            int offsetInHistoricFrame = fftSize - ( STFT_REQUIRED_PREVIOUS_FRAMES - histFrame ) * STFT_WINDOW_HOP_FRAMES;

                            for( int k = offsetInHistoricFrame; k < fftSize; k++ )
                            {
                                overlapAddedAudio.at( k - offsetInHistoricFrame ) += mAudioHistoryReconstructedMatrix.at( CRT_SPEAKER ).at( histFrame ).at( k );
                            }
                        }

                        for( size_t k = 0; k < fftSize; k++ )
                        {
                            if( mOverlapAddNormalizationVec.at( k ) > 1.e-6 )
                            {
                                overlapAddedAudio.at( k ) /= mOverlapAddNormalizationVec.at( k );
                                overlapAddedAudio.at( k ) *= mOverlapAddGain;
                            }
                        }

                        AudioChannelData channelData;
                        channelData.channel = CRT_SPEAKER;
                        channelData.data.clear();

                        for( size_t k = 0; k < fftSize; k++ )
                        {
                            channelData.data.push_back( overlapAddedAudio.at( k ) );
                        }

                        emit haveNewSpeakerAudio( channelData, CRT_SPEAKER );
                    }
                }
            }
        mMutex.unlock();
    }
}


//!************************************************************************
//! Receive a new noise spectral covariance matrix
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::receiveNewRnn
    (
    const Cx3Matrix& aRnnMatrix //!< Rnn matrix
    )
{
    mMutex.lock();
        mRnnMatrix = aRnnMatrix;
    mMutex.unlock();
}


//!************************************************************************
//! Receive new beamformer weights for a speaker
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::receiveNewSpeakerWeights
    (
    CxMatrix aMatrix,       //!< weights for a speaker, over all active mics x FFT bins
    int      aSpeaker       //!< speaker index
    )
{
    mMutex.lock();
        if( aMatrix.size() == mActiveMicArray.getArray().size()
         && aSpeaker >= 0
         && aSpeaker < mNumberOfSpeakers )
        {
            mActiveWeightsMatrix3d.at( aSpeaker ) = aMatrix;
        }
    mMutex.unlock();
}


//!************************************************************************
//! Redimension the weights matrix
//! Its size must equal NR_OF_SPEAKERS x NR_OF_ACTIVE_MICS x FFT_SIZE
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::redimensionWeightsMatrix
    (
    const size_t    aNumberOfSpeakers,  //!< number of speakers
    const MicArray  aActiveMicArray     //!< optimum/active mic array
    )
{
    mMutex.lock();
        if( aNumberOfSpeakers >= MultiSourceHandler::MIN_NR_OF_SPEAKERS
         && aNumberOfSpeakers <= MultiSourceHandler::MAX_NR_OF_SPEAKERS
         && aActiveMicArray.getArray().size() )
        {
            mNumberOfSpeakers = aNumberOfSpeakers;
            mActiveMicArray = aActiveMicArray;


            if( mHaveAllActiveMicsVec.size() != mActiveMicArray.getArray().size() )
            {
                mHaveAllActiveMicsVec.clear();
                mHaveAllActiveMicsVec.resize( mActiveMicArray.getArray().size(), false );
            }


            if( mSpeakersCxMatrix.size() != mNumberOfSpeakers )
            {
                for( size_t crtSpeaker = 0; crtSpeaker < mSpeakersCxMatrix.size(); crtSpeaker++ )
                {
                    mSpeakersCxMatrix.at( crtSpeaker ).clear();
                }

                mSpeakersCxMatrix.clear();
                mSpeakersCxMatrix.resize( mNumberOfSpeakers );

                for( size_t crtSpeaker = 0; crtSpeaker < mNumberOfSpeakers; crtSpeaker++ )
                {
                    mSpeakersCxMatrix.at( crtSpeaker ).resize( mFftSize );
                }
            }


            if( mActiveWeightsMatrix3d.size() )
            {
                for( size_t crtSpeaker = 0; crtSpeaker < mActiveWeightsMatrix3d.size(); crtSpeaker++ )
                {
                    for( size_t crtActiveMic = 0; crtActiveMic < mActiveWeightsMatrix3d.at( 0 ).size(); crtActiveMic++ )
                    {
                        mActiveWeightsMatrix3d.at( crtSpeaker ).at( crtActiveMic ).clear();
                    }

                    mActiveWeightsMatrix3d.at( crtSpeaker ).clear();
                }

                mActiveWeightsMatrix3d.clear();
            }

            mActiveWeightsMatrix3d.resize( mNumberOfSpeakers );

            for( size_t crtSpeaker = 0; crtSpeaker < mNumberOfSpeakers; crtSpeaker++ )
            {
                mActiveWeightsMatrix3d.at( crtSpeaker ).resize( mActiveMicArray.getArray().size() );

                for( size_t crtActiveMic = 0; crtActiveMic < mActiveMicArray.getArray().size(); crtActiveMic++ )
                {
                    mActiveWeightsMatrix3d.at( crtSpeaker ).at( crtActiveMic ).resize( mFftSize );
                }
            }

            mIsRedimensioning = false;

            for( size_t crtMic = 0; crtMic < mSignalProcessingWorkersVec.size(); crtMic++ )
            {
                mSignalProcessingWorkersVec.at( crtMic )->setPaused( false );
            }
        }
    mMutex.unlock();
}


//!************************************************************************
//! Set the FFT size
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::setFftSize
    (
    const uint32_t aFFtSize     //!< FFT size
    )
{
    if( aFFtSize == STFT_WINDOW_TOTAL_FRAMES )
    {
        mMutex.lock();
            mFftSize = aFFtSize;

            mWindowFunctionVec.clear();
            mWindowFunctionVec.resize( mFftSize, 0 );
            setWindowFunction( mWindowType );

            mOverlapAddNormalizationVec.clear();
            mOverlapAddNormalizationVec.resize( mFftSize, 0 );
            fillOverlapAddNormalizationVec();

            for( size_t crtMic = 0; crtMic < MicArray::TOTAL_MICS_COUNT; crtMic++ )
            {
                mAudioDataMatrix.at( crtMic ).clear();
                mAudioDataMatrix.at( crtMic ).resize( mFftSize, 0 );

                mFftFeedMatrix.at( crtMic ).clear();
                mFftFeedMatrix.at( crtMic ).resize( mFftSize, 0 );

                mFftDirectValuesMatrix.at( crtMic ).clear();
                mFftDirectValuesMatrix.at( crtMic ).resize( mFftSize );

                mNoisePwrValuesMatrix.at( crtMic ).clear();
                mNoisePwrValuesMatrix.at( crtMic ).resize( mFftSize, 0 );

                mFftIndexCountVec.at( crtMic ) = 0;

                mPrevDenoisedPwrMatrix.at( crtMic ).clear();
                mPrevDenoisedPwrMatrix.at( crtMic ).resize( mFftSize, 0 );

                mSmoothedGainDenoisingMatrix.at( crtMic ).clear();
                mSmoothedGainDenoisingMatrix.at( crtMic ).resize( mFftSize, 0 );

                mNoiseOnlyDataFifoMatrix.at( crtMic ).clear();
                mNoiseOnlyDataFifoMatrix.at( crtMic ).resize( mFftSize );

                for( size_t crtBin = 0; crtBin < mFftSize; crtBin++ )
                {
                    mNoiseOnlyDataFifoMatrix.at( crtMic ).at( crtBin ).resize( NOISE_ONLY_FRAMES );
                }

                for( size_t secondMic = 0; secondMic < MicArray::TOTAL_MICS_COUNT; secondMic++ )
                {
                    mRnnMatrix.at( crtMic ).at( secondMic ).clear();
                    mRnnMatrix.at( crtMic ).at( secondMic ).resize( mFftSize );
                }
            }

            for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
            {
                mAudioReconstructedMatrix.at( crtSpeaker ).clear();
                mAudioReconstructedMatrix.at( crtSpeaker ).resize( mFftSize, 0 );

                for( size_t histFrame = 0; histFrame < STFT_REQUIRED_PREVIOUS_FRAMES; histFrame++ )
                {
                    mAudioHistoryReconstructedMatrix.at( crtSpeaker ).at( histFrame ).clear();
                    mAudioHistoryReconstructedMatrix.at( crtSpeaker ).at( histFrame ).resize( mFftSize, 0 );
                }

                mAudioReconstructIndexCountVec.at( crtSpeaker ) = 0;
            }

            for( size_t crtSpeaker = 0; crtSpeaker < mNumberOfSpeakers; crtSpeaker++ )
            {
                mSpeakersCxMatrix.at( crtSpeaker ).clear();
                mSpeakersCxMatrix.at( crtSpeaker ).resize( mFftSize );
            }
        mMutex.unlock();
    }
}


//!************************************************************************
//! Set the status about redimensioning vectors/matrixes
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::setIsRedimensioning
    (
    bool aState           //!< status
    )
{
    mMutex.lock();
        mIsRedimensioning = aState;
    mMutex.unlock();
}


//!************************************************************************
//! Set the noise power
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::setNoisePwr
    (
    const size_t    aMic        //!< mic index
    )
{
    if( aMic < MicArray::TOTAL_MICS_COUNT )
    {
        const uint16_t NOISE_LISTENING_DURATION_MS = 500;
        const int NOISE_PWR_FRAMES_COUNT = ceil( static_cast<double>( NOISE_LISTENING_DURATION_MS * AudioCaptureThread::SAMPLE_RATE ) /
                                                 static_cast<double>( 1000 * STFT_WINDOW_HOP_FRAMES ) );

        mMutex.lock();
            if( mNoisePwrCounterVec.at( aMic ) < NOISE_PWR_FRAMES_COUNT )
            {
                if( mNoisePwrValuesMatrix.at( aMic ).size() == mFftDirectValuesMatrix.at( aMic ).size() )
                {
                    mNoisePwrCounterVec.at( aMic )++;

                    for( size_t bin = 0; bin < mFftDirectValuesMatrix.at( aMic ).size(); bin++ )
                    {
                        double pwr = std::norm( mFftDirectValuesMatrix.at( aMic ).at( bin ) );
                        mNoisePwrValuesMatrix.at( aMic ).at( bin ) += pwr;
                    }
                }
            }
            else
            {
                for( size_t bin = 0; bin < mNoisePwrValuesMatrix.at( aMic ).size(); bin++ )
                {
                    mNoisePwrValuesMatrix.at( aMic ).at( bin ) /= NOISE_PWR_FRAMES_COUNT;
                }

                mSigProcSettingNoisePwrVec.at( aMic ) = false;
            }
        mMutex.unlock();
    }
}


//!************************************************************************
//! Set the noise threshold
//!
//! @returns: true if the value could be set
//!************************************************************************
bool RawSignalHandler::setNoiseThd
    (
    double aThresholdDb         //!< noise thd [dB]
    )
{
    bool status = false;

    if( aThresholdDb >= NOISE_THD.minDb
     && aThresholdDb <= NOISE_THD.maxDb )
    {
        mMutex.lock();
            mNoiseThdDb = aThresholdDb;
        mMutex.unlock();
        status = true;
    }

    return status;
}


//!************************************************************************
//! Set the Overlap-Add gain
//!
//! @returns: true if the value could be set
//!************************************************************************
bool RawSignalHandler::setOverlapAddGain
    (
    double aGain            //!< gain
    )
{
    bool status = false;

    if( aGain >= OLA_GAIN.min
     && aGain <= OLA_GAIN.max )
    {
        mMutex.lock();
            mOverlapAddGain = aGain;
        mMutex.unlock();
        status = true;
    }

    return status;
}


//!************************************************************************
//! Updates the status of having the RMS window shown
//! It allows emitting the signals with RMS values only when the window is
//! shown.
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::setRmsWindowShown
    (
    bool aStatus                //!< status
    )
{
    mRmsWindowShown = aStatus;
}


//!************************************************************************
//! Start setting the noise power
//! It must be run when *only* noise is present, no voice/speech/music etc.
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::setSigProcStartNoisePwr()
{
    mMutex.lock();
        for( size_t mic = 0; mic < mNoisePwrValuesMatrix.size(); mic++ )
        {
            for( size_t bin = 0; bin < mNoisePwrValuesMatrix.at( mic ).size(); bin++ )
            {
                mNoisePwrValuesMatrix.at( mic ).at( bin ) = 0;
            }
        }

        for( size_t mic = 0; mic < mNoisePwrCounterVec.size(); mic++ )
        {
            mNoisePwrCounterVec.at( mic ) = 0;
        }

        for( size_t mic = 0; mic < mSigProcSettingNoisePwrVec.size(); mic++ )
        {
            mSigProcSettingNoisePwrVec.at( mic ) = true;
        }
    mMutex.unlock();
}


//!************************************************************************
//! Set the speech threshold
//!
//! @returns: true if the value could be set
//!************************************************************************
bool RawSignalHandler::setSpeechThd
    (
    double aThresholdDb         //!< speech thd [dB]
    )
{
    bool status = false;

    if( aThresholdDb >= SPEECH_THD.minDb
     && aThresholdDb <= SPEECH_THD.maxDb )
    {
        mMutex.lock();
            mSpeechThdDb = aThresholdDb;
        mMutex.unlock();
        status = true;
    }

    return status;
}


//!************************************************************************
//! Set the window function
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::setWindowFunction
    (
    const WindowFunction::WindowFunctionType aWindow  //!< window function
    )
{
    if( aWindow < WindowFunction::WINDOW_FUNCTION_TYPE_MAX_COUNT )
    {
        mWindowType = aWindow;
        FrequencyAnalysis* faInstance = FrequencyAnalysis::getInstance();

        if( faInstance )
        {
            faInstance->getWindowFunction().setActiveType( mWindowType );
            double windowValue = 0;

            for( size_t i = 0; i < mWindowFunctionVec.size(); i++ )
            {
                faInstance->getWindowFunction().getWindowValue( i, &windowValue );
                mWindowFunctionVec.at( i ) = windowValue;
            }

            fillOverlapAddNormalizationVec();
        }
    }
}


//!************************************************************************
//! Shift left the elements of a vector by a number N
//! Last N elements remain unchanged.
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::shiftLeftByN
    (
    std::vector<double>&    aVector,    //!< vector
    const size_t            aN          //!< number of elements to shift
    ) const
{
    size_t len = aVector.size();

    if( aN
     && aN < len
     && len >= 2 )
    {
        for( size_t i = 0; i <= len - aN - 1; i++ )
        {
            aVector.at( i ) = aVector.at( i + aN );
        }
    }
}


//!************************************************************************
//! Shift right the elements of a vector by a number N
//! First N elements remain unchanged.
//!
//! @returns: nothing
//!************************************************************************
void RawSignalHandler::shiftRightByN
    (
    std::vector<double>&    aVector,    //!< vector
    const size_t            aN          //!< number of elements to shift
    ) const
{
    const size_t len = aVector.size();

    if( aN
     && aN < len
     && len >= 2 )
    {
        for( size_t i = len; i > aN; i-- )
        {
            aVector.at( i - 1 ) = aVector.at( i - aN - 1 );
        }
    }
}
