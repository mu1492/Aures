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
BeampatternLayout.h

This file contains the definitions for the beampattern layout.
*/

#ifndef BeampatternLayout_h
#define BeampatternLayout_h

#include "Numeric.h"

#include <cstdint>

#include <QDial>


//************************************************************************
// Class for handling the beampattern layout
//************************************************************************
class BeampatternLayout : public QDial
{       
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        explicit BeampatternLayout
            (
            QWidget*    aParent = nullptr    //!< parent widget
            );

        void setShowSpeakers
            (
            const bool      aEnabled,               //!< state
            const double    aFirstSpeakerAngleDeg,  //!< angle [deg]
            const uint8_t   aNrOfSpeakers           //!< number of speakers
            );

        void updateBeampatternPlot
            (
            CxVector    aVector   //!< beampattern values
            );

    protected:
        void paintEvent
            (
            QPaintEvent*    aEvent  //!< paint event
            ) override;


    //************************************************************************
    // variables
    //************************************************************************
    private:
        std::vector<double> mBeampatternMagDbVec;   //!< beampattern magnitudes [dB]

        bool                mShowSpeakers;          //!< true if showing speaker directions
        double              mFirstSpeakerAngleDeg;  //!< 1st speaker angle [deg]
        uint8_t             mNrOfSpeakers;          //!< number of speakers
};

#endif // BeampatternLayout_h
