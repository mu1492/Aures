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
RnnComputeThread.h

This file contains the definitions for the Rnn matrix compute thread.
*/


#ifndef RnnComputeThread_h
#define RnnComputeThread_h

#include "Numeric.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <cstdint>
#include <deque>
#include <vector>


//************************************************************************
// Class for handling the Rnn matrix calculation thread
//************************************************************************
class RnnComputeThread : public QThread
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        RnnComputeThread
            (
            QObject* aParent = nullptr              //!< parent object
            );

        ~RnnComputeThread();

        void compute
            (
            std::vector<std::vector<std::deque<cdouble>>>   aNoiseDataMatrix,   //!< noise data matrix
            uint32_t                                        aFftSize            //!< FFT size
            );

    protected:
        void run() override;

    signals:
        void rnnComputeDone
            (
            const Cx3Matrix& aRnnMatrix    //!< Rnn matrix
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        QMutex                      mMutex;         //!< mutex
        QWaitCondition              mWaitCondition; //!< wait condition

        std::vector<std::vector<std::deque<cdouble>>>   mNoiseDataMatrix;   //!< noise data matrix
        uint32_t                    mFftSize;       //!< FFT size

        bool                        mIsComputing;   //!< true if currently computing
        bool                        mIsAborting;    //!< true if aborting
};

#endif // RnnComputeThread_h
