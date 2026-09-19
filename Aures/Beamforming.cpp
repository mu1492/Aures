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
Beamforming.cpp

This file contains the sources for the acoustic beamforming.
*/

#include "Beamforming.h"

#include "AcousticsHandler.h"
#include "FrequencyAnalysis.h"

#include <cstring>


const std::map<Beamforming::BeampatternType, std::string> Beamforming::BEAMPATTERN_TYPE_NAMES =
{
    { Beamforming::BEAMPATTERN_TYPE_DAS,        "DAS"       },
    { Beamforming::BEAMPATTERN_TYPE_MVDR,       "MVDR"      },
    { Beamforming::BEAMPATTERN_TYPE_ADAPTIVE,   "Adaptive"  }
};

//!************************************************************************
//! Constructor
//!************************************************************************
Beamforming::Beamforming()
{   
}


//!************************************************************************
//! Calculate the adaptive (MVDR <> DAS) beampattern
//!
//! @returns the adaptive beampattern value
//!************************************************************************
cdouble Beamforming::calculateAdaptiveBeampattern
    (
    const double aFrequency,                //!< frequency [Hz]
    const double aDoa,                      //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
    const double aTheta,                    //!< angle [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray,                 //!< mic array
    Cx3Matrix    aRnn,                      //!< noise covariance matrix
    const double aCenterFrequency           //!< center frequency [Hz]
    )
{
    cdouble bp;

    if( aFrequency >= 0
     && aCenterFrequency > 0 )
    {
        CxVector w = calculateMixedWeights( aFrequency, aDoa, aMicArray, aRnn, aCenterFrequency );
        CxVector a = calculateSteeringVector( aFrequency, aTheta, aMicArray );

        for( size_t i = 0; i < w.size(); i++ )
        {
            bp += std::conj( w[i] ) * a[i];
        }
    }

    return bp;
}


//!************************************************************************
//! Calculate the DAS (Delay-And-Sum) beampattern
//!
//! @returns the DAS beampattern value
//!************************************************************************
cdouble Beamforming::calculateDasBeampattern
    (
    const double aFrequency,        //!< frequency [Hz]
    const double aDoa,              //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
    const double aTheta,            //!< angle [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray,         //!< mic array
    const bool   aRadialApodization //!< true if using radial apodization
    )
{
    cdouble bp;

    if( aFrequency >= 0 )
    {
        CxVector w;

        if( !aRadialApodization )
        {
            w = calculateDasWeights( aFrequency, aDoa, aMicArray );
        }
        else
        {
            w = calculateDasWeightsRadialApodization( aFrequency, aDoa, aMicArray );
        }

        CxVector a = calculateSteeringVector( aFrequency, aTheta, aMicArray );

        for( size_t i = 0; i < w.size(); i++ )
        {
            bp += std::conj( w[i] ) * a[i];
        }
    }

    return bp;
}


//!************************************************************************
//! Calculate the DAS (Delay-And-Sum) uniform weights
//! *NO* radial apodization is used.
//! The vector length equals the number of mics in the array.
//!
//! @returns the DAS weights vector
//!************************************************************************
CxVector Beamforming::calculateDasWeights
    (
    const double aFrequency,    //!< frequency [Hz]
    const double aDoa,          //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray      //!< mic array
    ) const
{
    CxVector w;

    if( aFrequency >= 0 )
    {
        CxVector a = calculateSteeringVector( aFrequency, aDoa, aMicArray );
        size_t M = a.size();

        if( M )
        {
            w.resize( M );

            for( size_t i = 0; i < M; i++ )
            {
                w[i] = a[i] / static_cast<double>( M );
            }
        }
    }

    return w;
}


//!************************************************************************
//! Calculate the DAS (Delay-And-Sum) weights with radial apodization
//! The vector length equals the number of mics in the array.
//!
//! @returns the DAS weights vector
//!************************************************************************
CxVector Beamforming::calculateDasWeightsRadialApodization
    (
    const double aFrequency,    //!< frequency [Hz]
    const double aDoa,          //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray      //!< mic array
    )
{
    CxVector w;
    const size_t MICS_COUNT = aMicArray.getArray().size();

    if( aFrequency >= 0
     && MICS_COUNT )
    {
        w.resize( MICS_COUNT );
        CxVector a = calculateSteeringVector( aFrequency, aDoa, aMicArray );

        // radial weights
        std::vector<double> beta( MICS_COUNT, 0 );
        size_t i = 0;
        double rMax = aMicArray.getMaxRadius();
        double p = calculateFrequencyExponent( aFrequency );

        if( rMax )
        {
            for( i = 0; i < MICS_COUNT; i++ )
            {
                beta[i] = pow( aMicArray.getRadius( i ) / rMax, p );
            }
        }

        // unnormalized weights
        CxVector w_tilde( MICS_COUNT, 0 );

        for( i = 0; i < MICS_COUNT; i++ )
        {
            w_tilde[i] = beta[i] * a[i];
        }

        // norming - unit response
        cdouble denom;

        for( i = 0; i < MICS_COUNT; i++ )
        {
            denom += conj( a[i] ) * w_tilde[i];
        }

        for( i = 0; i < MICS_COUNT; i++ )
        {
            w[i] = w_tilde[i] / denom;
        }
    }

    return w;
}


