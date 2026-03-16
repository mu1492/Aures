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
AsrRecognizer.h

This file contains the definitions for the Automatic Speech Recognition recognizer.
*/

#ifndef AsrRecognizer_h
#define AsrRecognizer_h

#include "vosk_api.h"

#include "AsrModel.h"


//************************************************************************
// Class for handling the Automatic Speech Recognition recognizer
//************************************************************************
class AsrRecognizer
{
    //************************************************************************
    // functions
    //************************************************************************
    public:
        AsrRecognizer
            (
            AsrModel::Language aLanguage    //!< model language
            );

        ~AsrRecognizer();

        VoskRecognizer* getRecognizer();


    //************************************************************************
    // variables
    //************************************************************************
    private:
        AsrModel::Language  mLanguage;          //!< language
        AsrModel*           mAsrModelInstance;  //!< ASR model instance
        VoskRecognizer*     mRecognizer;        //!< ASR recognizer
};

#endif // AsrRecognizer_h
