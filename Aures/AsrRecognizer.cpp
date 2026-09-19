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


//!************************************************************************
//! Constructor
//!************************************************************************
AsrRecognizer::AsrRecognizer
    (
    AsrModel::Language aLanguage    //!< model language
    )
    : mLanguage( aLanguage )
    , mAsrModelInstance( AsrModel::getInstance( mLanguage ) )
    , mVoskRecognizer( nullptr )
{
    if( mAsrModelInstance )
    {
        mAsrModelInstance->incLanguageCounter( mLanguage );
        VoskModel* asrModel = mAsrModelInstance->getModel();

        if( asrModel )
        {
            mVoskRecognizer = vosk_recognizer_new( asrModel, AudioCaptureThread::SAMPLE_RATE );
        }
    }
}


//!************************************************************************
//! Destructor
//!************************************************************************
AsrRecognizer::~AsrRecognizer()
{
    if( mVoskRecognizer )
    {
        vosk_recognizer_free( mVoskRecognizer );
        mVoskRecognizer = nullptr;

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
//! Get the VOSK ASR recognizer
//!
//! @returns the VOSK ASR recognizer
//!************************************************************************
VoskRecognizer* AsrRecognizer::getVoskRecognizer()
{
    return mVoskRecognizer;
}


//!************************************************************************
//! Set a new recognizer language
//!
//! @returns: true if the language can be set
//!************************************************************************
bool AsrRecognizer::setLanguage
    (
    AsrModel::Language aLanguage    //!< language
    )
{
    bool status = true;

    if( aLanguage != mLanguage )
    {
        status = aLanguage < AsrModel::LANGUAGE_COUNT;

        if( status )
        {
            if( mVoskRecognizer )
            {
                vosk_recognizer_free( mVoskRecognizer );
                mVoskRecognizer = nullptr;

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

            status = false;
            mLanguage = aLanguage;
            mAsrModelInstance = AsrModel::getInstance( mLanguage );

            if( mAsrModelInstance )
            {
                mAsrModelInstance->incLanguageCounter( mLanguage );
                VoskModel* asrModel = mAsrModelInstance->getModel();

                if( asrModel )
                {
                    mVoskRecognizer = vosk_recognizer_new( asrModel, AudioCaptureThread::SAMPLE_RATE );

                    if( mVoskRecognizer )
                    {
                        status = true;
                        emit changedRecognizer();
                    }
                }
            }
        }
    }

    return status;
}
