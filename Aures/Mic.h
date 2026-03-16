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
Mic.h

This file contains the definitions for a microphone.
*/

#ifndef Mic_h
#define Mic_h

#include <cmath>


//************************************************************************
// Class for handling a microphone
//************************************************************************
class Mic
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        typedef struct
        {
            double x;       //!< x coordinate [m]
            double y;       //!< y coordinate [m]
            double z;       //!< z coordinate [m]
        }Location;

        typedef struct
        {
            double fmin;    //!< fmin [Hz]
            double fmax;    //!< fmax [Hz]
        }FreqRange3db;

    private:
        static constexpr double REF_SPL = 20.0 * log10( 1.0 / 20.e-6 );     //!< 94 dB


    //************************************************************************
    // functions
    //************************************************************************
    public:
        Mic
            (
            FreqRange3db    aFrequencyRange,    //!< frequency range @3dB [Hz]
            double          aThd,               //!< THD [-]
            double          aSensitivity,       //!< sensitivity [dB FS]
            double          aSnr,               //!< SNR [dBA]
            double          aAop                //!< AOP [dB SPL]
            );

        double getAop() const;

        double getDynRng() const;

        double getEin() const;

        Location getLocation() const;

        double getNf() const;

        double getSensitivity() const;

        double getSnr() const;

        void setLocation
            (
            const Location  aLocation   //!< xyz location
            );


    //************************************************************************
    // variables
    //************************************************************************
    protected:
        Location        mLocation;          //!< xyz location

        FreqRange3db    mFrequencyRange;    //!< fmin-fmax [Hz]
        double          mThd;               //!< THD [0..1] [-]

        double          mSensitivity;       //!< sensitivity [dB FS]
        double          mSnr;               //!< SNR [dBA]
        double          mAop;               //!< AOP [dB SPL]

        double          mEin;               //!< EIN [dB SPL]
        double          mNf;                //!< NF [dB FS]
        double          mDynRng;            //!< dynamic range [dB SPL]
};

#endif // Mic_h
