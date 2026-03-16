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
MicGain.cpp

This file contains the sources for microphone gain calculator.
*/

#include "MicGain.h"
#include "./ui_MicGain.h"


//!************************************************************************
//! Constructor
//!************************************************************************
MicGain::MicGain
    (
    QWidget*    aParent //!< parent widget
    )
    : QMainWindow( aParent )
    , mMainUi( new Ui::MicGain )
    , mSpeechLevelDb( 60.0 )     // dB SPL @1m
    , mDistanceM( 1.5 )          // m
    , mMicSensitivityDb( -26.0 ) // dB FS
    , mAsrInputDb( -18.0 )       // dB FS RMS
    , mGainDb( 0 )               // dB
{
    mMainUi->setupUi( this );

    mMainUi->speechLevelSpinBox->setMinimum( SPEECH_LEVEL.min );
    mMainUi->speechLevelSpinBox->setMaximum( SPEECH_LEVEL.max );
    mMainUi->speechLevelSpinBox->setValue( mSpeechLevelDb );
    connect( mMainUi->speechLevelSpinBox, SIGNAL( valueChanged(double) ), this, SLOT( handleChangedSpeechLevel(double) ) );

    mMainUi->distanceSpinBox->setMinimum( DISTANCE.min );
    mMainUi->distanceSpinBox->setMaximum( DISTANCE.max );
    mMainUi->distanceSpinBox->setValue( mDistanceM );
    connect( mMainUi->distanceSpinBox, SIGNAL( valueChanged(double) ), this, SLOT( handleChangedDistance(double) ) );

    mMainUi->micSensitivitySpinBox->setMinimum( MIC_SENSITIVITY.min );
    mMainUi->micSensitivitySpinBox->setMaximum( MIC_SENSITIVITY.max );
    mMainUi->micSensitivitySpinBox->setValue( mMicSensitivityDb );
    connect( mMainUi->micSensitivitySpinBox, SIGNAL( valueChanged(double) ), this, SLOT( handleChangedMicSensitivity(double) ) );

    mMainUi->asrInputSpinBox->setMinimum( ASR_INPUT.min );
    mMainUi->asrInputSpinBox->setMaximum( ASR_INPUT.max );
    mMainUi->asrInputSpinBox->setValue( mAsrInputDb );
    connect( mMainUi->asrInputSpinBox, SIGNAL( valueChanged(double) ), this, SLOT( handleChangedAsrInput(double) ) );

    calculateGain();
    mMainUi->micGainValue->setText( createGainString() );
}


//!************************************************************************
//! Destructor
//!************************************************************************
MicGain::~MicGain()
{
    delete mMainUi;
}


//!************************************************************************
//! Calculate the microphone gain
//!
//! @returns nothing
//!************************************************************************
void MicGain::calculateGain()
{
    mGainDb = -mSpeechLevelDb + 20.0 * log10( mDistanceM )
            - mMicSensitivityDb
            + mAsrInputDb
            + REF_SPL;
}


//!************************************************************************
//! Creates the formatted string with the microphone gain
//!
//! @returns The formatted string with the mic gain
//!************************************************************************
QString MicGain::createGainString()
{
    QString gainStr = QString::number( mGainDb, 'f', 2 ) + " dB";
    return gainStr;
}


//!************************************************************************
//! Handle for changing the ASR input level [dB FS RMS]
//!
//! @returns nothing
//!************************************************************************
void MicGain::handleChangedAsrInput
    (
    double aValue   //!< value
    )
{
    if( aValue >= ASR_INPUT.min && aValue <= ASR_INPUT.max )
    {
        mAsrInputDb = aValue;
        calculateGain();
        updateGainUi();
    }
}


//!************************************************************************
//! Handle for changing the distance [m]
//!
//! @returns nothing
//!************************************************************************
void MicGain::handleChangedDistance
    (
    double aValue   //!< value
    )
{
    if( aValue >= DISTANCE.min && aValue <= DISTANCE.max )
    {
        mDistanceM = aValue;
        calculateGain();
        updateGainUi();
    }
}


//!************************************************************************
//! Handle for changing the microphone sensitivity [dB FS]
//!
//! @returns nothing
//!************************************************************************
void MicGain::handleChangedMicSensitivity
    (
    double aValue   //!< value
    )
{
    if( aValue >= MIC_SENSITIVITY.min && aValue <= MIC_SENSITIVITY.max )
    {
        mMicSensitivityDb = aValue;
        calculateGain();
        updateGainUi();
    }
}


//!************************************************************************
//! Handle for changing the speech level [dB SPL @1m]
//!
//! @returns nothing
//!************************************************************************
void MicGain::handleChangedSpeechLevel
    (
    double aValue   //!< value
    )
{
    if( aValue >= SPEECH_LEVEL.min && aValue <= SPEECH_LEVEL.max )
    {
        mSpeechLevelDb = aValue;
        calculateGain();
        updateGainUi();
    }
}


//!************************************************************************
//! Update the mic gain UI controls
//!
//! @returns nothing
//!************************************************************************
void MicGain::updateGainUi()
{
    mMainUi->micGainValue->setText( createGainString() );
}
