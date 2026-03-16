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
RawSignalWorker.cpp

This file contains the sources for the audio raw signal worker.
*/

#include "RawSignalWorker.h"
#include "Numeric.h"

#include <algorithm>
#include <iterator>


//!************************************************************************
//! Constructor
//!************************************************************************
RawSignalWorker::RawSignalWorker
    (
    size_t  aWorkerIndex        //!< worker index
    )
    : mWorkerIndex( aWorkerIndex )
{
}


//!************************************************************************
//! Main worker activity
//!
//! @returns nothing
//!************************************************************************
/* slot */ void RawSignalWorker::processNewAudio
    (
    AudioChannelData aData  //!< new data
    )
{
    if( mWorkerIndex == ( aData.channel - 1 ) )
    {
        QList<double> periodDataList = aData.data;
        std::vector<double> periodDataVec( periodDataList.begin(), periodDataList.end() );

        Numeric* numericInstance = Numeric::getInstance();
        double rms = 0;

        if( numericInstance )
        {
            rms = numericInstance->calculateRms( periodDataVec );
            rms = 20.0 * log10( rms );
        }

        ChannelValueDouble rmsInfo = { aData.channel, rms };
        emit computeRmsDone( rmsInfo );
    }
}
