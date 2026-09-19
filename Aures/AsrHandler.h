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
        AsrHandler
            (
            int                 aSpeakerId, //!< spreaker ID
            AsrModel::Language  aLanguage   //!< language
            );

        ~AsrHandler();

        AsrModel::Language getLanguage() const;

        bool setLanguage
            (
            AsrModel::Language aLanguage    //!< language
            );

    private slots:
        void forwardChangedRecognizer();

        void receiveNewSpeakerAudio
            (
            AudioChannelData    aData,  //!< new data
            int                 aIndex  //!< index
            );

        void receiveNewString
            (
            QString aString             //!< new string
            );

    signals:
        void changedRecognizer
            (
            bool    aStatus             //!< status
            );

        void haveNewString
            (
            QString aString,            //!< extracted string
            int     aSpeakerId          //!< speaker ID
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        int                 mSpeakerId;                 //!< speaker ID
        AsrModel::Language  mLanguage;                  //!< language
        AsrRecognizer*      mAsrLanguageRecognizer;     //!< ASR object, each handler must own a unique recognizer
        AsrThread           mAsrThread;                 //!< ASR thread
};

#endif // AsrHandler_h
