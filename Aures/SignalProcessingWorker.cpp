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
SignalProcessingWorker.cpp

This file contains the sources for the signal processing worker.
*/

#include "SignalProcessingWorker.h"

#include "AcousticsHandler.h"
#include "AudioCaptureThread.h"
#include "RawSignalHandler.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <deque>
#include <stdexcept>
#include <vector>

#include <QDebug>


//!************************************************************************
//! Constructor
//!************************************************************************
SignalProcessingWorker::SignalProcessingWorker
    (
    size_t      aWorkerIndex,       //!< worker index
    QObject*    aParent
    )
    : mWorkerIndex( aWorkerIndex )
    , QObject( aParent )
    , mIsPaused( false )
{
}


//!************************************************************************
//! Get the paused status
//!
//! @returns true if the worker is paused
//!************************************************************************
bool SignalProcessingWorker::isPaused()
{
    return mIsPaused.load( std::memory_order_relaxed );
}


//!************************************************************************
//! Main worker activity
//!
//! Expected to occur every
//! RawSignalHandler::STFT_WINDOW_HOP_FRAMES / AudioCaptureThread::SAMPLE_RATE
//! seconds.
//! -> occurence rate depends on HOP size (related to STFT TOTAL size)
//! -> see RawSignalHandler::receiveNewFft()
//!
//! Operations from this method are subject to vectors and/or matrixes
//! redimensioning by other threads, therefore execution control must be
//! handled at any time.
//!
//! @returns nothing
//!************************************************************************
void SignalProcessingWorker::processNewSpectrum
    (
    int aIndex              //!< index
    )
{
    if( aIndex == mWorkerIndex )
    {
        RawSignalHandler* rawSignalHndl = RawSignalHandler::getInstance();
        AcousticsHandler* acousticsHndl = AcousticsHandler::getInstance();

        if( rawSignalHndl && acousticsHndl )
        {
            try
            {
                if( mIsPaused.load( std::memory_order_relaxed ) )
                {
                    return;
                }

                const size_t CRT_MIC = mWorkerIndex;

                const CxMatrix& FFT_DIRECT_VALUES_MATRIX = rawSignalHndl->getFftDirectValuesMatrix();
                CxVector crtMicProcessedFftCxVec = FFT_DIRECT_VALUES_MATRIX.at( CRT_MIC );

                const size_t FFT_SIZE = crtMicProcessedFftCxVec.size();
                const double BIN_WIDTH = static_cast<double>( AudioCaptureThread::SAMPLE_RATE ) / FFT_SIZE;
                const int K_SPEECH_MIN = acousticsHndl->getFrequencyRange().min / BIN_WIDTH;
                const int K_SPEECH_MAX = acousticsHndl->getFrequencyRange().max / BIN_WIDTH;
                const std::vector<std::vector<double>>& NOISE_PWR_VALUES_MATRIX = rawSignalHndl->getNoisePwrValuesMatrix();
                const double MIN_NOISE_PWR = 1.e-12;

                double crtMicTotalObservedPwr = 0;
                double crtMicTotalNoisePwr = 0;

                double crtMicSpeechObservedPwr = 0;
                double crtMicSpeechNoisePwr = 0;

                if( mIsPaused.load( std::memory_order_relaxed ) )
                {
                    return;
                }

                for( size_t k = 1; k <= FFT_SIZE / 2; k++ )
                {
                    double p = std::norm( crtMicProcessedFftCxVec.at( k ) );
                    double n = std::max( NOISE_PWR_VALUES_MATRIX.at( CRT_MIC ).at( k ), MIN_NOISE_PWR );

                    crtMicTotalObservedPwr += p;
                    crtMicTotalNoisePwr += n;

                    if( k >= K_SPEECH_MIN && k <= K_SPEECH_MAX )
                    {
                        crtMicSpeechObservedPwr += p;
                        crtMicSpeechNoisePwr += n;
                    }
                }

                double crtMicTotalSnr = crtMicTotalObservedPwr / std::max( crtMicTotalNoisePwr, MIN_NOISE_PWR );
                double crtMicSpeechSnr = crtMicSpeechObservedPwr / std::max( crtMicSpeechNoisePwr, MIN_NOISE_PWR );

                double crtMicTotalSnrDb = 10 * log10( std::max( crtMicTotalSnr, 1.e-12 ) );
                double crtMicSpeechSnrDb = 10 * log10( std::max( crtMicSpeechSnr, 1.e-12 ) );

                const double& NOISE_THD_DB = rawSignalHndl->getNoiseThdDb();
                bool crtMicHaveNoiseOnly = ( crtMicSpeechSnrDb < NOISE_THD_DB ) && ( crtMicTotalSnrDb < NOISE_THD_DB );

                if( mIsPaused.load( std::memory_order_relaxed ) )
                {
                    return;
                }

                //*///////////////////////////////////////////////
                // Rnn
                //*///////////////////////////////////////////////
                std::vector<std::deque<bool>>& noiseOnlyLogicFifoMatrix = rawSignalHndl->getNoiseOnlyLogicFifoMatrix();
                noiseOnlyLogicFifoMatrix.at( CRT_MIC ).pop_back();
                noiseOnlyLogicFifoMatrix.at( CRT_MIC ).push_front( crtMicHaveNoiseOnly );

                std::vector<std::vector<std::deque<cdouble>>>& noiseOnlyDataFifoMatrix = rawSignalHndl->getNoiseOnlyDataFifoMatrix();

                if( mIsPaused.load( std::memory_order_relaxed ) )
                {
                    return;
                }

                for( size_t crtBin = 0; crtBin < FFT_SIZE; crtBin++ )
                {
                    noiseOnlyDataFifoMatrix.at( CRT_MIC ).at( crtBin ).pop_back();
                    noiseOnlyDataFifoMatrix.at( CRT_MIC ).at( crtBin ).push_front( crtMicProcessedFftCxVec.at( crtBin ) );
                }

                if( MicArray::TOTAL_MICS_COUNT - 1 == CRT_MIC )
                {
                    bool rnnUpdate = true;

                    for( size_t crtMic = 0; crtMic < MicArray::TOTAL_MICS_COUNT; crtMic++ )
                    {
                        for( size_t crtFrame = 0; crtFrame < RawSignalHandler::NOISE_ONLY_FRAMES; crtFrame++ )
                        {
                            if( !noiseOnlyLogicFifoMatrix.at( crtMic ).at( crtFrame ) )
                            {
                                rnnUpdate = false;
                                break;
                            }
                        }

                        if( !rnnUpdate )
                        {
                            break;
                        }
                    }

                    if( mIsPaused.load( std::memory_order_relaxed ) )
                    {
                        return;
                    }

                    if( rnnUpdate )
                    {
                        RnnComputeThread& rnnComputeThread = rawSignalHndl->getRnnComputeThread();
                        rnnComputeThread.compute( noiseOnlyDataFifoMatrix, FFT_SIZE );
                    }
                }

                if( mIsPaused.load( std::memory_order_relaxed ) )
                {
                    return;
                }

                //*///////////////////////////////////////////////
                // Speakers signals
                //*///////////////////////////////////////////////
                const MicArray& ACTIVE_MIC_ARRAY = rawSignalHndl->getActiveMicArray();
                const std::vector<size_t> ACTIVE_MICS_VEC = ACTIVE_MIC_ARRAY.getMicIndexes();

                if( ACTIVE_MICS_VEC.size() )
                {
                    const size_t& NUMBER_OF_SPEAKERS = rawSignalHndl->getNumberOfSpeakers();
                    const bool& VAD_ENABLED = rawSignalHndl->getVadEnabled();
                    std::vector<bool>& haveAllActiveMicsVec = rawSignalHndl->getHaveAllActiveMicsVec();
                    CxMatrix& speakersCxMatrix = rawSignalHndl->getSpeakersCxMatrix();
                    const Cx3Matrix& ACTIVE_WEIGHTS_MATRIX_3D = rawSignalHndl->getActiveWeightsMatrix3d();
                    std::vector<int>& vadSpeakersHangoverCounterVec = rawSignalHndl->getVadSpeakersHangoverCounterVec();
                    std::vector<VoiceActivityDetection*> vadSpeakersVec = rawSignalHndl->getVadSpeakersVec();
                    std::vector<FftThread>& invFftThreadsVec = rawSignalHndl->getInvFftThreadsVec();

                    for( size_t crtSpeaker = 0; crtSpeaker < NUMBER_OF_SPEAKERS; crtSpeaker++ )
                    {
                        auto it = std::find( ACTIVE_MICS_VEC.begin(), ACTIVE_MICS_VEC.end(), CRT_MIC );

                        if( ACTIVE_MICS_VEC.end() != it )
                        {
                            size_t indexInActiveMicsVec = it - ACTIVE_MICS_VEC.begin();
                            haveAllActiveMicsVec.at( indexInActiveMicsVec ) = true;

                            if( mIsPaused.load( std::memory_order_relaxed ) )
                            {
                                return;
                            }

                            for( size_t crtBin = 0; crtBin < FFT_SIZE; crtBin++ )
                            {
                                speakersCxMatrix.at( crtSpeaker ).at( crtBin ) += crtMicProcessedFftCxVec.at( crtBin ) * std::conj( ACTIVE_WEIGHTS_MATRIX_3D.at( crtSpeaker ).at( indexInActiveMicsVec ).at( crtBin ) );
                            }
                        }

                        if( std::find( haveAllActiveMicsVec.begin(), haveAllActiveMicsVec.end(), false ) == haveAllActiveMicsVec.end() )
                        {
                            if( mIsPaused.load( std::memory_order_relaxed ) )
                            {
                                return;
                            }

                            //*///////////////////////////////////////////////
                            // VAD
                            //*///////////////////////////////////////////////
                            if( VAD_ENABLED )
                            {
                                double crtSpeakerTotalObservedPwr = 0;
                                double crtSpeakerTotalNoisePwr = 0;

                                double crtSpeakerSpeechObservedPwr = 0;
                                double crtSpeakerSpeechNoisePwr = 0;

                                for( size_t k = 1; k <= FFT_SIZE / 2; k++ )
                                {
                                    double p = std::norm( speakersCxMatrix.at( crtSpeaker ).at( k ) );
                                    crtSpeakerTotalObservedPwr += p;

                                    double crtSpeakerNoisePwrAtCrtBin = 0;

                                    for( size_t crtActiveMicIdx = 0; crtActiveMicIdx < ACTIVE_MICS_VEC.size(); crtActiveMicIdx++ )
                                    {
                                        crtSpeakerNoisePwrAtCrtBin += NOISE_PWR_VALUES_MATRIX.at( ACTIVE_MICS_VEC.at( crtActiveMicIdx ) ).at( k );
                                    }

                                    crtSpeakerNoisePwrAtCrtBin /= ACTIVE_MICS_VEC.size();
                                    double n = std::max( crtSpeakerNoisePwrAtCrtBin, MIN_NOISE_PWR );
                                    crtSpeakerTotalNoisePwr += n;

                                    if( k >= K_SPEECH_MIN && k <= K_SPEECH_MAX )
                                    {
                                        crtSpeakerSpeechObservedPwr += p;
                                        crtSpeakerSpeechNoisePwr += n;
                                    }
                                }

                                double crtSpeakerTotalSnr = crtSpeakerTotalObservedPwr / std::max( crtSpeakerTotalNoisePwr, MIN_NOISE_PWR );
                                double crtSpeakerSpeechSnr = crtSpeakerSpeechObservedPwr / std::max( crtSpeakerSpeechNoisePwr, MIN_NOISE_PWR );

                                double crtSpeakerTotalSnrDb = 10 * log10( std::max( crtSpeakerTotalSnr, 1.e-12 ) );
                                double crtSpeakerSpeechSnrDb = 10 * log10( std::max( crtSpeakerSpeechSnr, 1.e-12 ) );

                                const double& SPEECH_THD_DB = rawSignalHndl->getSpeechThdDb();
                                bool haveInstantaneousSpeech = ( crtSpeakerSpeechSnrDb > SPEECH_THD_DB ) && ( crtSpeakerTotalSnrDb > NOISE_THD_DB );

                                bool vadState = false;

                                if( haveInstantaneousSpeech )
                                {
                                    vadState = true;

                                    const uint16_t VOICE_PAUSE_MIN_DURATION_MS = 500;
                                    const int VAD_HANGOVER_FRAMES = ceil( static_cast<double>( VOICE_PAUSE_MIN_DURATION_MS ) /
                                                                          static_cast<double>( RawSignalHandler::STFT_WINDOW_TOTAL_PERIODS * RawSignalHandler::PERIOD_DURATION_MS ) );

                                    vadSpeakersHangoverCounterVec.at( crtSpeaker ) = VAD_HANGOVER_FRAMES;
                                }
                                else
                                {
                                    if( vadSpeakersHangoverCounterVec.at( crtSpeaker ) > 0 )
                                    {
                                        vadState = true;
                                        vadSpeakersHangoverCounterVec.at( crtSpeaker )--;
                                    }
                                }

                                vadSpeakersVec.at( crtSpeaker )->setIsVoice( vadState );
                            }

                            if( mIsPaused.load( std::memory_order_relaxed ) )
                            {
                                return;
                            }

                            invFftThreadsVec.at( crtSpeaker ).compute( speakersCxMatrix.at( crtSpeaker ), FrequencyAnalysis::FFT_SENSE_INVERSE, crtSpeaker, true );
                        }
                    }

                    if( std::find( haveAllActiveMicsVec.begin(), haveAllActiveMicsVec.end(), false ) == haveAllActiveMicsVec.end() )
                    {
                        std::fill( haveAllActiveMicsVec.begin(), haveAllActiveMicsVec.end(), false );

                        for( size_t crtSpeaker = 0; crtSpeaker < NUMBER_OF_SPEAKERS; crtSpeaker++ )
                        {
                            std::fill( speakersCxMatrix.at( crtSpeaker ).begin(), speakersCxMatrix.at( crtSpeaker ).end(), cdouble( 0.0, 0.0 ) );
                        }
                    }
                }
            }
            catch( const std::out_of_range& )
            {
                return;
            }
        }
    }
}


//!************************************************************************
//! Set the paused status
//!
//! @returns nothing
//!************************************************************************
void SignalProcessingWorker::setPaused
    (
    bool aState             //!< state
    )
{
    mIsPaused.store( aState, std::memory_order_relaxed );
}
