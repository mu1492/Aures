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
SignalProcessingWorker.h

This file contains the definitions for the signal processing worker.
*/

#ifndef SignalProcessingWorker_h
#define SignalProcessingWorker_h

#include <QObject>

#include <atomic>


//************************************************************************
// Class for handling the signal processing worker
//************************************************************************
class SignalProcessingWorker : public QObject
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        explicit SignalProcessingWorker
            (
            size_t      aWorkerIndex,       //!< worker index
            QObject*    aParent = nullptr   //!< parent
            );

        bool isPaused();

        void processNewSpectrum
            (
            int aIndex              //!< index
            );

        void setPaused
            (
            bool aState             //!< state
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        size_t           mWorkerIndex;  //!< worker index

        std::atomic_bool mIsPaused;     //!< paused status
};

#endif // SignalProcessingWorker_h
