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
VoiceActivityDetection.h

This file contains the definitions for the VAD (voice activity detection).
*/

#ifndef VoiceActivityDetection_h
#define VoiceActivityDetection_h

#include <cstdint>

#include <QMetaType>
#include <QObject>


//************************************************************************
// Class for handling the VAD (voice activity detection)
//************************************************************************
class VoiceActivityDetection : public QObject
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        typedef enum : uint8_t
        {
            SOURCE_UNKNOWN,
            SOURCE_MICROPHONE,
            SOURCE_DIRECTION,
            SOURCE_SPEAKER,

            // keep this last
            SOURCE_MAX_COUNT
        }Source;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        explicit VoiceActivityDetection
            (
            QObject* aParent = nullptr  //!< parent object
            );

        int getIndex() const;

        bool getIsVoice() const;

        Source getSource() const;

        void setIndex
            (
            int aIndex          //!< index
            );

        void setIsVoice
            (
            bool aIsVoice       //!< true if voice is detected
            );

        void setSource
            (
            Source aSource      //!< source
            );

    signals:
        void haveVoiceDetectionChanged
            (
            bool    aIsVoice,       //!< true if voice is detected
            int     aIndex,         //!< index
            Source  aSource         //!< source
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        Source  mSource;        //!< voice source
        int     mIndex;         //!< voice-associated index (mic / direction / speaker)
        bool    mIsVoice;       //!< true if voice is detected
};

Q_DECLARE_METATYPE( VoiceActivityDetection::Source )

#endif // VoiceActivityDetection_h
