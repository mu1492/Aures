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
AsrHandler.cpp

This file contains the sources for the Automatic Speech Recognition handler.
*/

#include "AsrHandler.h"


AsrHandler* AsrHandler::sInstance = nullptr;

AsrRecognizer* AsrHandler::sAsrEn = nullptr;

//!************************************************************************
//! Constructor
//!************************************************************************
AsrHandler::AsrHandler()
{
    sAsrEn = new AsrRecognizer( AsrModel::LANGUAGE_ENGLISH );

    if( sAsrEn )
    {
        mAsrThreadEn.setAsr( sAsrEn );
        connect( &mAsrThreadEn, &AsrThread::haveNewString, this, &AsrHandler::receiveNewString );
    }
}


//!************************************************************************
//! Destructor
//!************************************************************************
AsrHandler::~AsrHandler()
{
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
AsrHandler* AsrHandler::getInstance()
{
    if( !sInstance )
    {
        sInstance = new AsrHandler;
    }

    return sInstance;
}


//!************************************************************************
//! Instance destroyer
//!
//! @returns nothing
//!************************************************************************
void AsrHandler::destroyInstance()
{
    delete sAsrEn;

    delete sInstance;
    sInstance = nullptr;
}


//!************************************************************************
//! Receive new data sent by the audio capture thread
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void AsrHandler::receiveNewAudio
    (
    AudioChannelData aData      //!< new data
    )
{   
    // select front-most microphone on external circle
    const int CRT_MIC = 5;

    if( CRT_MIC == aData.channel )
    {
        mAsrThreadEn.feedAudioData( aData );
    }
}


//!************************************************************************
//! Receive new string from ASR thread
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void AsrHandler::receiveNewString
    (
    QString aString             //!< nex string
    )
{
    emit haveNewString( aString );
}
