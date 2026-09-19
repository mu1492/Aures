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
Beamforming.h

This file contains the definitions for the acoustic beamforming.
*/

#ifndef Beamforming_h
#define Beamforming_h

#include "MicArray.h"
#include "Numeric.h"

#include <cstdint>
#include <map>


//************************************************************************
// Class for handling the acoustic beamforming
//************************************************************************
class Beamforming
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        typedef enum : uint8_t
        {
            BEAMPATTERN_TYPE_DAS,       //!< Delay-And-Sum
            BEAMPATTERN_TYPE_MVDR,      //!< Minimum Variance Distortionless Response
            BEAMPATTERN_TYPE_ADAPTIVE   //!< adaptive DAS-MVDR
        }BeampatternType;

        static const std::map<BeampatternType, std::string> BEAMPATTERN_TYPE_NAMES;

        typedef struct
        {
            BeampatternType type;              //!< type
            int             doaDeg;            //!< DoA [deg] relative to mic 5, trigonometric sense
            double          frequency;         //!< f [Hz]
            double          frequencyCenter;   //!< fC [Hz] - adaptive only
            bool            radialApodization; //!< DAS only
        }BeampatternConfig;

        static constexpr double DEFAULT_CENTER_FREQUENCY = 1500.0;  //!< [Hz]

    private:
        typedef struct
        {
            double pMin;
            double pMax;
        }WeightingFunctionExponent;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        Beamforming();

        cdouble calculateAdaptiveBeampattern
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
            const double aTheta,                    //!< angle [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray,                 //!< mic array
            Cx3Matrix    aRnn,                      //!< noise covariance matrix
            const double aCenterFrequency = DEFAULT_CENTER_FREQUENCY    //!< center frequency [Hz]
            );

        cdouble calculateDasBeampattern
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
            const double aTheta,                    //!< angle [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray,                 //!< mic array
            const bool   aRadialApodization = false //!< true if using radial apodization
            );

        CxVector calculateDasWeights
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray                  //!< mic array
            ) const;

        CxVector calculateDasWeightsRadialApodization
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray                  //!< mic array
            );

        CxVector calculateMixedWeights
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray,                 //!< mic array
            Cx3Matrix    aRnn,                      //!< noise covariance matrix
            const double aCenterFrequency = DEFAULT_CENTER_FREQUENCY    //!< center frequency [Hz]
            );

        cdouble calculateMvdrBeampattern
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
            const double aTheta,                    //!< angle [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray,                 //!< mic array
            Cx3Matrix    aRnn                       //!< noise covariance matrix
            );

        CxVector calculateMvdrWeights
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray,                 //!< mic array
            Cx3Matrix    aRnn                       //!< noise covariance matrix
            );

        CxVector calculateSteeringVector
            (
            const double aFrequency,                //!< frequency [Hz]
            const double aTheta,                    //!< angle [rad] relative to mic 3, trigonometric sense
            MicArray     aMicArray                  //!< mic array
            ) const;

        double convertBeampatternCxToMagnitudeDb
            (
            cdouble aBeampatternCxValue             //!< complex beampattern value
            ) const;

    private:
        double calculateFrequencyExponent
            (
            const double aFrequency                 //!< frequency [Hz]
            );

        CxMatrix extractBinContentFromRnn
            (
            Cx3Matrix    aRnn,                      //!< noise covariance matrix
            const size_t aBin                       //!< frequency bin for which the content is extracted
            ) const;

        CxVector mixWeights
            (
            CxVector        aDasWeights,       //!< DAS weights
            CxVector        aMvdrWeights,      //!< MVDR weights
            const double    aFrequency,        //!< frequency [Hz]
            const double    aCenterFrequency = DEFAULT_CENTER_FREQUENCY    //!< center frequency [Hz]
            ) const;

        Cx3Matrix regularizeCovariance
            (
            const Cx3Matrix&    aMatrix,            //!< noise covariance matrix
            const double        aFrequency,         //!< frequency [Hz]
            const double        aBeta = 1.e-3       //!< beta
            ) const;       
};

#endif // Beamforming_h
