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
RawSignalHandler.h

This file contains the definitions for processing raw audio signals.
*/

#ifndef RawSignalHandler_h
#define RawSignalHandler_h

#include "AudioCaptureThread.h"
#include "AudioChannelData.h"
#include "FftThread.h"
#include "MicArray.h"
#include "Numeric.h"
#include "RawSignalWorker.h"
#include "RnnComputeThread.h"
#include "SignalProcessingWorker.h"
#include "VoiceActivityDetection.h"

#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QTimer>


//************************************************************************
// Class for handling the processing of raw audio signals
//************************************************************************
class RawSignalHandler : public QObject
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        typedef enum : uint8_t
        {
            DENOISING_GAIN_WIENER_SIMPLIFIED,
            DENOISING_GAIN_WIENER_PARAMETERIZED,
            DENOISING_GAIN_WIENER_DECISION_DIRECTED,
            // keep this last
            DENOISING_GAIN_MAX_COUNT
        }DenoisingGain;

        static constexpr double PERIOD_DURATION_MS = 1000.0 * AudioCaptureThread::FRAMES_PER_PERIOD / AudioCaptureThread::SAMPLE_RATE;
                                                                //!< duration of one ALSA period (4 ms)

        // Required amount of transient acoustic information per STFT window is [20-40] ms.
        // => for 32 ms window length => 8 periods * 4 ms/period
        static const uint8_t STFT_WINDOW_TOTAL_PERIODS = 8;     //!< total number of periods in a STFT window
                                                                //!< -> this value directly determines the FFT size,
                                                                //!< which is automatically set
        static const uint8_t STFT_WINDOW_HOP_PERIODS = STFT_WINDOW_TOTAL_PERIODS / 4;
                                                                //!< number of hop periods in a STFT window
                                                                //!< /4 for 75% overlap, /2 for 50% overlap etc.

        static const int BASE_TIMER_MS = PERIOD_DURATION_MS * ( STFT_WINDOW_TOTAL_PERIODS + 1 );
                                                                //!< duration of the base timer [ms]

        static const int STFT_REQUIRED_PREVIOUS_FRAMES = ceil( static_cast<double>( STFT_WINDOW_TOTAL_PERIODS ) /
                                                               static_cast<double>( STFT_WINDOW_HOP_PERIODS )  ) - 1;
                                                                //!< required previous frames (except current one)

        static const uint16_t STFT_WINDOW_TOTAL_FRAMES = STFT_WINDOW_TOTAL_PERIODS * AudioCaptureThread::FRAMES_PER_PERIOD;
                                                                //!< total number of frames in a STFT window (e.g. 16*64=1024)
                                                                //!< == FFT size for perfect alignment
                                                                //! -> see FrequencyAnalysis::FrequencyAnalysis() where the FFT size is selected

        static const uint16_t STFT_WINDOW_HOP_FRAMES = STFT_WINDOW_HOP_PERIODS * AudioCaptureThread::FRAMES_PER_PERIOD;
                                                                //!< number of hop frames in a STFT window (e.g. 4*64=256)

        static const uint16_t NOISE_ONLY_DURATION_MS = 500 ;    //!< duration to look for noise-only
        static const int NOISE_ONLY_FRAMES = NOISE_ONLY_DURATION_MS / ( STFT_WINDOW_HOP_PERIODS * PERIOD_DURATION_MS );
                                                                //!< number of frames to look for noise-only for all mics
                                                                //!< it determines how much content gets into the Rnn matrix

        typedef struct
        {
            double minDb;   //!< minimum threshold [dB]
            double maxDb;   //!< maximum threshold [dB]
            double scale;   //!< control scale factor
        }SignalThreshold;

        static constexpr SignalThreshold NOISE_THD = { 0.0, 6.0, 10.0 };

        static constexpr SignalThreshold SPEECH_THD = { 0.0, 40.0, 10.0 };

        typedef struct
        {
            double min;     //!< minimum gain
            double max;     //!< maximum gain
            double scale;   //!< control scale factor
        }OverlapAddGain;

        static constexpr OverlapAddGain OLA_GAIN = { 1.0, 2.0, 100.0 };


    //************************************************************************
    // functions
    //************************************************************************
    public:
        RawSignalHandler();

        ~RawSignalHandler();

        static RawSignalHandler* getInstance();

        static void destroyInstance();

        double convertBinIndex2Frequency
            (
            const size_t aBinIndex      //!< bin index
            ) const;

        size_t convertFrequency2BinIndex
            (
            const double aFrequency     //!< frequency [Hz]
            ) const;

        const MicArray& getActiveMicArray() const;

        const Cx3Matrix& getActiveWeightsMatrix3d() const;

        const CxMatrix& getFftDirectValuesMatrix() const;

        std::vector<bool>& getHaveAllActiveMicsVec();

        std::vector<FftThread>& getInvFftThreadsVec();

        std::vector<std::vector<std::deque<cdouble>>>& getNoiseOnlyDataFifoMatrix();

        std::vector<std::deque<bool>>& getNoiseOnlyLogicFifoMatrix();

        const std::vector<std::vector<double>>& getNoisePwrValuesMatrix() const;

        const double& getNoiseThdDb() const;

        const size_t& getNumberOfSpeakers() const;

        RnnComputeThread& getRnnComputeThread();

        std::vector<SignalProcessingWorker*> getSignalProcessingWorkersVec();

        CxMatrix& getSpeakersCxMatrix();

        const double& getSpeechThdDb() const;

        const bool& getVadEnabled() const;

        std::vector<int>& getVadSpeakersHangoverCounterVec();

        std::vector<VoiceActivityDetection*>& getVadSpeakersVec();

        Cx3Matrix getRnnMatrix
            (
            std::vector<size_t> aSelectedMicsVec //!< vector with selected/active microphones
            );

        void redimensionWeightsMatrix
            (
            const size_t    aNumberOfSpeakers,  //!< number of speakers
            const MicArray  aActiveMicArray     //!< optimum/active mic array
            );

        void setFftSize
            (
            const uint32_t aFFtSize     //!< FFT size
            );

        void setIsRedimensioning
            (
            bool aState                 //!< status
            );

        bool setNoiseThd
            (
            double aThresholdDb         //!< noise thd [dB]
            );

        bool setOverlapAddGain
            (
            double aGain                //!< gain
            );

        void setRmsWindowShown
            (
            bool aStatus                //!< status
            );

        void setSigProcStartNoisePwr();

        bool setSpeechThd
            (
            double aThresholdDb         //!< speech thd [dB]
            );

        void setWindowFunction
            (
            const WindowFunction::WindowFunctionType aWindow  //!< window function
            );

    private:
        void denoise
            (
            CxVector&       aCxSpectrumVec, //!< spectrum data
            const size_t    aMic            //!< mic index
            );

        void fillOverlapAddNormalizationVec();

        void filterBandPass
            (
            CxVector&       aCxSpectrumVec, //!< spectrum data
            const double    aFmin,          //!< lower cutoff
            const double    aFmax,          //!< upper cutoff
            const uint32_t  aSamplingRate   //!< sampling rate
            );

        void setNoisePwr
            (
            const size_t    aMic        //!< mic index
            );

        void shiftLeftByN
            (
            std::vector<double>&    aVector,    //!< vector
            const size_t            aN          //!< number of elements to shift
            ) const;

        void shiftRightByN
            (
            std::vector<double>&    aVector,    //!< vector
            const size_t            aN          //!< number of elements to shift
            ) const;

    public slots:
        void receiveNewSpeakerWeights
            (
            CxMatrix aMatrix,       //!< weights for a speaker, over all active mics x FFT bins
            int      aSpeaker       //!< speaker index
            );

    private slots:
        void handleBaseTimer();

        void handleRms
            (
            ChannelValueDouble aRmsInfo //!< RMS [dBFS] information
            );

        void receiveNewCapturedAudio
            (
            AudioChannelData aData      //!< new data
            );

        void receiveNewFft
            (
            double*  aDataArray, //!< array with computed FFT values
            int      aLength,    //!< array length (2N + 1)
            int      aIndex      //!< compute index
            );

        void receiveNewInvFft
            (
            double*  aDataArray, //!< array with computed inverse FFT values
            int      aLength,    //!< array length (2N + 1)
            int      aIndex      //!< compute index
            );

        void receiveNewRnn
            (
            const Cx3Matrix& aRnnMatrix //!< Rnn matrix
            );

    signals:
        void haveNewRawAudio
            (
            AudioChannelData aData      //!< new data
            );

        void haveNewSignalToProcess
            (
            int             aIndex      //!< index
            );

        void haveNewSpeakerAudio
            (
            AudioChannelData aData,     //!< new data
            int              aIndex     //!< speaker index
            );

        void sendRms
            (
            ChannelValueDouble aRmsInfo //!< RMS [dBFS] information
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static RawSignalHandler*            sInstance;                      //!< singleton
        QMutex                              mMutex;                         //!< mutex
        mutable QMutex                      mMutableMutex;                  //!< mutable mutex

        std::vector<RawSignalWorker*>       mRawSignalWorkersVec;           //!< vector of raw signal workers
        std::vector<QThread*>               mRawSignalThreadsVec;           //!< vector of threads for raw signals

        QTimer                              mBaseTimer;                     //!< base timer

        std::vector<double>                 mSumRmsBaseTimerVec;            //!< vector of sums for base timer RMS values
        std::vector<uint32_t>               mCountRmsBaseTimerVec;          //!< vector of counts for base timer RMS values

        std::vector<double>                 mRmsBaseTimerVec;               //!< vector of RMS for last >= BASE_TIMER_MS (e.g. 64 ms)
        std::vector<double>                 mRmsMultiplierTimerVec;         //!< vector of RMS for last >= multiplier x BASE_TIMER_MS (e.g. 512 ms)
        bool                                mRmsWindowShown;                //!< true if the RMS window is displayed

        WindowFunction::WindowFunctionType  mWindowType;                    //!< window function type
        std::vector<double>                 mWindowFunctionVec;             //!< vector with window function values

        bool                                mIsRedimensioning;              //!< true if redimensioning vectors/matrixes

        std::vector<std::vector<double>>    mAudioDataMatrix;               //!< audio data values from all mics
        std::vector<std::vector<double>>    mAudioReconstructedMatrix;      //!< reconstructed audio after signal processing from all mics
        std::vector<std::vector<std::vector<double>>>   mAudioHistoryReconstructedMatrix;   //!< reconstructed audio history after signal processing from all mics
        std::vector<uint32_t>               mAudioReconstructIndexCountVec; //!< vector with audio reconstructed frames index counts

        std::vector<double>                 mOverlapAddNormalizationVec;    //!< vector with normalization values for Overlap-Add
        double                              mOverlapAddGain;                //!< gain for the Overlap-Add

        uint32_t                            mFftSize;                       //!< FFT size
        std::vector<uint32_t>               mFftIndexCountVec;              //!< vector with FFT index counts

        std::vector<std::vector<double>>    mFftFeedMatrix;                 //!< values used to compute the direct FFT for all mics
        std::vector<FftThread>              mFftThreadsVec;                 //!< vector of FFT threads for all mics

        std::vector<SignalProcessingWorker*>    mSignalProcessingWorkersVec;//!< vector of signal processing workers
        std::vector<QThread*>               mSignalProcessingThreadsVec;    //!< vector of threads for signal processing

        std::vector<FftThread>              mInvFftThreadsVec;              //!< vector of inverse FFT threads for all mics
        CxMatrix                            mFftDirectValuesMatrix;         //!< calculated values from direct FFTs for all mics

        double                              mNoiseThdDb;                    //!< noise thershold [dB], it determines how much content gets into Rnn
        std::vector<std::vector<double>>    mNoisePwrValuesMatrix;          //!< noise power values for all mics
        std::vector<int>                    mNoisePwrCounterVec;            //!< noise power counter for all mics
        std::vector<bool>                   mSigProcSettingNoisePwrVec;     //!< true if setting the noise power for all mics

        DenoisingGain                       mDenoisingGainType;             //!< type of denoising gain
        std::vector<std::vector<double>>    mPrevDenoisedPwrMatrix;         //!< previously denoised values for all mics
        std::vector<std::vector<double>>    mSmoothedGainDenoisingMatrix;   //!< smoothed gain values for denoising for all mics

        double                              mSpeechThdDb;                   //!< speech threshold [dB]

        bool                                mVadEnabled;                    //!< true if VAD is enabled
        std::vector<VoiceActivityDetection*>    mVadSpeakersVec;            //!< vector of VADs for all speakers
        std::vector<int>                    mVadSpeakersHangoverCounterVec; //!< vector of VAD hangover counters for all speakers

        std::vector<std::deque<bool>>       mNoiseOnlyLogicFifoMatrix;      //!< matrix with most recent (FIFO) noise-only truth states
        std::vector<std::vector<std::deque<cdouble>>>   mNoiseOnlyDataFifoMatrix;   //!< 3D matrix with most recent (FIFO) noise-only data
        Cx3Matrix                           mRnnMatrix;                     //!< noise spectral covariance 3D matrix

        RnnComputeThread                    mRnnComputeThread;              //!< Rnn compute thread

        size_t                              mNumberOfSpeakers;              //!< number of speakers
        MicArray                            mActiveMicArray;                //!< optimum/active mic array
        Cx3Matrix                           mActiveWeightsMatrix3d;         //!< 3D matrix of calculated values for active weights

        std::vector<bool>                   mHaveAllActiveMicsVec;          //!< states of having data from all active mics
        CxMatrix                            mSpeakersCxMatrix;              //!< summed data for all speakers
};

#endif // RawSignalHandler_h