//!************************************************************************
//! Calculate the frequency exponent function p(f)
//!
//! @returns the value of the frequency exponent
//!************************************************************************
double Beamforming::calculateFrequencyExponent
    (
    const double aFrequency     //!< frequency [Hz]
    )
{
    double p = 0;
    AcousticsHandler* acousticsInstance = AcousticsHandler::getInstance();

    if( acousticsInstance )
    {
        double fMin = acousticsInstance->getFrequencyRange().min;
        double fMax = acousticsInstance->getFrequencyRange().max;
        WeightingFunctionExponent wfe = { 0, 2 };
        p = wfe.pMin + ( wfe.pMax - wfe.pMin ) * ( aFrequency - fMin ) / ( fMax - fMin );
    }

    return p;
}


//!************************************************************************
//! Calculate the mixed beamformer weights
//! The vector length equals the number of mics in the array.
//!
//! @returns the mixed weights vector
//!************************************************************************
CxVector Beamforming::calculateMixedWeights
    (
    const double aFrequency,        //!< frequency [Hz]
    const double aDoa,              //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray,         //!< mic array
    Cx3Matrix    aRnn,              //!< noise covariance matrix
    const double aCenterFrequency   //!< center frequency [Hz]
    )
{
    CxVector wMixed;

    if( aFrequency >= 0
     && aCenterFrequency > 0 )
    {
        CxVector wDas = calculateDasWeights( aFrequency, aDoa, aMicArray );
        Cx3Matrix rnnReg = regularizeCovariance( aRnn, aFrequency );
        CxVector wMvdr = calculateMvdrWeights( aFrequency, aDoa, aMicArray, rnnReg );
        wMixed = mixWeights( wDas, wMvdr, aFrequency, aCenterFrequency );
    }

    return wMixed;
}


//!************************************************************************
//! Calculate the MVDR (Minimum Variance Distortionless Response) beampattern
//!
//! @returns the MVDR beampattern value
//!************************************************************************
cdouble Beamforming::calculateMvdrBeampattern
    (
    const double aFrequency,        //!< frequency [Hz]
    const double aDoa,              //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
    const double aTheta,            //!< angle [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray,         //!< mic array
    Cx3Matrix    aRnn               //!< noise covariance matrix
    )
{
    cdouble bp;

    if( aFrequency >= 0 )
    {
        CxVector w = calculateMvdrWeights( aFrequency, aDoa, aMicArray, aRnn );
        CxVector a = calculateSteeringVector( aFrequency, aTheta, aMicArray );

        for( size_t i = 0; i < w.size(); i++ )
        {
            bp += std::conj( w[i] ) * a[i];
        }
    }

    return bp;
}


//!************************************************************************
//! Calculate the MVDR (Minimum Variance Distortionless Response) weights
//! The vector length equals the number of mics in the array.
//!
//! @returns the MVDR weights vector
//!************************************************************************
CxVector Beamforming::calculateMvdrWeights
    (
    const double aFrequency,    //!< frequency [Hz]
    const double aDoa,          //!< Direction of Arrival [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray,     //!< mic array
    Cx3Matrix    aRnn           //!< noise covariance matrix
    )
{
    CxVector w;
    CxVector a = calculateSteeringVector( aFrequency, aDoa, aMicArray );
    size_t M = a.size();
    FrequencyAnalysis* faInstance = FrequencyAnalysis::getInstance();

    if( aFrequency >= 0
     && M
     && faInstance )
    {
        w.resize( M );
        Numeric* numericInstance = Numeric::getInstance();

        if( numericInstance )
        {
            if( aRnn.size() )
            {
                double f = std::min( aFrequency, faInstance->getFftFreqMax() );
                double binWidth = faInstance->getFftBinWidth();
                const uint32_t BIN_MIN = 0;
                const uint32_t BIN_MAX = faInstance->getFftSizeValue() / 2;
                size_t crtBin = std::clamp( static_cast<uint32_t>( f / binWidth ), BIN_MIN, BIN_MAX );

                CxMatrix rnnAtCrtBin = extractBinContentFromRnn( aRnn, crtBin );
                CxMatrix rnnInv = numericInstance->invertMatrix( rnnAtCrtBin );

                for( size_t i = 0; i < M; i++ )
                {
                    for( size_t j = 0; j < M; j++ )
                    {
                        w[i] += rnnInv[i][j] * a[j];
                    }
                }

                cdouble denom;

                for( size_t i = 0; i < M; i++ )
                {
                    denom += std::conj( a[i] ) * w[i];
                }

                if( std::abs( denom ) )
                {
                    for( size_t i = 0; i < M; i++ )
                    {
                        w[i] /= denom;
                    }
                }
            }
        }
    }

    return w;
}


