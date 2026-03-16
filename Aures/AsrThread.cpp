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
AsrThread.cpp

This file contains the sources for the Automatic Speech Recognition thread.
*/

#include "AsrThread.h"

#include "AudioCaptureThread.h"


//!************************************************************************
//! Constructor
//!************************************************************************
AsrThread::AsrThread
    (
    QObject*    aParent     //!< parent object
    )
    : QThread( aParent )
    , mAsr( nullptr )
    , mAsrRecognizer( nullptr )
    , mFrames( 0 )
    , mIsAborting( false )
{
    mAsrBuffer.resize( AudioCaptureThread::FRAMES_PER_PERIOD );
}


//!************************************************************************
//! Destructor
//!************************************************************************
AsrThread::~AsrThread()
{
    mMutex.lock();
        mIsAborting = true;
    mMutex.unlock();

    wait();
}


//!************************************************************************
//! Extract the string from a formatted result
//!
//! @returns the extracted string
//!************************************************************************
std::string AsrThread::extractString
    (
    const char* aResult     //!< formatted result
    ) const
{
    std::string formattedString = aResult;
    size_t p1 = formattedString.find( '"' );
    size_t p2 = formattedString.find( '"', p1 + 1 );
    size_t p3 = formattedString.find( '"', p2 + 1 );
    size_t p4 = formattedString.find( '"', p3 + 1 );
    std::string extractedString = formattedString.substr( p3 + 1, p4 - p3 - 1 );

    return extractedString;
}


//!************************************************************************
//! Feed the ASR with new audio data
//!
//! @returns nothing
//!************************************************************************
void AsrThread::feedAudioData
    (
    AudioChannelData    aData   //!< audio data
    )
{
    QMutexLocker locker( &mMutex );
    mFrames = aData.data.size();

    for( int i = 0; i < mFrames; i++ )
    {
        mAsrBuffer.at( i ) = static_cast<int32_t>( aData.data.at( i ) * INT32_MAX ) >> 16;
    }

    if( !mIsAborting )
    {
        if( !isRunning() )
        {
            start();
        }
    }
}


//!************************************************************************
//! ASR thread main function
//!
//! @returns nothing
//!************************************************************************
/* virtual */ void AsrThread::run()
{    
    std::string extractedStr;
    static std::string oldExtractedStr;

    mMutex.lock();
        const int FRAMES = mFrames;
        int16_t* bufferData = mAsrBuffer.data();
    mMutex.unlock();

    if( mIsAborting )
    {
        return;
    }

    if( mAsrRecognizer )
    {
        if( vosk_recognizer_accept_waveform( mAsrRecognizer, ( const char* )bufferData, FRAMES * sizeof( int16_t ) ) )
        {
            extractedStr = extractString( vosk_recognizer_result( mAsrRecognizer ) );
        }
        else
        {
            extractedStr = extractString( vosk_recognizer_partial_result( mAsrRecognizer ) );
        }

        if( extractedStr != oldExtractedStr )
        {
            emit haveNewString( QString::fromUtf8( extractedStr.c_str() ) );
        }

        oldExtractedStr = extractedStr;
    }
}


//!************************************************************************
//! Set the ASR object
//!
//! @returns nothing
//!************************************************************************
void AsrThread::setAsr
    (
    AsrRecognizer*      aAsr    //!< ASR object
    )
{
    QMutexLocker locker( &mMutex );

    if( aAsr )
    {
        mAsr = aAsr;
        mAsrRecognizer = mAsr->getRecognizer();
    }
}
