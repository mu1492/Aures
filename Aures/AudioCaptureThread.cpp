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
AudioCaptureThread.cpp

This file contains the sources for the audio capture thread.
*/

#include "AudioCaptureThread.h"

#include <iostream>


//!************************************************************************
//! Constructor
//!************************************************************************
AudioCaptureThread::AudioCaptureThread()
    : QThread()
    , mPcmDevice( nullptr )
    , mPcmParameters( nullptr )
    , mIsRunning( false )
{   
    snd_pcm_open( &mPcmDevice, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0 );

    snd_pcm_hw_params_alloca( &mPcmParameters );
    snd_pcm_hw_params_any( mPcmDevice, mPcmParameters );

    snd_pcm_hw_params_set_access( mPcmDevice, mPcmParameters, SND_PCM_ACCESS_RW_INTERLEAVED );
    snd_pcm_hw_params_set_format( mPcmDevice, mPcmParameters, FORMAT );
    snd_pcm_hw_params_set_channels( mPcmDevice, mPcmParameters, CHANNELS_PER_FRAME );
    snd_pcm_hw_params_set_rate( mPcmDevice, mPcmParameters, SAMPLE_RATE, 0 );

    snd_pcm_hw_params( mPcmDevice, mPcmParameters );
    snd_pcm_prepare( mPcmDevice );
}


//!************************************************************************
//! Destructor
//!************************************************************************
AudioCaptureThread::~AudioCaptureThread()
{
    mMutex.unlock();

    snd_pcm_drain( mPcmDevice );
    snd_pcm_close( mPcmDevice );
}


//!************************************************************************
//! Audio capture thread execution function
//! Do not move portions of code to run().
//!
//! @returns zero
//!************************************************************************
int AudioCaptureThread::exec()
{
    std::vector<int32_t> buffer32( FRAMES_PER_PERIOD * CHANNELS_PER_FRAME ); // 64 * 16 = 1024
    AudioChannelData channelData;
    const int TIMEOUT_MS = 1000 * FRAMES_PER_PERIOD * CHANNELS_PER_FRAME / SAMPLE_RATE; // 64 ms

    forever
    {
        if( mIsRunning )
        {
            mMutex.lock();
                snd_pcm_sframes_t frames = snd_pcm_readi( mPcmDevice, buffer32.data(), FRAMES_PER_PERIOD );

                if( frames > 0 )
                {
                    for( int crtChannel = 0; crtChannel < CHANNELS_PER_FRAME; crtChannel++ )
                    {
                        //*///////////////////////////////////////////////////////////////////////////////
                        // Data belonging to channel=0 is not used but still needs to be sent
                        // so that the timing for all 16 channels remains aligned.
                        //*///////////////////////////////////////////////////////////////////////////////

                        // EVAL-MICCANVASZ -> numbers [1-15]
                        channelData.channel = crtChannel;
                        channelData.data.clear();

                        for( int j = crtChannel; j < buffer32.size(); j += CHANNELS_PER_FRAME )
                        {
                            channelData.data.push_back( buffer32.at( j ) / static_cast<double>( INT32_MAX ) );
                        }

                        if( FRAMES_PER_PERIOD == channelData.data.size() )
                        {                            
                            emit haveNewCaptureAudio( channelData );
                        }
                    }
                }
                else if( -EPIPE == frames )
                {
                    snd_pcm_prepare( mPcmDevice );
                }
                else if( -EAGAIN == frames )
                {
                    snd_pcm_wait( mPcmDevice, TIMEOUT_MS );
                }
                else if( -EINTR == frames )
                {
                    break;
                }
            mMutex.unlock();
        }
        else
        {
            return 0;
        }
    }

    return 0;
}


//!************************************************************************
//! Audio capture thread main function
//! Do not move here portions from exec().
//!
//! @returns nothing
//!************************************************************************
/* virtual */ void AudioCaptureThread::run()
{
    mIsRunning = true;
    exec();
}


//!************************************************************************
//! Stop the thread
//!
//! @returns nothing
//!************************************************************************
void AudioCaptureThread::stop()
{
    QMutexLocker locker( &mMutex );
    mIsRunning = false;
}
