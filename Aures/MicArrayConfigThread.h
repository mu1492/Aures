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
MicArrayConfigThread.h

This file contains the definitions for the microphone array configuration thread.
*/

#ifndef MicArrayConfigThread_h
#define MicArrayConfigThread_h

#include "AcousticsHandler.h"
#include "MicArray.h"

#include <cstdint>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>


//************************************************************************
// Class for handling the microphone array configuration thread
//************************************************************************
class MicArrayConfigThread : public QThread
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        MicArrayConfigThread
            (
            QObject* aParent = nullptr              //!< parent object
            );

        ~MicArrayConfigThread();

        void compute
            (
            uint8_t                             aNrOfSpeakers,  //!< number of speakers
            double                              aAngle,         //!< angle of first speeker
            AcousticsHandler::FrequencyRange    aFrequencyRange //!< frequency range
            );

    protected:
        void run() override;

    signals:
        void micArrayConfigComputeDone
            (
            MicArray   aMicArray   //!< microphone array
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        QMutex          mMutex;             //!< mutex
        QWaitCondition  mWaitCondition;     //!< wait condition

        uint8_t         mNrOfSpeakers;      //!< number of speakers
        double          mFirstSpeakerAngle; //!< angle [rad] of the 1st speaker related to mic 5, trigonometric sense
        AcousticsHandler::FrequencyRange    mFrequencyRange; //!< frequency range

        bool            mIsRestarting;      //!< true if restarting
        bool            mIsAborting;        //!< true if aborting
};

#endif // MicArrayConfigThread_h
