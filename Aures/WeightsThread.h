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
WeightsThread.h

This file contains the definitions for the weights compute thread.
*/

#ifndef WeightsThread_h
#define WeightsThread_h

#include "Beamforming.h"
#include "MicArray.h"
#include "MultiSourceHandler.h"
#include "Numeric.h"

#include <array>

#include <QMutex>
#include <QThread>
#include <QWaitCondition>


//************************************************************************
// Class for handling the weights compute thread
//************************************************************************
class WeightsThread : public QThread
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    private:
        static constexpr std::array<double, MultiSourceHandler::MAX_NR_OF_SPEAKERS> RAD_APOD_FACTOR_VEC =
                 { 1.0, 1.25, 1.0, 1.125612, 1.358720, 1.593176, 1.831110 }; //!< vector with factors for radial apodization


    //************************************************************************
    // functions
    //************************************************************************
    public:
        WeightsThread
            (
            QObject* aParent = nullptr              //!< parent object
            );

        ~WeightsThread();

        void compute
            (
            Beamforming::BeampatternConfig  aBeampatternConfig, //!< configuration data
            MicArray                        aMicArray,          //!< microphone array
            int                             aIndex              //!< index
            );

    protected:
        void run() override;

    private:
        double getRadialApodizationFactor
            (
            const size_t aNrOfMics      //!< number of microphones
            ) const;

    signals:
        void weightsComputeDone
            (
            CxMatrix aWeightsValues,    //!< weights values
            int      aIndex             //!< index
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        QMutex              mMutex;             //!< mutex
        QWaitCondition      mWaitCondition;     //!< wait condition

        Beamforming::BeampatternConfig  mBeampatternConfig; //!< configuration data
        MicArray            mMicArray;          //!< microphone array
        int                 mIndex;             //!< index

        bool                mIsRestarting;      //!< true if restarting
        bool                mIsAborting;        //!< true if aborting
};

#endif // WeightsThread_h
