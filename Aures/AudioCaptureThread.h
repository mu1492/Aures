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
AudioCaptureThread.h

This file contains the definitions for the audio capture thread.
*/

#ifndef AudioCaptureThread_h
#define AudioCaptureThread_h

#include "AudioChannelData.h"

#include <alsa/asoundlib.h>

#include <QMutex>
#include <QThread>


//************************************************************************
// Class for handling the audio capture thread
//************************************************************************
class AudioCaptureThread : public QThread
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        // SPS = 16 kHz
        static const int SAMPLE_RATE = 16000;

        // number of audio frames per period <=> ALSA parameter PERIOD SIZE [16..4096]
        // PERIOD SIZE <= (PERIOD BYTES max) / ( Bytes/frame )
        // PERIOD SIZE <= (PERIOD BYTES max) / ( CHANNELS * Bytes/sample )
        // PERIOD SIZE <= 4096 / ( 16 * 4 ) = 64
        static const int FRAMES_PER_PERIOD = 64; // 64 is maximum for TDM16 and S32_LE

    private:
        const char* AUDIO_DEVICE =
#if BUILD_JETSON_ORIN_NANO
            "hw:1,0"
#elif BUILD_JETSON_AGX_ORIN
            "hw:1,1"
#endif
        ;       

        // int32_t <=> 4 Bytes / channel
        static const snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S32_LE;

        // 16 for TDM16 <=> ALSA parameter CHANNELS [1..16]
        static const int CHANNELS_PER_FRAME = 16;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        AudioCaptureThread();

        ~AudioCaptureThread();

        void stop();

    protected:
        void run();

    private:
        int exec();

    signals:
        void haveNewAudio
            (
            AudioChannelData aData      //!< new data
            );      


    //************************************************************************
    // variables
    //************************************************************************
    private:
        snd_pcm_t*              mPcmDevice;         //!< PCM device
        snd_pcm_hw_params_t*    mPcmParameters;     //!< PCM parameters

        bool                    mIsRunning;         //!< true if thread is running
        QMutex                  mMutex;             //!< mutex
};

#endif // AudioCaptureThread_h
