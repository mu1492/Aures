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
Numeric.h

This file contains the definitions for numerical constants and functions.
*/

#ifndef Numeric_h
#define Numeric_h

#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

#include <QMetaType>

using cdouble = std::complex<double>;
using CxVector = std::vector<cdouble>;
using CxMatrix = std::vector<CxVector>;
using Cx3Matrix = std::vector<CxMatrix>;


//************************************************************************
// Class for handling vibrations monitoring
//************************************************************************
class Numeric
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        static constexpr double PI       = 4.0 * atan( 1.0 );   //!< pi
        static constexpr double TWO_PI   = 2.0 * PI;            //!< 2*pi


    //************************************************************************
    // functions
    //************************************************************************
    public:
        Numeric();

        ~Numeric();

        static Numeric* getInstance();

        cdouble calculateDeterminantGaussian
            (
            CxMatrix aMatrix           //!< matrix
            );

        CxMatrix calculateGramMatrix
            (
            const CxMatrix& aMatrix    //!< matrix
            ) const;

        double calculateRms
            (
            double      aData[],        //!< data array
            uint32_t    aStartIndex,    //!< first index used
            uint32_t    aStopIndex      //!< last index used
            ) const;

        double calculateRms
            (
            const std::vector<double> aData   //!< data vector
            ) const;

        bool containsNan
            (
            const std::vector<double>& aVector  //!< data vector
            );

        CxMatrix createIdentityMatrix
            (
            const int aSize     //!< matrix size
            ) const;

        double deg2Rad
            (
            const double aAngleDeg  //!< angle [deg]
            ) const;

        double dotProduct
            (
            const double aX1,   //!< x1
            const double aY1,   //!< y1
            const double aX2,   //!< x2
            const double aY2    //!< y2
            ) const;

        CxMatrix invertMatrix
            (
            CxMatrix aMatrix    //!< matrix
            ) const;

        bool isPowerOfTwo
            (
            const uint32_t aNumber      //!< number
            ) const;

        double rad2Deg
            (
            const double aAngleRad  //!< angle [rad]
            ) const;

        void swap
            (
            double&     aVariable1,     //!< 1st variable
            double&     aVariable2      //!< 2nd variable
            ) const;


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static Numeric*   sInstance;          //!< singleton
};

Q_DECLARE_METATYPE( CxVector )
Q_DECLARE_METATYPE( CxMatrix )
Q_DECLARE_METATYPE( Cx3Matrix )

#endif // Numeric_h
