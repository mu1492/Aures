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
AsrHandler.h

This file contains the definitions for the Automatic Speech Recognition handler.
*/

#ifndef AsrHandler_h
#define AsrHandler_h

#include "AsrRecognizer.h"
#include "AsrThread.h"
#include "AudioChannelData.h"

#include <vector>

#include <QObject>
#include <QString>


//************************************************************************
// Class for handling the Automatic Speech Recognition
//************************************************************************
class AsrHandler : public QObject
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        AsrHandler();

        ~AsrHandler();

        static AsrHandler* getInstance();

        static void destroyInstance();

    private slots:
        void receiveNewAudio
            (
            AudioChannelData aData      //!< new data
            );

        void receiveNewString
            (
            QString aString             //!< nex string
            );

    signals:
        void haveNewString
            (
            QString aString             //!< extracted string
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static AsrHandler*      sInstance;      //!< singleton

        static AsrRecognizer*   sAsrEn;         //!< ASR object - English
        AsrThread               mAsrThreadEn;   //!< ASR thread - English
};

#endif // AsrHandler_h
