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
AsrModel.cpp

This file contains the sources for the Automatic Speech Recognition model.
*/

#include "AsrModel.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>


AsrModel* AsrModel::sInstance = nullptr;

const std::map<AsrModel::Language, std::string> AsrModel::LANGUAGE_NAMES =
{
    { LANGUAGE_ARABIC,      "Arabic"     },
    { LANGUAGE_CHINESE,     "Chinese"    },
    { LANGUAGE_DUTCH,       "Dutch"      },
    { LANGUAGE_ENGLISH,     "English"    },
    { LANGUAGE_FARSI,       "Farsi"      },
    { LANGUAGE_FRENCH,      "French"     },
    { LANGUAGE_GERMAN,      "German"     },
    { LANGUAGE_ITALIAN,     "Italian"    },
    { LANGUAGE_JAPANESE,    "Japanese"   },
    { LANGUAGE_KOREAN,      "Korean"     },
    { LANGUAGE_SPANISH,     "Spanish"    },
    { LANGUAGE_PORTUGUESE,  "Portuguese" }
};

AsrModel::Language AsrModel::sLanguage = AsrModel::LANGUAGE_ENGLISH;
VoskModel* AsrModel::sModel = nullptr;
std::map<AsrModel::Language, uint8_t> AsrModel::sLanguageMap = {};

//!************************************************************************
//! Constructor
//!************************************************************************
AsrModel::AsrModel
    (
    Language aLanguage  //!< language model
    )
{
    sLanguage = aLanguage;
    vosk_set_log_level( -1 );

    for( uint8_t i = 0; i < LANGUAGE_COUNT; i++ )
    {
        sLanguageMap.insert( { static_cast<Language>( i ), 0 } );
    }

    const char* HOME_PATH = getenv( "HOME" );
    char fullPath[256] = "";

    if( strlen( HOME_PATH ) )
    {
        strcpy( fullPath, HOME_PATH );
    }

    switch( sLanguage )
    {
        case LANGUAGE_ARABIC:
            strcat( fullPath, "/vosk_models/vosk-model-ar-mgb2-0.4" );
            break;

        case LANGUAGE_CHINESE:
            strcat( fullPath, "/vosk_models/vosk-model-small-cn-0.22" );
            break;

        case LANGUAGE_DUTCH:
            strcat( fullPath, "/vosk_models/vosk-model-small-nl-0.22" );
            break;

        case LANGUAGE_ENGLISH:
            strcat( fullPath, "/vosk_models/vosk-model-small-en-us-0.15" );
            break;

        case LANGUAGE_FARSI:
            strcat( fullPath, "/vosk_models/vosk-model-small-fa-0.42" );
            break;

        case LANGUAGE_FRENCH:
            strcat( fullPath, "/vosk_models/vosk-model-small-fr-0.22" );
            break;

        case LANGUAGE_GERMAN:
            strcat( fullPath, "/vosk_models/vosk-model-small-de-0.15" );
            break;

        case LANGUAGE_ITALIAN:
            strcat( fullPath, "/vosk_models/vosk-model-small-it-0.22" );
            break;

        case LANGUAGE_JAPANESE:
            strcat( fullPath, "/vosk_models/vosk-model-small-ja-0.22" );
            break;

        case LANGUAGE_KOREAN:
            strcat( fullPath, "/vosk_models/vosk-model-small-ko-0.22" );
            break;

        case LANGUAGE_SPANISH:
            strcat( fullPath, "/vosk_models/vosk-model-small-es-0.42" );
            break;

        case LANGUAGE_PORTUGUESE:
            strcat( fullPath, "/vosk_models/vosk-model-small-pt-0.3" );
            break;

        default:
            break;
    }

    sModel = vosk_model_new( fullPath );

    if( !sModel )
    {
        std::cout << "\nFailed loading the VOSK ASR model for " << LANGUAGE_NAMES.at( sLanguage ) << ".\n" << std::flush;
    }
}


//!************************************************************************
//! Destructor
//!************************************************************************
AsrModel::~AsrModel()
{
    if( sModel )
    {
        vosk_model_free( sModel );
        sModel = nullptr;
    }
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
AsrModel* AsrModel::getInstance
    (
    Language aLanguage      //!< language model
    )
{
    if( !sInstance
     || ( aLanguage != sLanguage ) )
    {
        if( sModel )
        {
            vosk_model_free( sModel );
            sModel = nullptr;

            if( sLanguageMap[sLanguage] > 0 )
            {
                sLanguageMap[sLanguage]--;
            }
        }

        sInstance = new AsrModel( aLanguage );
    }

    return sInstance;
}


//!************************************************************************
//! Decrement the counter for a specified language
//!
//! @returns new counter
//!************************************************************************
uint8_t AsrModel::decLanguageCounter
    (
    const Language aLanguage    //!< language
    )
{
    if( sLanguageMap[aLanguage] > 0 )
    {
        sLanguageMap[aLanguage]--;
    }

    return sLanguageMap[aLanguage];
}


//!************************************************************************
//! Instance destroyer
//!
//! @returns nothing
//!************************************************************************
void AsrModel::destroyInstance()
{
    delete sInstance;
    sInstance = nullptr;
}


//!************************************************************************
//! Get the language model
//!
//! @returns the language model
//!************************************************************************
AsrModel::Language AsrModel::getLanguage() const
{
    return sLanguage;
}


//!************************************************************************
//! Get the counter for a specified language
//!
//! @returns nothing
//!************************************************************************
uint8_t AsrModel::getLanguageCounter
    (
    Language aLanguage      //!< language model
    ) const
{
    return sLanguageMap.at( aLanguage );
}


//!************************************************************************
//! Get the ASR model
//!
//! @returns the ASR model
//!************************************************************************
VoskModel* AsrModel::getModel()
{
    return sModel;
}


//!************************************************************************
//! Increment the counter for a specified language
//!
//! @returns new counter
//!************************************************************************
uint8_t AsrModel::incLanguageCounter
    (
    const Language aLanguage    //!< language
    )
{
    if( sLanguageMap[aLanguage] < 255 )
    {
        sLanguageMap[aLanguage]++;
    }

    return sLanguageMap[aLanguage];
}
