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
AsrRecognizer.cpp

This file contains the sources for the Automatic Speech Recognition recognizer.
*/

#include "AsrRecognizer.h"

#include "AudioCaptureThread.h"

#include <iostream>


//!************************************************************************
//! Constructor
//!************************************************************************
AsrRecognizer::AsrRecognizer
    (
    AsrModel::Language aLanguage    //!< model language
    )
    : mLanguage( aLanguage )
    , mAsrModelInstance( AsrModel::getInstance( mLanguage ) )
    , mRecognizer( nullptr )
{
    if( mAsrModelInstance )
    {
        mAsrModelInstance->incLanguageCounter( mLanguage );
        VoskModel* asrModel = mAsrModelInstance->getModel();

        if( asrModel )
        {
            mRecognizer = vosk_recognizer_new( asrModel, AudioCaptureThread::SAMPLE_RATE );
        }
    }

    if( !mRecognizer )
    {
        std::cout << "ERROR: ASR engine could not be created." << std::endl;
    }
}


//!************************************************************************
//! Destructor
//!************************************************************************
AsrRecognizer::~AsrRecognizer()
{
    if( mRecognizer )
    {
        std::cout << "Unloading the ASR recognizer..";
        vosk_recognizer_free( mRecognizer );
        std::cout << " done." << std::endl << std::flush;

        mRecognizer = nullptr;

        if( mAsrModelInstance )
        {
            uint8_t langCtr = mAsrModelInstance->getLanguageCounter( mLanguage );

            if( langCtr )
            {
                langCtr = mAsrModelInstance->decLanguageCounter( mLanguage );
            }

            if( 0 == langCtr )
            {
                mAsrModelInstance->destroyInstance();
                mAsrModelInstance = nullptr;
            }
        }
    }
}


//!************************************************************************
//! Get the ASR recognizer
//!
//! @returns the ASR recognizer
//!************************************************************************
VoskRecognizer* AsrRecognizer::getRecognizer()
{
    return mRecognizer;
}
