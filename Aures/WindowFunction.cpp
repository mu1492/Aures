///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023, 2026 Mihai Ursu                                           //
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
WindowFunction.cpp

This file contains the sources for the FFT window functions.
*/

#include "WindowFunction.h"

#include "Numeric.h"


//!************************************************************************
//! Constructor
//!************************************************************************
WindowFunction::WindowFunction()
    : mFftSize( 1 )
    , mActiveType( WINDOW_FUNCTION_TYPE_HANN )
{
    mParametersMap =
    {
        /////////////////////////////////////////////////////////////////////
        /// Boundaries in the following list can be modified as convenient.
        /// Table I (pp. 55) and Fig 12 (pp. 58) from [3] may provide more insight.
        /////////////////////////////////////////////////////////////////////

        { WINDOW_FUNCTION_TYPE_BLACKMAN,             { false,  false, 0, 0, 0, "N/A" } },
        { WINDOW_FUNCTION_TYPE_GAUSSIAN,             { false,  true,  2.0, 2.5, 5.0, ALPHA_SMALL } },
        { WINDOW_FUNCTION_TYPE_HAMMING,              { true,   false, 0, 0, 0, "N/A" } },
        { WINDOW_FUNCTION_TYPE_HANN,                 { true,   false, 0, 0, 0, "N/A" } },
        { WINDOW_FUNCTION_TYPE_KAISER_BESSEL,        { false,  true,  2.0, 3.0, 3.5, ALPHA_SMALL } },
        { WINDOW_FUNCTION_TYPE_NUTTALL,              { false,  false, 0, 0, 0, "N/A" } },
        { WINDOW_FUNCTION_TYPE_RECTANGULAR,          { true,   false, 0, 0, 0, "N/A" } }
    };
}


//!************************************************************************
//! Destructor
//!************************************************************************
WindowFunction::~WindowFunction()
{
}


//!************************************************************************
//! Get the active type
//!
//! @returns The active type
//!************************************************************************
WindowFunction::WindowFunctionType WindowFunction::getActiveType()
{
    return mActiveType;
}


