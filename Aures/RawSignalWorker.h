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
RawSignalWorker.h

This file contains the definitions for the audio raw signal worker.
*/

#ifndef RawSignalWorker_h
#define RawSignalWorker_h

#include "AudioChannelData.h"

#include <cstddef>

#include <QObject>


//************************************************************************
// Class for handling the audio raw signal worker
//************************************************************************
class RawSignalWorker : public QObject
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        RawSignalWorker
            (
            size_t aWorkerIndex     //!< worker index
            );

    public slots:
        void processNewAudio
            (
            AudioChannelData aData  //!< new data
            );

    signals:
        void computeRmsDone
            (
            ChannelValueDouble aRmsInfo //!< RMS [dBFS] information
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        size_t  mWorkerIndex;       //!< worker index
};

#endif // RawSignalWorker_h
