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
AsrModel.h

This file contains the definitions for the Automatic Speech Recognition model.
*/

#ifndef AsrModel_h
#define AsrModel_h

#include "vosk_api.h"

#include <cstdint>
#include <map>
#include <string>


//************************************************************************
// Class for handling the Automatic Speech Recognition model
//************************************************************************
class AsrModel
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        typedef enum : uint8_t
        {
            LANGUAGE_CHINESE,
            LANGUAGE_ENGLISH,
            LANGUAGE_FRENCH,
            LANGUAGE_GERMAN,
            LANGUAGE_JAPANESE,
            LANGUAGE_SPANISH,
            LANGUAGE_PORTUGUESE,

            // keep this last
            LANGUAGE_COUNT
        }Language;

    private:
        static const std::map<Language, std::string> LANGUAGE_NAMES;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        AsrModel
            (
            Language aLanguage      //!< language model
            );

        ~AsrModel();

        static AsrModel* getInstance
            (
            Language aLanguage      //!< language model
            );      

        uint8_t decLanguageCounter
            (
            const Language aLanguage    //!< language
            );

        static void destroyInstance();

        Language getLanguage() const;

        uint8_t getLanguageCounter
            (
            Language aLanguage      //!< language model
            ) const;

        VoskModel* getModel();

        uint8_t incLanguageCounter
            (
            const Language aLanguage    //!< language
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static AsrModel*            sInstance;      //!< singleton

        Language                    mLanguage;      //!< language model
        VoskModel*                  mModel;         //!< ASR model
        std::map<Language, uint8_t> mLanguageMap;   //!< language map
};

#endif // AsrModel_h
