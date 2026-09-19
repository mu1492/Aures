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
AcousticsHandler.cpp

This file contains the sources for acoustics.
*/

#include "AcousticsHandler.h"

#include "Beamforming.h"
#include "Numeric.h"

#include <cstdint>


AcousticsHandler* AcousticsHandler::sInstance = nullptr;

const std::map<AcousticsHandler::FrequencyRangeOption, AcousticsHandler::FrequencyRange> AcousticsHandler::FREQUENCY_RANGE_VALUES =
{
    // 1/3 octave bands around a center frequency
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_500,               {  445.4,  561.2 } },
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_750,               {  668.2,  841.8 } },
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_1000,              {  890.9, 1122.5 } },
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_1500,              { 1336.3, 1683.7 } },
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_2000,              { 1781.8, 2244.9 } },
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_2500,              { 2227.2, 2806.2 } },
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_3000,              { 2672.7, 3367.4 } },
    { AcousticsHandler::FREQUENCY_RANGE_CENTERED_4000,              { 3563.6, 4489.8 } },
    // speech subbands
    { AcousticsHandler::FREQUENCY_RANGE_SPEECH_SUBBAND_300_700,     {  300,    700  } },
    { AcousticsHandler::FREQUENCY_RANGE_SPEECH_SUBBAND_700_1200,    {  700,   1200  } },
    { AcousticsHandler::FREQUENCY_RANGE_SPEECH_SUBBAND_1200_2000,   { 1200,   2000  } },
    { AcousticsHandler::FREQUENCY_RANGE_SPEECH_SUBBAND_2000_3400,   { 2000,   3400  } },
    // speech bands
    { AcousticsHandler::FREQUENCY_RANGE_SPEECH_BAND_INTELLIGIBILITY,{ 1000,   2000  } },
    { AcousticsHandler::FREQUENCY_RANGE_SPEECH_BAND_NOMINAL,        {  300,   3400  } },
    // audio band
    { AcousticsHandler::FREQUENCY_RANGE_AUDIO,                      {   20,   8000  } }
};

//!************************************************************************
//! Constructor
//!************************************************************************
AcousticsHandler::AcousticsHandler()
    : mFrequencyRangeIndex( FREQUENCY_RANGE_SPEECH_BAND_NOMINAL )
    , mFrequencyRange( FREQUENCY_RANGE_VALUES.at( mFrequencyRangeIndex ) )
{
}


