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
SpeakerRingLayout.h

This file contains the definitions for the speaker ring layout.
*/

#ifndef SpeakerRingLayout_h
#define SpeakerRingLayout_h

#include "MicArray.h"

#include <QDial>


//************************************************************************
// Class for handling the speaker ring layout
//************************************************************************
class SpeakerRingLayout : public QDial
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        explicit SpeakerRingLayout
            (
            QWidget*    aParent = nullptr    //!< parent widget
            );

        void updateMicArray
            (
            MicArray    aMicArray   //!< mic array
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
        MicArray    mMicArray;          //!< microphone array
};

#endif // SpeakerRingLayout_h
