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
Aures.cpp

This file contains the sources for the acoustics project.
*/

#include "Aures.h"
#include "./ui_Aures.h"

#include "AudioCaptureThread.h"
#include "AudioChannelData.h"

#include <iomanip>
#include <sstream>


//!************************************************************************
//! Constructor
//!************************************************************************
Aures::Aures
    (
    QWidget*    aParent //!< parent widget
    )
    : QMainWindow( aParent )
    // UI
    , mMainUi( new Ui::Aures )
    , mAboutUi( new Ui::AboutDialog )
    , mMicBoardUi( new Ui::MicBoardDialog )
    // audio capture
    , mAudioCaptureThread( nullptr )
    // audio raw signals
    , mRawSignalHandler( nullptr )
    // ASR
    , mAsrHandler( nullptr )
    // ROS
#if BUILD_ROS
    , mRosPublisher( nullptr )
    , mRosThread( nullptr )
#endif
{
    mMainUi->setupUi( this );

    //****************************************
    // microphone array
    //****************************************
    mMicArrayInstance = MicArray::getInstance();

    //****************************************
    // ALSA
    //****************************************
    mAudioCaptureThread = new AudioCaptureThread();

    if( mAudioCaptureThread )
    {
        mAudioCaptureThread->start();
    }

    //****************************************
    // acoustic raw signals
    //****************************************
    mRawSignalHandler = RawSignalHandler::getInstance();

    if( mRawSignalHandler )
    {
        connect( mAudioCaptureThread, SIGNAL( haveNewAudio( AudioChannelData ) ), this->mRawSignalHandler, SLOT( receiveNewAudio( AudioChannelData ) ) );
    }

    //****************************************
    // ASR
    //****************************************
    mAsrHandler = AsrHandler::getInstance();

    if( mAsrHandler )
    {
        connect( mAudioCaptureThread, SIGNAL( haveNewAudio( AudioChannelData ) ), this->mAsrHandler, SLOT( receiveNewAudio( AudioChannelData ) ) );
        connect( this->mAsrHandler, SIGNAL( haveNewString( QString ) ), this, SLOT( receiveAsrString( QString ) ) );
    }

    //****************************************
    // menus
    //****************************************
    connect( mMainUi->actionAbout, &QAction::triggered, this, &Aures::handleAbout );

    connect( mMainUi->actionSoundLevels, &QAction::triggered, this, &Aures::handleSoundLevels );

    //****************************************
    // ROS
    //****************************************
#if BUILD_ROS
    mRosPublisher = new RosPublisher();

    if( mRosPublisher )
    {
        mRosThread = new QThread();
        mRosPublisher->moveToThread( mRosThread );
        connect( mRosThread, &QThread::started, mRosPublisher, &RosPublisher::start );
        mRosThread->start();
    }
#endif
}


//!************************************************************************
//! Destructor
//!************************************************************************
Aures::~Aures()
{
#if BUILD_ROS
    if( mRosThread )
    {
        if( mRosThread->isRunning() )
        {
            mRosThread->quit();

            if( !mRosThread->wait( 500 ) )
            {
                mRosThread->terminate();
                mRosThread->wait();
            }

            delete mRosThread;
            mRosThread = nullptr;
        }
    }

    if( mRosPublisher )
    {
        delete mRosPublisher;
        mRosPublisher = nullptr;
    }
#endif

    if( mAsrHandler )
    {        
        mAsrHandler->destroyInstance();
        mAsrHandler = nullptr;
    }

    if( mRawSignalHandler )
    {
        mRawSignalHandler->destroyInstance();
        mRawSignalHandler = nullptr;
    }

    if( mAudioCaptureThread )
    {
        mAudioCaptureThread->stop();
        mAudioCaptureThread->quit();

        if( !mAudioCaptureThread->wait( 500 ) )
        {
            mAudioCaptureThread->terminate();
            mAudioCaptureThread->wait();
        }

        delete mAudioCaptureThread;
        mAudioCaptureThread = nullptr;
    }

    if( mMicArrayInstance )
    {
        mMicArrayInstance->destroyInstance();
        mMicArrayInstance = nullptr;
    }

    delete mMainUi;
}


//!************************************************************************
//! Close event handler
//!
//! @returns nothing
//!************************************************************************
void Aures::closeEvent
    (
    QCloseEvent*    aEvent      //!< close event
    )
{
    QApplication::quit();
    aEvent->accept();
}


