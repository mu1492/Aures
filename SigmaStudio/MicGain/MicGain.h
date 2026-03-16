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
MicGain.h

This file contains the definitions for microphone gain calculator.
*/

#ifndef MicGain_h
#define MicGain_h

#include <cmath>

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
    namespace Ui
    {
        class MicGain;
    }
QT_END_NAMESPACE


//************************************************************************
// Class for handling the strong motion monitoring
//************************************************************************
class MicGain : public QMainWindow
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    private:
        static constexpr double REF_SPL = 20.0 * log10( 1.0 / 2.e-5 );

        typedef struct
        {
            double min;
            double max;
        }MinMax;

        static constexpr MinMax SPEECH_LEVEL    = {  50.0,  75.0 };
        static constexpr MinMax DISTANCE        = {   0.5,   5.0 };
        static constexpr MinMax MIC_SENSITIVITY = { -45.0, -25.0 };
        static constexpr MinMax ASR_INPUT       = { -24.0, -12.0 };


    //************************************************************************
    // functions
    //************************************************************************
    public:
        MicGain
            (
            QWidget*    aParent = nullptr   //!< parent widget
            );

        ~MicGain();

    private:
        void calculateGain();

        QString createGainString();

        void updateGainUi();

    private slots:
        void handleChangedAsrInput
            (
            double aValue   //!< value
            );

        void handleChangedDistance
            (
            double aValue   //!< value
            );

        void handleChangedMicSensitivity
            (
            double aValue   //!< value
            );

        void handleChangedSpeechLevel
            (
            double aValue   //!< value
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        Ui::MicGain*    mMainUi;

        double          mSpeechLevelDb;      //!< speech level [dB SPL @1m]
        double          mDistanceM;          //!< distance [m]
        double          mMicSensitivityDb;   //!< microphone sensitivity [dB FS]
        double          mAsrInputDb;         //!< ASR input level [dB FS RMS]
        double          mGainDb;             //!< calculated mic gain [dB]
};

#endif // MicGain_h
