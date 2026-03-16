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
Aures.h

This file contains the definitions for the acoustics project.
*/

#ifndef Aures_h
#define Aures_h

#include "./ui_About.h"
#include "./ui_MicBoard.h"

#include "AsrHandler.h"
#include "MicArray.h"
#include "RawSignalHandler.h"

#if BUILD_ROS
    #include "RosPublisher.h"
#endif

#include <memory>
#include <string>
#include <vector>

#include <QCloseEvent>
#include <QDialog>
#include <QMainWindow>

class AudioCaptureThread;

QT_BEGIN_NAMESPACE
    namespace Ui
    {
        class Aures;
    }
QT_END_NAMESPACE


//************************************************************************
// Class for handling the acoustics project
//************************************************************************
class Aures : public QMainWindow
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************


    //************************************************************************
    // functions
    //************************************************************************
    public:
        Aures
            (
            QWidget*    aParent = nullptr   //!< parent widget
            );

        ~Aures();

    protected:
        void closeEvent
            (
            QCloseEvent* aEvent             //!< close event
            );

    private:
#if BUILD_ROS
        std::string formatRosMessage
            (
            const std::string   aMessage,           //!< message
            const double        aAzimuthDeg = 0,    //!< az [deg]
            const double        aElevationDeg = 0   //!< el [deg]
            );
#endif

    private slots:
        void handleAbout();

        void handleSoundLevels();

        void handleSoundLevelsClose();

        void handleSoundLevelsRmsUpdate
            (
            ChannelValueDouble aRmsInfo     //!< RMS [dBFS] information
            );

        void receiveAsrString
            (
            QString aString                 //!< ASR string
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        Ui::Aures*          mMainUi;                //!< main UI
        Ui::AboutDialog*    mAboutUi;               //!< about dialog
        Ui::MicBoardDialog* mMicBoardUi;            //!< mic board dialog

        QDialog             mSoundLevelsDlg;        //!< dialog for sound levels

        MicArray*           mMicArrayInstance;      //!< microphone array

        AudioCaptureThread* mAudioCaptureThread;    //!< audio capture thread

        RawSignalHandler*   mRawSignalHandler;      //!< audio raw signal handler
        AsrHandler*         mAsrHandler;            //!< ASR handler

#if BUILD_ROS
        RosPublisher*       mRosPublisher;          //!< ROS publisher
        QThread*            mRosThread;             //!< ROS thread
#endif
};

#endif // Aures_h
