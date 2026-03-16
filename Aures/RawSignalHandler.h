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
RawSignalHandler.h

This file contains the definitions for processing raw audio signals.
*/

#ifndef RawSignalHandler_h
#define RawSignalHandler_h

#include "AudioChannelData.h"
#include "MicArray.h"
#include "RawSignalWorker.h"

#include <cstdint>
#include <memory>
#include <vector>

#include <QObject>
#include <QThread>
#include <QTimer>


//************************************************************************
// Class for handling the processing of raw audio signals
//************************************************************************
class RawSignalHandler : public QObject
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        RawSignalHandler();

        ~RawSignalHandler();

        static RawSignalHandler* getInstance();

        static void destroyInstance();

    private slots:
        void handleRms
            (
            ChannelValueDouble aRmsInfo //!< RMS [dBFS] information
            );

        void handleTimer20Ms();

        void receiveNewAudio
            (
            AudioChannelData aData      //!< new data
            );

    signals:
        void sendNewAudio
            (
            AudioChannelData aData      //!< new data
            );

        void sendRms
            (
            ChannelValueDouble aRmsInfo //!< RMS [dBFS] information
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static RawSignalHandler*        sInstance;              //!< singleton

        MicArray*                       mMicArrayHandler;       //!< microphone array handler
        size_t                          mMicArraySize;          //!< number of microphones

        std::vector<RawSignalWorker*>   mRawSignalWorkersVec;   //!< vector of raw signal workers
        std::vector<QThread*>           mRawSignalThreadsVec;   //!< vector of threads for raw signals

        QTimer                          m20MsTimer;            //!< 20ms timer

        std::vector<double>             mSumRms20MsVec;        //!< vector of sums for RMS values @20ms
        std::vector<uint32_t>           mCountRms20MsVec;      //!< vector of counts for RMS values @20ms

        std::vector<double>             mRms020MsVec;           //!< vector of RMS for last 20ms
        std::vector<double>             mRms100MsVec;           //!< vector of RMS for last 100ms
        std::vector<double>             mRms500MsVec;           //!< vector of RMS for last 500ms
};

#endif // RawSignalHandler_h
