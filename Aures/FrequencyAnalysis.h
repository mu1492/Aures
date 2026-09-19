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
FrequencyAnalysis.h

This file contains the definitions for the frequency analysis.
*/

#ifndef FrequencyAnalysis_h
#define FrequencyAnalysis_h

#include "WindowFunction.h"

#include <cstdint>
#include <map>

class Numeric;


//************************************************************************
// Class for handling frequency analysis
//************************************************************************
class FrequencyAnalysis
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        //************************************************************************
        // Fast Fourier Transform
        //************************************************************************
        typedef enum : uint8_t
        {
            FFT_SIZE_256,
            FFT_SIZE_512,
            FFT_SIZE_1024,
            FFT_SIZE_2048,
            // keep this last
            FFT_SIZE_MAX
        }FftSizeOption;

        static const std::map<FftSizeOption, uint32_t> FFT_SIZE_VALUES;

        typedef enum : int8_t
        {
            FFT_SENSE_DIRECT  =  1,
            FFT_SENSE_INVERSE = -1
        }FftSense;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        FrequencyAnalysis();

        ~FrequencyAnalysis();

        static FrequencyAnalysis* getInstance();

        void calculateFourierTransform
            (
            double          aData[],            //!< data, dimension is right shifted FFT size
            const uint32_t  aShrFftSize,        //!< right shifted FFT size, must be a power of two
            const FftSense  aTransformSense     //!< direct or inverse transform
            );

        double getFftBinWidth() const;

        double getFftFreqMax() const;

        FftSizeOption getFftSizeOptionIndex() const;

        uint32_t getFftSizeValue() const;

        double getFftSps() const;

        double getFftTimeGate() const;

        WindowFunction& getWindowFunction();

        bool setFftSizeOptionIndex
            (
            const FftSizeOption aFftSizeIndex           //!< FFT size index
            );

        bool setFftSps
            (
            const double        aSps                    //!< SPS [Hz]
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static FrequencyAnalysis*   sInstance;              //!< singleton
        Numeric*                    mNumericInstance;       //!< Numeric instance
        double                      mSps;                   //!< sampling rate [Hz]

        double                      mFftFreqMax;            //!< FFT max frequency [Hz]
        FftSizeOption               mFftSizeIndex;          //!< FFT size index
        uint32_t                    mFftSizeValue;          //!< FFT size value
        double                      mFftBinWidth;           //!< FFT bin width [Hz/bin]
        double                      mFftTimeGate;           //!< FFT time gate [s]

        WindowFunction              mWindowFunction;        //!< FFT window function
};

#endif // FrequencyAnalysis_h
