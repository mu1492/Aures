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
#include "./ui_MicBoardRms.h"

#include "AcousticsHandler.h"
#include "BeampatternThread.h"
#include "AsrHandler.h"
#include "FrequencyAnalysis.h"
#include "MicArray.h"
#include "MicArrayConfigThread.h"
#include "MultiSourceHandler.h"
#include "Numeric.h"
#include "RawSignalHandler.h"
#include "WeightsThread.h"

#if BUILD_ROS
    #include "RosPublisher.h"
#endif

#include <memory>
#include <string>
#include <vector>

#include <QCloseEvent>
#include <QDialog>
#include <QMainWindow>
#include <QThread>

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
        void calculateActiveWeights();

        void calculateBeampattern();

        void findOptimumMicArray();

        void initAsrLanguageControls();

        void prepareRedimensioning();

        void updateActiveBeamformers
            (
            bool aRedimensionWeightsMatrix  //!< if redimensioning the weights matrix
            );

        void updateActiveBeamformersConfig();

        void updateAsrVadControls();

        void updateAsrSpeakerLabels();

        void updateDasOnlyControls();

        void updateDasOnlyValues();

#if BUILD_ROS
        std::string formatRosMessage
            (
            const std::string   aMessage,           //!< message
            const double        aAzimuthDeg = 0,    //!< az [deg]
            const double        aElevationDeg = 0   //!< el [deg]
            );
#endif

    public slots:
        void setSpeakerLanguagesChange
            (
            bool aStatus        //!< status
            );

        void receiveNewRnn
            (
            const Cx3Matrix& aRnnMatrix     //!< Rnn matrix
            );

    private slots:
        void handleAbout();

        void handleChangedBeampatternDoaDial
            (
            int aValue          //!< value
            );

        void handleChangedBeampatternDoaSpinbox
            (
            int aValue          //!< value
            );

        void handleChangedBeampatternFrequency
            (
            double aValue       //!< value
            );

        void handleChangedBeampatternFreqCenter
            (
            double aValue       //!< value
            );

        void handleChangedBeampatternRadialApodization
            (
            bool aEnabled       //!< state
            );

        void handleChangedBeampatternShowSpeakers
            (
            bool aEnabled       //!< state
            );

        void handleChangedBeampatternType
            (
            int aIndex          //!< index
            );

        void handleChangedFrequencyRange
            (
            int aIndex          //!< index
            );

        void handleChangedNoiseThd
            (
            int aValue          //!< value
            );

        void handleChangedOverlapAddGain
            (
            int aValue          //!< value
            );

        void handleChangedSpeaker1Lang
            (
            int aValue          //!< value
            );

        void handleChangedSpeaker2Lang
            (
            int aValue          //!< value
            );

        void handleChangedSpeaker3Lang
            (
            int aValue          //!< value
            );

        void handleChangedSpeaker4Lang
            (
            int aValue          //!< value
            );

        void handleChangedSpeaker5Lang
            (
            int aValue          //!< value
            );

        void handleChangedSpeaker6Lang
            (
            int aValue          //!< value
            );

        void handleChangedSpeaker7Lang
            (
            int aValue          //!< value
            );

        void handleChangedSpeakerAngleSpinBox
            (
            int aValue          //!< value
            );

        void handleChangedSpeakerRingAngle
            (
            int aValue          //!< value
            );

        void handleChangedSpeakersNr
            (
            int aNrOfSpeakers   //!< number of speakers
            );

        void handleChangedSpeechThd
            (
            int aValue          //!< value
            );

        void handleExit();

        void handleResetNoiseFloor();

        void handleSoundLevels();

        void handleSoundLevelsClose();

        void handleSoundLevelsRmsUpdate
            (
            ChannelValueDouble aRmsInfo     //!< RMS [dBFS] information
            );

        void receiveAsrString
            (
            QString aString,                //!< ASR string
            int     aSpeakerId              //!< speaker
            );

        void receiveNewBeampattern
            (
            CxVector aVector                //!< beampattern values
            );

        void receiveNewMicArrayConfig
            (
            MicArray aMicArray              //!< microphone array
            );

        void receiveNewVad
            (
            bool                            aIsVoice,       //!< true if voice is detected
            int                             aIndex,         //!< index
            VoiceActivityDetection::Source  aSource         //!< source
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        Ui::Aures*              mMainUi;                //!< main UI
        Ui::AboutDialog*        mAboutUi;               //!< about dialog
        Ui::MicBoardRmsDialog*  mMicBoardRmsUi;         //!< mic board dialog
        QLabel                  mStatusbarLabel;        //!< label on statusbar
        QDialog                 mSoundLevelsDlg;        //!< dialog for sound levels

        Numeric*                mNumericInstance;       //!< numeric instance
        AcousticsHandler*       mAcousticsInstance;     //!< acoustics
        AudioCaptureThread*     mAudioCaptureThread;    //!< audio capture thread
        FrequencyAnalysis*      mFreqAnalysisInstance;  //!< frequency analysis instance
        RawSignalHandler*       mRawSignalHandler;      //!< audio raw signal handler

        QThread*                mRawSignalThread;       //!< thread for the raw signal

        std::vector<AsrHandler*>    mAsrHandlerVec;     //!< ASR handler vector

        MultiSourceHandler      mMultiSrcHndl;          //!< multiple source handler

        MicArray                mMicArrayOptimum;       //!< optimum configuration of the mic array
        MicArrayConfigThread*   mMicArrayConfigThread;  //!< thread for configuring the mic array

        Beamforming::BeampatternConfig  mBeampatternConfig; //!< beampattern configuration parameters
        BeampatternThread*      mBeampatternThread;     //!< beampattern compute thread
        CxVector                mBeampatternCxValues;   //!< beampattern values at given configuration

        bool                    mBeampatternShowSpeakers;   //!< true if showing speaker directions on beampattern

        std::vector<Beamforming::BeampatternConfig> mActiveBeamformersConfigsVec;   //!< vector of configurations for active beamformers
        std::vector<WeightsThread>                  mActiveWeightsThreadsVec;       //!< vector of threads for calculating active weights

#if BUILD_ROS
        RosPublisher*           mRosPublisher;          //!< ROS publisher
        QThread*                mRosThread;             //!< ROS thread
#endif
};

#endif // Aures_h
