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
BeampatternThread.h

This file contains the definitions for the beampattern compute thread.
*/

#ifndef BeampatternThread_h
#define BeampatternThread_h

#include "Beamforming.h"
#include "MicArray.h"
#include "Numeric.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>


//************************************************************************
// Class for handling the beampattern compute thread
//************************************************************************
class BeampatternThread : public QThread
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        BeampatternThread
            (
            QObject* aParent = nullptr              //!< parent object
            );

        ~BeampatternThread();

        void compute
            (
            Beamforming::BeampatternConfig  aBeampatternConfig, //!< configuration data
            MicArray                        aMicArray           //!< microphone array
            );

    protected:
        void run() override;

    signals:
        void beampatternComputeDone
            (
            CxVector aBeampatternCxValues   //!< beampattern values
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        QMutex          mMutex;             //!< mutex
        QWaitCondition  mWaitCondition;     //!< wait condition

        Beamforming::BeampatternConfig  mBeampatternConfig; //!< configuration data
        MicArray        mMicArray;          //!< microphone array

        bool            mIsRestarting;      //!< true if restarting
        bool            mIsAborting;        //!< true if aborting
};

#endif // BeampatternThread_h
