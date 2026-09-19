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
AsrThread.h

This file contains the definitions for the Automatic Speech Recognition thread.
*/

#ifndef AsrThread_h
#define AsrThread_h

#include "AudioChannelData.h"
#include "AsrRecognizer.h"

#include <cstdint>
#include <string>
#include <vector>

#include <QMutex>
#include <QString>
#include <QThread>


//************************************************************************
// Class for handling the Automatic Speech Recognition thread
//************************************************************************
class AsrThread : public QThread
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        AsrThread
            (
            QObject*    aParent = nullptr   //!< parent object
            );

        ~AsrThread();

        void feedAudioData
            (
            AudioChannelData    aData   //!< audio data
            );

        void setRecognizer
            (
            AsrRecognizer*      aAsrRecognizer  //!< ASR recognizer
            );

    protected:
        void run();

    private:
        std::string extractString
            (
            const char* aResult     //!< formatted result
            ) const;

    signals:
        void haveNewString
            (
            QString aText           //!< new text
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        QMutex                  mMutex;             //!< mutex

        VoskRecognizer*         mVoskAsrRecognizer; //!< VOSK ASR recognizer engine
        int                     mFrames;            //!< number of frames
        std::vector<int16_t>    mAsrBuffer;         //!< data buffer

        bool                    mIsAborting;        //!< true if aborting
};

#endif // AsrThread_h