//!************************************************************************
//! Calculate the steering vector
//! The vector length equals the number of mics in the array.
//!
//! @returns the steering vector
//!************************************************************************
CxVector Beamforming::calculateSteeringVector
    (
    const double aFrequency,    //!< frequency [Hz]
    const double aTheta,        //!< angle [rad] relative to mic 3, trigonometric sense
    MicArray     aMicArray      //!< mic array
    ) const
{
    CxVector a;

    if( aFrequency >= 0 )
    {
        const size_t MICS_COUNT = aMicArray.getArray().size();
        a.resize( MICS_COUNT );
        double ux = cos( aTheta );
        double uy = sin( aTheta );

        for( size_t i = 0; i < MICS_COUNT; i++ )
        {
            if( aMicArray.isInUse( i ) )
            {
                double tau = ( aMicArray.getArray().at( i ).getXyzLocation().x * ux +
                               aMicArray.getArray().at( i ).getXyzLocation().y * uy )
                             / AcousticsHandler::C;

                a[i] = std::exp( cdouble( 0, -Numeric::TWO_PI * aFrequency * tau ) );
            }
        }
    }

    return a;
}


//!************************************************************************
//! Convert a complex beampattern value to magnitude [dB]
//!
//! @returns the magnitude [dB]
//!************************************************************************
double Beamforming::convertBeampatternCxToMagnitudeDb
    (
    cdouble aBeampatternCxValue             //!< complex beampattern value
    ) const
{
    double bpMag = std::abs( aBeampatternCxValue );
    const double Z_EPS = 1.e-15;
    double bpMagDb = 20.0 * log10( std::max( bpMag, Z_EPS ) );

    return bpMagDb;
}


//!************************************************************************
//! Extract the content for a single bin from a noise covariance matrix
//!
//! @returns the extracted Rnn content for a bin
//!************************************************************************
CxMatrix Beamforming::extractBinContentFromRnn
    (
    Cx3Matrix    aRnn,                      //!< noise covariance matrix
    const size_t aBin                       //!< frequency bin for which the content is extracted
    ) const
{
    CxMatrix rnnBin;
    const size_t M = aRnn.size();
    const size_t N = aRnn.at( 0 ).size();
    const size_t FFT_SIZE = aRnn.at( 0 ).at( 0 ).size();

    if( M == N
     && aBin < FFT_SIZE )
    {
        rnnBin.resize( M, CxVector( N ) );

        for( size_t i = 0; i < M; i++ )
        {
            for( size_t j = 0; j < N; j++ )
            {
                rnnBin[i][j] = aRnn[i][j][aBin];
            }
        }
    }

    return rnnBin;
}


//!************************************************************************
//! Mix the DAS and MVDR beamformer weights considering a center frequency
//! The lengths of the two weight vectors must match.
//!
//! @returns the mixed weights vector
//!************************************************************************
CxVector Beamforming::mixWeights
    (
    CxVector        aDasWeights,       //!< DAS weights
    CxVector        aMvdrWeights,      //!< MVDR weights
    const double    aFrequency,        //!< frequency [Hz]
    const double    aCenterFrequency   //!< center frequency [Hz]
    ) const
{
    CxVector w;

    if( aFrequency >= 0
     && aCenterFrequency > 0
     && aDasWeights.size() == aMvdrWeights.size() )
    {
        const double ETA = std::min( 1.0, aFrequency / aCenterFrequency );
        size_t M = aMvdrWeights.size();

        if( M )
        {
            w.resize( M );

            for( size_t i = 0; i < M; i++ )
            {
                w[i] = ETA * aMvdrWeights[i] + ( 1 - ETA ) * aDasWeights[i];
            }
        }
    }

    return w;
}


//!************************************************************************
//! Calculate the regularized covariance matrix Rnn for a frequency
//!
//! @returns the matrix
//!************************************************************************
Cx3Matrix Beamforming::regularizeCovariance
    (
    const Cx3Matrix&    aMatrix,        //!< noise covariance matrix
    const double        aFrequency,     //!< frequency [Hz]
    const double        aBeta           //!< beta
    ) const
{
    Cx3Matrix regRnn = aMatrix;
    size_t M = aMatrix.size();
    FrequencyAnalysis* faInstance = FrequencyAnalysis::getInstance();

    if( aFrequency >= 0
     && M
     && faInstance )
    {
        double f = std::min( aFrequency, faInstance->getFftFreqMax() );
        double binWidth = faInstance->getFftBinWidth();
        const uint32_t BIN_MIN = 0;
        const uint32_t BIN_MAX = faInstance->getFftSizeValue() / 2;
        size_t crtBin = std::clamp( static_cast<uint32_t>( f / binWidth ), BIN_MIN, BIN_MAX );
        cdouble trace;

        for( size_t i = 0; i < M; i++ )
        {
            trace += aMatrix[i][i][crtBin];
        }

        double eps = aBeta * trace.real() / M;

        for( size_t i = 0; i < M; i++ )
        {
            regRnn[i][i][crtBin] += eps;
        }
    }

    return regRnn;
}