//!************************************************************************
//! Destructor
//!************************************************************************
AcousticsHandler::~AcousticsHandler()
{
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
AcousticsHandler* AcousticsHandler::getInstance()
{
    if( !sInstance )
    {
        sInstance = new AcousticsHandler;
    }

    return sInstance;
}


//!************************************************************************
//! Instance destroyer
//!
//! @returns nothing
//!************************************************************************
void AcousticsHandler::destroyInstance()
{
    delete sInstance;
    sInstance = nullptr;
}


//!************************************************************************
//! Calculate the angular resolution (beamwidth) for a UCA
//!
//! @returns the beamwidth [rad]
//!************************************************************************
double AcousticsHandler::computeAngularResolution
    (
    const MicArray::Uca     aUca,       //!< UCA
    const double            aFrequency  //!< frequency [Hz]
    ) const
{
    double bw = 0;

    if( aUca.radius > 0 && aFrequency > 0 )
    {
        bw = C / ( 2.0 * aFrequency * aUca.radius );
    }

    return bw;
}


//!************************************************************************
//! Calculate the Directivity Factor (DF) of a mic array
//! It uses the DAS beampattern *only* (no MVDR).
//!
//! @returns the directivity factor [rad]
//!************************************************************************
double AcousticsHandler::computeDirectivityFactor
    (
    MicArray     aMicArray,             //!< mic array
    const double aDoa,                  //!< Direction of Arrival [rad]
    const double aFrequency             //!< frequency [Hz]
    ) const
{
    double df = 0;
    Beamforming bf;
    cdouble bp = bf.calculateDasBeampattern( aFrequency, aDoa, aDoa, aMicArray );
    double num = Numeric::TWO_PI * pow( std::abs( bp ), 2.0 );
    double den = 0;
    den = integrateBeampattern( aMicArray, aDoa, aFrequency );

    if( den != 0 )
    {
        df = num / den;
    }

    return df;
}


//!************************************************************************
//! Calculate the Directivity Index (DI) of a mic array
//! It uses the DAS beampattern *only* (no MVDR).
//!
//! @returns the directivity index [dB]
//!************************************************************************
double AcousticsHandler::computeDirectivityIndex
    (
    MicArray     aMicArray,             //!< mic array
    const double aDoa,                  //!< Direction of Arrival [rad]
    const double aFrequency             //!< frequency [Hz]
    ) const
{
    double diDb = 0;
    double df = computeDirectivityFactor( aMicArray, aDoa, aFrequency );

    if( df > 0 )
    {
        diDb = 10.0 * log10( df );
    }

    return diDb;
}


//!************************************************************************
//! Calculate the estimated precision (variance) for DOA in terms of CRLB
//! (Cramer-Rao Lower Bound).
//! It refers to the estimation error of a single DOA angle, and is not
//! involving multiple sound sources (it does not relate to the separation
//! of two adjacent sources).
//!
//! It applies to azimuth only (the UCA plane).
//!
//! @returns the estimated precision variance [rad^2]
//!************************************************************************
double AcousticsHandler::computeDoaPrecisionVariance
    (
    MicArray                aMicArray,      //!< mic array
    const Mic::CcaLocation  aCcaLocation,   //!< CCA location
    const double            aFrequency      //!< frequency [Hz]
    ) const
{
    double crlb = 0;

    if( ( Mic::CCA_LOCATION_OUTER_CIRCLE == aCcaLocation || Mic::CCA_LOCATION_INNER_CIRCLE == aCcaLocation )
     && ( aFrequency > 0 )
        )
    {
        double k = Numeric::TWO_PI * aFrequency / C;
        uint8_t nrMics = 0;
        double snr = 0;
        double r = 0;

        for( size_t i = 0; i < aMicArray.getArray().size(); i++ )
        {
            if( aCcaLocation == aMicArray.getArray().at( i ).getCcaLocation() )
            {
                nrMics++;
                snr += aMicArray.getArray().at( i ).getSnr();

                if( 0 == r )
                {
                    r = sqrt( pow( aMicArray.getArray().at( i ).getXyzLocation().x, 2.0 )
                            + pow( aMicArray.getArray().at( i ).getXyzLocation().y, 2.0 ) );
                }
            }
        }

        if( nrMics )
        {
            snr /= nrMics;
            snr = pow( 10.0, snr / 10.0 );

            crlb = ( snr * nrMics ) * pow( k * r, 2.0 );
            crlb = 1.0 / crlb;
        }
    }

    return crlb;
}


//!************************************************************************
//! Calculate the estimated resolution angle (standard deviation) for DOA
//! in terms of CRLB (Cramer-Rao Lower Bound).
//! It may help estimating the angular resolution when dealing with
//! adjacent sound sources.
//!
//! It applies to azimuth only (the UCA plane).
//!
//! @returns the estimated resolution angle [rad]
//!************************************************************************
double AcousticsHandler::computeDoaResolutionStdDev
    (
    MicArray                aMicArray,      //!< mic array
    const Mic::CcaLocation  aCcaLocation,   //!< CCA location
    const double            aFrequency,     //!< frequency [Hz]
    const bool              aUseIQ          //!< if using both IQ components (complex)
    ) const
{
    double sigmaTheta = computeDoaPrecisionVariance( aMicArray, aCcaLocation, aFrequency );

    if( aUseIQ )
    {
        sigmaTheta /= 2.0;
    }

    sigmaTheta = sqrt( sigmaTheta );
    return sigmaTheta;
}


//!************************************************************************
//! Calculate the Rayleigh distance (near-field) for a UCA
//!
//! @returns the Rayleigh distance [m]
//!************************************************************************
double AcousticsHandler::computeRayleighDistance
    (
    const MicArray::Uca     aUca,       //!< UCA
    const double            aFrequency  //!< frequency [Hz]
    ) const
{
    double d = 0;

    if( aUca.radius > 0 && aFrequency > 0 )
    {
        d = ( 8.0 * aFrequency * aUca.radius * aUca.radius ) / C;
    }

    return d;
}


//!************************************************************************
//! Calculate the distance between two adjacent microphones on a UCA
//!
//! @returns the distance [m]
//!************************************************************************
double AcousticsHandler::getAdjacentMicsDistance
    (
    const MicArray::Uca     aUca        //!< UCA
    ) const
{
    double d = 0;

    if( aUca.micsCount && aUca.radius > 0 )
    {
        d = 2.0 * aUca.radius * sin( Numeric::PI / aUca.micsCount );
    }

    return d;
}


//!************************************************************************
//! Calculate the max. directivity index (DI) of a UCA
//!
//! @returns the max. directivity index [dB]
//!************************************************************************
double AcousticsHandler::getDirectivityIndexMax
    (
    const MicArray::Uca     aUca        //!< UCA
    ) const
{
    double di = 0;

    if( aUca.micsCount )
    {
        di = 10.0 * log10( aUca.micsCount );
    }

    return di;
}


//!************************************************************************
//! Calculate the minimum frequency for efficient aperture of a UCA
//!
//! @returns the min. frequency [Hz]
//!************************************************************************
double AcousticsHandler::getEfficientApertureFreqMin
    (
    const MicArray::Uca     aUca        //!< UCA
    ) const
{
    double fMin = 0;

    if( aUca.radius > 0 )
    {
        fMin = C / ( 4.0 * aUca.radius );
    }

    return fMin;
}


//!************************************************************************
//! Get the values of designated frequency range
//!
//! @returns the frequency range [Hz]
//!************************************************************************
AcousticsHandler::FrequencyRange AcousticsHandler::getFrequencyRange() const
{
    return mFrequencyRange;
}


//!************************************************************************
//! Get the index of the frequency range option
//!
//! @returns the frequency range index
//!************************************************************************
AcousticsHandler::FrequencyRangeOption AcousticsHandler::getFrequencyRangeOptionIndex() const
{
    return mFrequencyRangeIndex;
}


//!************************************************************************
//! Get the HPBW (Half-Power Beamwidth)
//! This function calculates an estimation based on a maximum (exterior)
//! known radius belonging to a CCA, relying on uniform geometries.
//! It does NOT use the beampattern or a specific DoA.
//!
//! @returns the HPBW [rad]
//!************************************************************************
double AcousticsHandler::getHpBw
    (
    MicArray                aMicArray,      //!< mic array
    const double            aFrequency      //!< frequency [Hz]
    ) const
{
    double hpbw = 0;

    if( aFrequency > 0 )
    {
        double rExt = aMicArray.getMaxRadius();

        if( rExt > 0 )
        {
            const double X_HPBW = 1.39155737825151;         // solution of fabs( sin(x)/x ) = 1/sqrt(2)
            const double K = 2.0 * X_HPBW / Numeric::PI;    // approx. 0.886
            hpbw = 0.5 * K * C / ( aFrequency * rExt );
        }
    }

    return hpbw;
}


//!************************************************************************
//! Get the HPBW (Half-Power Beamwidth)
//! It uses the DAS beampattern *only* (no MVDR).
//!
//! @returns the HPBW [rad]
//!************************************************************************
double AcousticsHandler::getHpBwBeampattern
    (
    MicArray                aMicArray,      //!< mic array
    const double            aDoa,           //!< Direction of Arrival [rad]
    const double            aFrequency      //!< frequency [Hz]
    ) const
{
    double hpbwbp = 0;
    Beamforming bf;
    cdouble bp = bf.calculateDasBeampattern( aFrequency, aDoa, aDoa, aMicArray );
    hpbwbp = std::abs( bp ) / sqrt( 2.0 );

    return hpbwbp;
}


//!************************************************************************
//! Calculate the maximum frequency for spatial aliasing on a UCA
//!
//! @returns the max. frequency [Hz]
//!************************************************************************
double AcousticsHandler::getSpatialAliasingFreqMax
    (
    const MicArray::Uca     aUca        //!< UCA
    ) const
{
    double fMax = 0;

    if( aUca.micsCount && aUca.radius > 0 )
    {
        fMax = C / ( 2.0 * getAdjacentMicsDistance( aUca ) );
    }

    return fMax;
}


//!************************************************************************
//! Calculate the integral     b
//!                            /
//!                         I= | |B(t)|^2 dt
//!                            /
//!                            a
//!
//! This function is mainly called from computeDirectivityFactor(), which
//! uses the DAS beampattern *only* (no MVDR).
//!
//! @returns the value of the integral
//!************************************************************************
double AcousticsHandler::integrateBeampattern
    (
    MicArray     aMicArray,     //!< mic array
    const double aDoa,          //!< Direction of Arrival [rad]
    const double aFrequency,    //!< frequency [Hz]
    const double aStartAngle,   //!< start angle
    const double aStopAngle,    //!< stop angle
    const int    aN             //!< nr of subintervals
    ) const
{
    double sum = 0;
    double step = 0;

    if( aStopAngle > aStartAngle
     && aN >= 1 )
    {
        Beamforming bf;

        step = ( aStopAngle - aStartAngle ) / aN;

        sum = pow( std::abs( bf.calculateDasBeampattern( aFrequency, aDoa, aStartAngle, aMicArray ) ), 2.0 ) +
            + pow( std::abs( bf.calculateDasBeampattern( aFrequency, aDoa, aStopAngle, aMicArray ) ), 2.0 );
        sum *= 0.5;

        for( int i = 1; i < aN; i++ )
        {
            sum += pow( std::abs( bf.calculateDasBeampattern( aFrequency, aDoa, aStartAngle + i * step, aMicArray ) ), 2.0 );
        }
    }

    return sum * step;
}


//!************************************************************************
//! Set a new index in the frequency range options
//!
//! @returns true if the value could be set
//!************************************************************************
bool AcousticsHandler::setFrequencyRangeOptionIndex
    (
    const FrequencyRangeOption aFrequencyRangeIndex //!< frequency range index
    )
{
    bool status = aFrequencyRangeIndex < FREQUENCY_RANGE_LAST;

    if( status && ( aFrequencyRangeIndex != mFrequencyRangeIndex ) )
    {
        mFrequencyRangeIndex = aFrequencyRangeIndex;
        mFrequencyRange = FREQUENCY_RANGE_VALUES.at( mFrequencyRangeIndex );
    }

    return status;
}


//!************************************************************************
//! Calculate the weighting A(f) dimensionless function for human hearing.
//! It should be applied to the amplitude spectrum of sound.
//!
//! @returns the value of the function [-]
//!************************************************************************
double AcousticsHandler::weightingAfunc
    (
    const double            aFrequency   //!< frequency [Hz]
    ) const
{
    const double F2 = aFrequency * aFrequency;
    const double F4 = F2 * F2;
    const double M = 12194 * 12194;
    const double N1 = 20.6 * 20.6;
    const double N2 = 107.7 * 107.7;
    const double N3 = 737.9 * 737.9;
    double r = M * F4;
    r /= ( ( F2 + N1 ) * ( F2 + M ) * sqrt( ( F2 + N2 ) * ( F2 + N3 ) ) );

    return r;
}


//!************************************************************************
//! Calculate the weighting A(f) [dB] for human hearing.
//! It should be applied to the amplitude spectrum of sound.
//!
//! @returns the value of the weighting function [dB]
//!************************************************************************
double AcousticsHandler::weightingAdb
    (
    const double            aFrequency   //!< frequency [Hz]
    ) const
{
    double aDb = 20.0 * ( log10( weightingAfunc( aFrequency ) ) - log10( weightingAfunc( 1000.0 ) ) );
    return aDb;
}
