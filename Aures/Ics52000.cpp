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
Ics52000.cpp

This file contains the sources for the TDK InvenSense ICS-52000 microphone.
*/

#include "Ics52000.h"


//!************************************************************************
//! Constructor
//!************************************************************************
Ics52000::Ics52000()
    : Mic
        (
        { 50, 20000 },  // frequency range [Hz]
        0.003,          // THD [-]
        -26.0,          // sensitivity [dB FS]
        65.0,           // SNR [dBA]
        117.0           // AOP [dB SPL]
        )
{
}