#if BUILD_ROS
    //!************************************************************************
    //! Format a ROS message using a string, Az, and El
    //!
    //! @returns the formatted message
    //!************************************************************************
    std::string Aures::formatRosMessage
        (
        const std::string   aMessage,       //!< message
        const double        aAzimuthDeg,    //!< az [deg]
        const double        aElevationDeg   //!< el [deg]
        )
    {

        std::string fStr = aMessage;

        if( 0 != aAzimuthDeg || 0 != aElevationDeg )
        {
            std::stringstream ss;
            ss << std::fixed << "|" << std::setprecision( 1 ) << aAzimuthDeg << "|" << aElevationDeg;
            fStr += ss.str();
        }

        return fStr;
    }
#endif


//!************************************************************************
//! About dialog box
//!
//! @returns nothing
//!************************************************************************
/* slot */ void Aures::handleAbout()
{
    QDialog dialog;
    mAboutUi->setupUi( &dialog );
    connect( mAboutUi->OkButton, SIGNAL( clicked() ), &dialog, SLOT( close() ) );
    dialog.exec();
}


//!************************************************************************
//! Sound levels dialog box
//!
//! @returns nothing
//!************************************************************************
/* slot */ void Aures::handleSoundLevels()
{
    mMicBoardUi->setupUi( &mSoundLevelsDlg );
    connect( mMicBoardUi->CloseButton, SIGNAL( clicked() ), this, SLOT( handleSoundLevelsClose() ) );

    if( mRawSignalHandler )
    {
        connect( mRawSignalHandler, &RawSignalHandler::sendRms, this, &Aures::handleSoundLevelsRmsUpdate );
    }

    mSoundLevelsDlg.exec();
}


//!************************************************************************
//! Handle for closing the sound levels dialog box
//!
//! @returns nothing
//!************************************************************************
/* slot */ void Aures::handleSoundLevelsClose()
{
    if( mRawSignalHandler )
    {
        disconnect( mRawSignalHandler, &RawSignalHandler::sendRms, this, &Aures::handleSoundLevelsRmsUpdate );
    }

    mSoundLevelsDlg.close();
}


//!************************************************************************
//! Handle for updating the RMS values in the sound level dialog box
//!
//! @returns nothing
//!************************************************************************
/* slot */ void Aures::handleSoundLevelsRmsUpdate
    (
    ChannelValueDouble aRmsInfo     //!< RMS information [dBFS]
    )
{
    QString rmsStr = QString::number( aRmsInfo.value, 'f', 2 ) + " dBFS";

    if( mMicBoardUi && mSoundLevelsDlg.isVisible() )
    {
        switch( aRmsInfo.channel )
        {
            case 1:
                mMicBoardUi->mic01Value->setText( rmsStr );
                break;

            case 2:
                mMicBoardUi->mic02Value->setText( rmsStr );
                break;

            case 3:
                mMicBoardUi->mic03Value->setText( rmsStr );
                break;

            case 4:
                mMicBoardUi->mic04Value->setText( rmsStr );
                break;

            case 5:
                mMicBoardUi->mic05Value->setText( rmsStr );
                break;

            case 6:
                mMicBoardUi->mic06Value->setText( rmsStr );
                break;

            case 7:
                mMicBoardUi->mic07Value->setText( rmsStr );
                break;

            case 8:
                mMicBoardUi->mic08Value->setText( rmsStr );
                break;

            case 9:
                mMicBoardUi->mic09Value->setText( rmsStr );
                break;

            case 10:
                mMicBoardUi->mic10Value->setText( rmsStr );
                break;

            case 11:
                mMicBoardUi->mic11Value->setText( rmsStr );
                break;

            case 12:
                mMicBoardUi->mic12Value->setText( rmsStr );
                break;

            case 13:
                mMicBoardUi->mic13Value->setText( rmsStr );
                break;

            case 14:
                mMicBoardUi->mic14Value->setText( rmsStr );
                break;

            case 15:
                mMicBoardUi->mic15Value->setText( rmsStr );
                break;

            default:
                break;
        }
    }
}


//!************************************************************************
//! Receive new ASR string
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::receiveAsrString
    (
    QString aString                 //!< ASR string
    )
{
    if( aString.size() )
    {
        mMainUi->asrValue->setText( aString );

#if BUILD_ROS
        if( mRosPublisher )
        {
            mRosPublisher->publishMessage( formatRosMessage( aString.toStdString() ) );
        }
#endif
    }
}
