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
AcousticsHandler.h

This file contains the definitions for the acoustics handler.
*/

#ifndef AcousticsHandler_h
#define AcousticsHandler_h

#include "MicArray.h"

#include <cmath>
#include <map>


//************************************************************************
// Class for handling acoustics
//************************************************************************
class AcousticsHandler
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        static constexpr double C = 343.2;      //!< m/s @ 20 C

        static constexpr double REF_SPL = 20.0 * log10( 1.0 / 20.e-6 );     //!< ~94 dB

        typedef struct
        {
            double min;     // [Hz]
            double max;     // [Hz]
        }FrequencyRange;

        typedef enum : uint8_t
        {
            // narrow bands around a center frequency
            // -> these do not necessarily match third octave bands as per ISO 266
            // approx 1/3 octave => f_low = 0.890899 fc, f_high = 1.122462 fc
            // <=> f_low = fc / pow(2, 1/6), f_high = fc * pow(2, 1/6)
            // <=> f_high / f_low = pow(2, 1/3)
            FREQUENCY_RANGE_CENTERED_500,                   // fc =  500 [Hz]
            FREQUENCY_RANGE_CENTERED_750,                   // fc =  750 [Hz]
            FREQUENCY_RANGE_CENTERED_1000,                  // fc = 1000 [Hz]
            FREQUENCY_RANGE_CENTERED_1500,                  // fc = 1500 [Hz]
            FREQUENCY_RANGE_CENTERED_2000,                  // fc = 2000 [Hz]
            FREQUENCY_RANGE_CENTERED_2500,                  // fc = 2500 [Hz]
            FREQUENCY_RANGE_CENTERED_3000,                  // fc = 3000 [Hz]
            FREQUENCY_RANGE_CENTERED_4000,                  // fc = 4000 [Hz]
            // speech subbands
            FREQUENCY_RANGE_SPEECH_SUBBAND_300_700,         //  300 -  700 [Hz]
            FREQUENCY_RANGE_SPEECH_SUBBAND_700_1200,        //  700 - 1200 [Hz]
            FREQUENCY_RANGE_SPEECH_SUBBAND_1200_2000,       // 1200 - 2000 [Hz]
            FREQUENCY_RANGE_SPEECH_SUBBAND_2000_3400,       // 2000 - 3400 [Hz]
            // speech bands
            FREQUENCY_RANGE_SPEECH_BAND_INTELLIGIBILITY,    // 1000 -  2000 [Hz]
            FREQUENCY_RANGE_SPEECH_BAND_NOMINAL,            //  300 -  3400 [Hz]
            // audio band
            FREQUENCY_RANGE_AUDIO,                          //   20 -  8000 [Hz]

            // keep this last
            FREQUENCY_RANGE_LAST
        }FrequencyRangeOption;

        static const std::map<FrequencyRangeOption, FrequencyRange> FREQUENCY_RANGE_VALUES;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        AcousticsHandler();

        ~AcousticsHandler();

        static AcousticsHandler* getInstance();

        void destroyInstance();

        double computeAngularResolution
            (
            const MicArray::Uca     aUca,       //!< UCA
            const double            aFrequency  //!< frequency [Hz]
            ) const;

        double computeDirectivityFactor
            (
            MicArray     aMicArray,             //!< mic array
            const double aDoa,                  //!< Direction of Arrival [rad]
            const double aFrequency             //!< frequency [Hz]
            ) const;

        double computeDirectivityIndex
            (
            MicArray     aMicArray,             //!< mic array
            const double aDoa,                  //!< Direction of Arrival [rad]
            const double aFrequency             //!< frequency [Hz]
            ) const;

        double computeDoaPrecisionVariance
            (
            MicArray                aMicArray,      //!< mic array
            const Mic::CcaLocation  aCcaLocation,   //!< CCA location
            const double            aFrequency      //!< frequency [Hz]
            ) const;

        double computeDoaResolutionStdDev
            (
            MicArray                aMicArray,      //!< mic array
            const Mic::CcaLocation  aCcaLocation,   //!< CCA location
            const double            aFrequency,     //!< frequency [Hz]
            const bool              aUseIQ = false  //!< if using both IQ components (complex)
            ) const;

        double computeRayleighDistance
            (
            const MicArray::Uca     aUca,       //!< UCA
            const double            aFrequency  //!< frequency [Hz]
            ) const;

        double getAdjacentMicsDistance
            (
            const MicArray::Uca     aUca        //!< UCA
            ) const;

        double getDirectivityIndexMax
            (
            const MicArray::Uca     aUca        //!< UCA
            ) const;

        double getEfficientApertureFreqMin
            (
            const MicArray::Uca     aUca        //!< UCA
            ) const;

        FrequencyRange getFrequencyRange() const;

        FrequencyRangeOption getFrequencyRangeOptionIndex() const;

        double getHpBw
            (
            MicArray                aMicArray,      //!< mic array
            const double            aFrequency      //!< frequency [Hz]
            ) const;

        double getHpBwBeampattern
            (
            MicArray                aMicArray,      //!< mic array
            const double            aDoa,           //!< Direction of Arrival [rad]
            const double            aFrequency      //!< frequency [Hz]
            ) const;

        double getSpatialAliasingFreqMax
            (
            const MicArray::Uca     aUca        //!< UCA
            ) const;

        double integrateBeampattern
            (
            MicArray     aMicArray,                     //!< mic array
            const double aDoa,                          //!< Direction of Arrival [rad]
            const double aFrequency,                    //!< frequency [Hz]
            const double aStartAngle = 0,               //!< start angle
            const double aStopAngle = Numeric::TWO_PI,  //!< stop angle
            const int    aN = 10000                     //!< nr of subintervals
            ) const;

        bool setFrequencyRangeOptionIndex
            (
            const FrequencyRangeOption aFrequencyRangeIndex //!< frequency range index
            );

        double weightingAfunc
            (
            const double            aFrequency   //!< frequency [Hz]
            ) const;

        double weightingAdb
            (
            const double            aFrequency   //!< frequency [Hz]
            ) const;


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static AcousticsHandler*    sInstance;              //!< singleton

        FrequencyRangeOption        mFrequencyRangeIndex;   //!< frequency range index
        FrequencyRange              mFrequencyRange;        //!< fMin-fMax [Hz]
};

#endif // AcousticsHandler_h
