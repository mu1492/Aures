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
Ics52000.h

This file contains the definitions for the TDK InvenSense ICS-52000 microphone.
*/

#ifndef Ics52000_h
#define Ics52000_h

#include "Mic.h"


//************************************************************************
// Class for handling an ICS-5200 microphone
//************************************************************************
class Ics52000 : public Mic
{
    //************************************************************************
    // functions
    //************************************************************************
    public:
        Ics52000();
};

#endif // Ics52000_h
