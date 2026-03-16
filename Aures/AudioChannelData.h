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
AudioChannelData.h

This file contains the definitions for the audio channel data.
*/

#ifndef AudioChannelData_h
#define AudioChannelData_h

#include <QList>
#include <QMetaType>

typedef struct
{
    int             channel;    //!< mic number as per EVAL-MICCANVASZ [1-15]
    QList<double>   data;       //!< audio data
}AudioChannelData;

typedef struct
{
    int      channel;       //!< mic number as per EVAL-MICCANVASZ [1-15]
    double   value;         //!< value
}ChannelValueDouble;


Q_DECLARE_METATYPE( AudioChannelData )
Q_DECLARE_METATYPE( ChannelValueDouble )

#endif // AudioChannelData_h