//!************************************************************************
//! Get the map with parameters for each type
//!
//! @returns The parameters map
//!************************************************************************
std::map<WindowFunction::WindowFunctionType, WindowFunction::ParameterData> WindowFunction::getParametersMap() const
{
    return mParametersMap;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValue
    (
    const uint32_t aIndex,      //!< index
    double*        aValue       //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        *aValue = 0;

        switch( mActiveType )
        {
            case WINDOW_FUNCTION_TYPE_BLACKMAN:
                status = getWindowValueBlackman( aIndex, aValue );
                break;

            case WINDOW_FUNCTION_TYPE_GAUSSIAN:
                status = getWindowValueGaussian( aIndex, aValue );
                break;

            case WINDOW_FUNCTION_TYPE_HAMMING:
                status = getWindowValueHamming( aIndex, aValue );
                break;

            case WINDOW_FUNCTION_TYPE_HANN:
                status = getWindowValueHann( aIndex, aValue );
                break;

            case WINDOW_FUNCTION_TYPE_KAISER_BESSEL:
                status = getWindowValueKaiserBessel( aIndex, aValue );
                break;

            case WINDOW_FUNCTION_TYPE_NUTTALL:
                status = getWindowValueNuttall( aIndex, aValue );
                break;

            case WINDOW_FUNCTION_TYPE_RECTANGULAR:
                status = getWindowValueRectangular( aIndex, aValue );
                break;

            default:
                break;
        }
    }

    return status;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//! *** Blackman ***
//! See (19) in [4]
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValueBlackman
    (
    const uint32_t aIndex,              //!< index
    double*        aValue               //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        const double A0 = 0.42;
        const double A1 = 0.50;
        const double A2 = 0.08;
        double q = Numeric::TWO_PI * static_cast<double>( aIndex ) / mFftSize;
        *aValue = A0 - A1 * cos( q ) + A2 * cos( 2.0 * q );
    }

    return status;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//! *** Gaussian (Weierstrass) ***
//! See (44) in [3]
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValueGaussian
    (
    const uint32_t aIndex,              //!< index
    double*        aValue               //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        const double ALPHA = mParametersMap.at( WINDOW_FUNCTION_TYPE_GAUSSIAN ).crtValue;
        *aValue = exp( -0.5 * pow( ( 2.0 * static_cast<double>( aIndex ) - mFftSize ) / ( mFftSize / ALPHA ), 2.0 ) );
    }

    return status;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//! *** Hamming ***
//! See (30) in [3]
//! See (E.4) in [2]
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValueHamming
    (
    const uint32_t aIndex,              //!< index
    double*        aValue               //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        const double A0 = 25.0 / 46.0;
        *aValue = A0 - ( 1.0 - A0 ) * cos( Numeric::TWO_PI * static_cast<double>( aIndex ) / mFftSize );
    }

    return status;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//! *** Hann ***
//! See (27) in [3]
//! See (3) in [1]
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValueHann
    (
    const uint32_t aIndex,              //!< index
    double*        aValue               //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        *aValue = 0.5 * ( 1.0 - cos( Numeric::TWO_PI * static_cast<double>( aIndex ) / mFftSize ) );
    }

    return status;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//! *** Kaiser-Bessel ***
//! See (46) in [3]
//! See (4) in [1]
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValueKaiserBessel
    (
    const uint32_t aIndex,              //!< index
    double*        aValue               //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        const double ALPHA = mParametersMap.at( WINDOW_FUNCTION_TYPE_KAISER_BESSEL ).crtValue;
        const double PIAL = Numeric::PI * ALPHA;
        *aValue = std::cyl_bessel_i( 0, PIAL * sqrt( 1.0 - pow( 2.0 * static_cast<double>( aIndex ) / mFftSize - 1.0, 2.0 ) ) );
        *aValue /= std::cyl_bessel_i( 0, PIAL );
    }

    return status;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//! *** Nuttall ***
//! See (34) in [4].
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValueNuttall
    (
    const uint32_t aIndex,              //!< index
    double*        aValue               //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        const double A0 = 0.355768;
        const double A1 = 0.487396;
        const double A2 = 0.144232;
        const double A3 = 0.012604;
        double q = Numeric::TWO_PI * static_cast<double>( aIndex ) / mFftSize;
        *aValue = A0 - A1 * cos( q ) + A2 * cos( 2.0 * q ) - A3 * cos( 3.0 * q );
    }

    return status;
}


//!************************************************************************
//! Get the value of the window function for a specified index
//! *** rectangular (boxcar, uniform, Dirichlet) ***
//! See (21) in [3]
//! See (2) in [1]
//!
//! @returns The function value
//!************************************************************************
bool WindowFunction::getWindowValueRectangular
    (
    const uint32_t aIndex,              //!< index
    double*        aValue               //!< value
    ) const
{
    bool status = aValue != nullptr && aIndex <= mFftSize;

    if( status )
    {
        *aValue = 1.0;
    }

    return status;
}


//!************************************************************************
//! Set the active type
//!
//! @returns true if the type could be set
//!************************************************************************
bool WindowFunction::setActiveType
    (
    const WindowFunctionType aType         //!< type
    )
{
    bool status = aType >= WINDOW_FUNCTION_TYPE_BLACKMAN
               && aType <= WINDOW_FUNCTION_TYPE_RECTANGULAR;

    if( status )
    {
        mActiveType = aType;
    }

    return status;
}


//!************************************************************************
//! Set the FFT size
//!
//! @returns true if the value could be set
//!************************************************************************
bool WindowFunction::setFftSize
    (
    const uint32_t  aSize               //!< FFT size
    )
{
    bool status = aSize > 0;

    if( status )
    {
        mFftSize = aSize;
    }

    return status;
}


//!************************************************************************
//! Set a new value for the window function parameter.
//! Not all window functions have a parameter defined.
//!
//! @returns true if the value could be set
//!************************************************************************
bool WindowFunction::setParameter
    (
    const WindowFunctionType aWindowType,   //!< window type
    const double             aValue         //!< parameter value
    )
{
    bool status = mParametersMap.at( aWindowType ).needsParameter;

    if( status )
    {
        if( mParametersMap.at( aWindowType ).minValue > aValue
         || mParametersMap.at( aWindowType ).maxValue < aValue )
        {
            status = false;
        }
    }

    if( status )
    {
        mParametersMap.at( aWindowType ).crtValue = aValue;
    }

    return status;
}
