///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023,2026 Mihai Ursu                                            //
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
Numeric.cpp

This file contains the sources for numerical constants and functions.
*/

#include "Numeric.h"

#include <algorithm>


Numeric* Numeric::sInstance = nullptr;

//!************************************************************************
//! Constructor
//!************************************************************************
Numeric::Numeric()
{
}


//!************************************************************************
//! Destructor
//!************************************************************************
Numeric::~Numeric()
{
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
Numeric* Numeric::getInstance()
{
    if( !sInstance )
    {
        sInstance = new Numeric;
    }

    return sInstance;
}


//!************************************************************************
//! Calculate the determinant of a matrix using Gaussian elimination with
//! partial pivoting
//!
//! @returns the determinant
//!************************************************************************
cdouble Numeric::calculateDeterminantGaussian
    (
    CxMatrix aMatrix           //!< matrix
    )
{
    size_t N = aMatrix.size();
    cdouble det( 1, 0 );

    for( size_t i = 0; i < N; i++ )
    {
        size_t pivot = i;

        for( size_t j = i + 1; j < N; j++ )
        {
            if( abs( aMatrix[j][i] ) > abs( aMatrix[pivot][i] ) )
            {
                pivot = j;
            }
        }

        if( abs( aMatrix[pivot][i] ) < 1e-12 )
        {
            const cdouble CZERO;
            det = CZERO;
            break;
        }

        if( pivot != i )
        {
            std::swap( aMatrix[i], aMatrix[pivot] );
            det *= -1.0;
        }

        det *= aMatrix[i][i];

        for( size_t j = i + 1; j < N; j++ )
        {
            cdouble factor = aMatrix[j][i] / aMatrix[i][i];

            for( size_t k = i; k < N; k++ )
            {
                aMatrix[j][k] -= factor * aMatrix[i][k];
            }
        }
    }

    return det;
}


//!************************************************************************
//! Calculate the Gram matrix
//! G = A^H * A
//!
//! @returns the Gram matrix
//!************************************************************************
CxMatrix Numeric::calculateGramMatrix
    (
    const CxMatrix& aMatrix    //!< matrix A
    ) const
{
    size_t M = aMatrix[0].size();
    size_t N = aMatrix.size();
    CxMatrix G( N, CxVector( N ) );

    for( size_t i = 0; i < N; i++ )
    {
        for( size_t j = 0; j < N; j++ )
        {
            cdouble sum;

            for( size_t k = 0; k < M; k++ )
            {
                sum += std::conj( aMatrix[i][k] ) * aMatrix[j][k];
            }

            G[i][j] = sum;
        }
    }

    return G;
}


//!************************************************************************
//! Calculate the root mean square of a data array
//!
//! @returns the RMS value
//!************************************************************************
double Numeric::calculateRms
    (
    double      aData[],        //!< data array
    uint32_t    aStartIndex,    //!< first index used
    uint32_t    aStopIndex      //!< last index used
    ) const
{
    double retValue = 0;
    bool status = ( nullptr != aData ) && ( aStartIndex <= aStopIndex );

    if( status )
    {
        for( uint32_t i = aStartIndex; i <= aStopIndex; i++ )
        {
            retValue += aData[i] * aData[i];
        }

        retValue = sqrt( retValue / static_cast<double>( 1 + aStopIndex - aStartIndex ) );
    }

    return retValue;
}


//!************************************************************************
//! Calculate the root mean square of a data vector
//!
//! @returns the RMS value
//!************************************************************************
double Numeric::calculateRms
    (
    const std::vector<double> aData     //!< data vector
    ) const
{
    double retValue = 0;
    size_t len = aData.size();

    if( len )
    {
        for( size_t i = 0; i < len; i++ )
        {
            retValue += aData[i] * aData[i];
        }

        retValue = sqrt( retValue / static_cast<double>( len ) );
    }

    return retValue;
}


//!************************************************************************
//! Looks for at least one presence of NaN in a vector
//!
//! @returns true if at least one element is NaN
//!************************************************************************
bool Numeric::containsNan
    (
    const std::vector<double>& aVector  //!< data vector
    )
{
    return std::any_of( aVector.begin(), aVector.end(), []( double x )
    {
        return std::isnan( x );
    });
}


//!************************************************************************
//! Create an identity matrix
//!
//! @returns the identity matrix
//!************************************************************************
CxMatrix Numeric::createIdentityMatrix
    (
    const int aSize     //!< matrix size
    ) const
{
    CxMatrix idMatrix( aSize, CxVector( aSize ) );

    for( size_t i = 0; i < aSize; i++ )
    {
        idMatrix[i][i] = 1.0;
    }

    return idMatrix;
}


//!************************************************************************
//! Convert a value from degrees to radians
//!
//! @returns the converted value [rad]
//!************************************************************************
double Numeric::deg2Rad
    (
    const double aAngleDeg  //!< angle [deg]
    ) const
{
    return aAngleDeg * PI / 180.0;
}


//!************************************************************************
//! Calculate the dot product
//!
//! @returns the value of the dot product
//!************************************************************************
double Numeric::dotProduct
    (
    const double aX1,   //!< x1
    const double aY1,   //!< y1
    const double aX2,   //!< x2
    const double aY2    //!< y2
    ) const
{
    return aX1 * aX2 + aY1 * aY2;
}


//!************************************************************************
//! Invert a matrix with complex elements
//! Gauss-Jordan method.
//!
//! @returns the inverted matrix
//!************************************************************************
CxMatrix Numeric::invertMatrix
    (
    CxMatrix aMatrix    //!< matrix
    ) const
{
    const size_t N = aMatrix.size();
    CxMatrix invMatrix = aMatrix;

    if( N )
    {    
        invMatrix = createIdentityMatrix( N );
        const double EPS = 1e-12;

        for( size_t i = 0; i < N; i++ )
        {
            size_t pivotRow = i;

            for( ; pivotRow < N; pivotRow++ )
            {
                if( std::abs( aMatrix[pivotRow][i] ) > EPS )
                {
                    break;
                }
            }

            if( N == pivotRow )
            {
                invMatrix = aMatrix;
                break;
            }

            if( pivotRow != i )
            {
                std::swap( aMatrix[i], invMatrix[i] );
                std::swap( aMatrix[pivotRow], invMatrix[pivotRow] );
            }

            const cdouble PIVOT = aMatrix[i][i];

            for( size_t j = 0; j < N; j++ )
            {
                aMatrix[i][j] /= PIVOT;
                invMatrix[i][j] /= PIVOT;
            }

            for( size_t k = 0; k < N; k++ )
            {
                if( k == i )
                {
                    continue;
                }

                const cdouble FACTOR = aMatrix[k][i];

                if( std::abs( FACTOR ) <= EPS )
                {
                    continue;
                }

                for( size_t j = 0; j < N; j++ )
                {
                    aMatrix[k][j] -= FACTOR * aMatrix[i][j];
                    invMatrix[k][j] -= FACTOR * invMatrix[i][j];
                }
            }
        }
    }

    return invMatrix;
}


//! ***********************************************************************
//! Check if a number is a power of two
//!
//! @returns true if the parameter is a power of two
//!************************************************************************
bool Numeric::isPowerOfTwo
    (
    const uint32_t aNumber  //!< number
    ) const
{
    bool retval = false;

    if( aNumber > 0 )
    {
        retval = ( 0 == ( aNumber & ( aNumber - 1 ) ) );
    }

    return retval;
}


//!************************************************************************
//! Convert a value from radians to degrees
//!
//! @returns the converted value [deg]
//!************************************************************************
double Numeric::rad2Deg
    (
    const double aAngleRad  //!< angle [rad]
    ) const
{
    return aAngleRad * 180.0 / PI;
}


//!************************************************************************
//! Swap the values of two numbers
//!
//! @returns nothing
//!************************************************************************
void Numeric::swap
    (
    double&     aVariable1,     //!< 1st variable
    double&     aVariable2      //!< 2nd variable
    ) const
{
    double temp = aVariable1;
    aVariable1 = aVariable2;
    aVariable2 = temp;
}
