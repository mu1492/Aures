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
WindowFunction.h

This file contains the definitions for the FFT window functions.
*/

#ifndef WindowFunction_h
#define WindowFunction_h

#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

#include <QString>


//************************************************************************
// Class for handling vibrations monitoring
//************************************************************************
class WindowFunction
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        //************************************************************************
        // Defined window functions
        //************************************************************************
        typedef enum : uint8_t
        {
            WINDOW_FUNCTION_TYPE_BLACKMAN,
            WINDOW_FUNCTION_TYPE_GAUSSIAN,
            WINDOW_FUNCTION_TYPE_HAMMING,
            WINDOW_FUNCTION_TYPE_HANN,
            WINDOW_FUNCTION_TYPE_KAISER_BESSEL,
            WINDOW_FUNCTION_TYPE_NUTTALL,
            WINDOW_FUNCTION_TYPE_RECTANGULAR,

            // keep this last
            WINDOW_FUNCTION_TYPE_MAX_COUNT
        }WindowFunctionType;

        typedef struct
        {
            bool   isEmphasized;    //!< true if emphasized for mechanical vibrations
            bool   needsParameter;  //!< true if the type needs a parameter
            double minValue;        //!< parameter minimum allowed value
            double crtValue;        //!< parameter current value
            double maxValue;        //!< parameter maximum allowed value
            QString paramSymbol;    //!< symbol for the parameter
        }ParameterData;

    private:
        const QString ALPHA_SMALL = QString::fromUtf8( "\u03B1" );   //!< small alpha


    //************************************************************************
    // functions
    //************************************************************************
    public:
        WindowFunction();

        ~WindowFunction();

        WindowFunctionType getActiveType();

        std::map<WindowFunctionType, ParameterData> getParametersMap() const;

        bool getWindowValue
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;

        bool setActiveType
            (
            const WindowFunctionType aType      //!< type
            );

        bool setFftSize
            (
            const uint32_t  aSize               //!< FFT size
            );

        bool setParameter
            (
            const WindowFunctionType aWindowType,   //!< window type
            const double             aValue         //!< parameter value
            );

    private:
        bool getWindowValueBlackman
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;

        bool getWindowValueGaussian
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;

        bool getWindowValueHamming
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;

        bool getWindowValueHann
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;

        bool getWindowValueKaiserBessel
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;

        bool getWindowValueNuttall
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;

        bool getWindowValueRectangular
            (
            const uint32_t aIndex,              //!< index
            double*        aValue               //!< value
            ) const;


    //************************************************************************
    // variables
    //************************************************************************
    private:
        uint32_t                                    mFftSize;               //!< FFT size
        std::map<WindowFunctionType, ParameterData> mParametersMap;         //!< available parameters
        WindowFunctionType                          mActiveType;            //!< active window function
};

#endif // WindowFunction_h
