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
#include "Beamforming.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include <QMessageBox>


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
    , mMicBoardRmsUi( new Ui::MicBoardRmsDialog )
    // numeric
    , mNumericInstance( Numeric::getInstance() )
    // acoustics
    , mAcousticsInstance( nullptr )
    // audio capture
    , mAudioCaptureThread( nullptr )
    // frequency analysis
    , mFreqAnalysisInstance( nullptr )
    // audio raw signals
    , mRawSignalHandler( nullptr )
    , mRawSignalThread( new QThread( this ) )
    // mic array configuration
    , mMicArrayConfigThread( nullptr )
    // beampattern compute
    , mBeampatternThread( nullptr )
    , mBeampatternShowSpeakers( true )
    // ROS
#if BUILD_ROS
    , mRosPublisher( nullptr )
    , mRosThread( nullptr )
#endif
{
    mMainUi->setupUi( this );
    mMainUi->statusbar->addWidget( &mStatusbarLabel );
    
    //****************************************
    // menus
    //****************************************
    connect( mMainUi->actionExit, &QAction::triggered, this, &Aures::handleExit );

    connect( mMainUi->actionSoundLevels, &QAction::triggered, this, &Aures::handleSoundLevels );

    connect( mMainUi->actionAbout, &QAction::triggered, this, &Aures::handleAbout );    

    //****************************************
    // acoustics
    //****************************************
    mAcousticsInstance = AcousticsHandler::getInstance();
    
    //****************************************
    // ALSA
    //****************************************
    mAudioCaptureThread = new AudioCaptureThread();
    
    if( mAudioCaptureThread )
    {
        mAudioCaptureThread->start();
    }

    //****************************************
    // frequency analysis
    //****************************************
    mFreqAnalysisInstance = FrequencyAnalysis::getInstance(); 
    
    //****************************************
    // acoustic raw signals
    //****************************************
    mRawSignalHandler = RawSignalHandler::getInstance();
    
    if( mRawSignalHandler )
    {
        if( mRawSignalThread )
        {
            if( mFreqAnalysisInstance )
            {
                mRawSignalHandler->setFftSize( mFreqAnalysisInstance->getFftSizeValue() );
                mRawSignalHandler->setWindowFunction( WindowFunction::WINDOW_FUNCTION_TYPE_HANN );
                mRawSignalHandler->setSigProcStartNoisePwr();
            }

            mRawSignalHandler->moveToThread( mRawSignalThread );
            connect( &mRawSignalHandler->getRnnComputeThread(), &RnnComputeThread::rnnComputeDone, this, &Aures::receiveNewRnn, Qt::QueuedConnection );

            mRawSignalThread->start();
        }

        connect( mAudioCaptureThread, SIGNAL( haveNewCaptureAudio( AudioChannelData ) ), this->mRawSignalHandler, SLOT( receiveNewCapturedAudio( AudioChannelData ) ) );
    }

    //****************************************
    // frequency band
    //****************************************
    for( const auto& i : AcousticsHandler::FREQUENCY_RANGE_VALUES )
    {
        QString fBandStr = QString::number( (i.second).min ) + " - " + QString::number( (i.second).max );
        mMainUi->FrequencyRangeComboBox->addItem( fBandStr );
    }

    mMainUi->FrequencyRangeComboBox->setCurrentIndex( mAcousticsInstance->getFrequencyRangeOptionIndex() );
    connect( mMainUi->FrequencyRangeComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedFrequencyRange(int) ) );

    //****************************************
    // noise
    //****************************************
    connect( mMainUi->ResetNoiseFloorButton, &QPushButton::clicked, this, &Aures::handleResetNoiseFloor );

    mMainUi->NoiseThdMinLabel->setText( QString::number( RawSignalHandler::NOISE_THD.minDb ) );
    mMainUi->NoiseThdMaxLabel->setText( QString::number( RawSignalHandler::NOISE_THD.maxDb ) );
    int noiseThdMin = RawSignalHandler::NOISE_THD.minDb * RawSignalHandler::NOISE_THD.scale;
    int noiseThdMax = RawSignalHandler::NOISE_THD.maxDb * RawSignalHandler::NOISE_THD.scale;
    int noiseThd = mRawSignalHandler->getNoiseThdDb() * RawSignalHandler::NOISE_THD.scale;
    mMainUi->NoiseThdSlider->setMinimum( noiseThdMin );
    mMainUi->NoiseThdSlider->setMaximum( noiseThdMax );
    mMainUi->NoiseThdSlider->setValue( noiseThd );
    mMainUi->NoiseThdValue->setText( QString::number( noiseThd / RawSignalHandler::NOISE_THD.scale, 'f', 1 ) );
    connect( mMainUi->NoiseThdSlider, SIGNAL( valueChanged(int)), this, SLOT( handleChangedNoiseThd(int) ) );

    //****************************************
    // speech
    //****************************************
    mMainUi->SpeechThdMinLabel->setText( QString::number( RawSignalHandler::SPEECH_THD.minDb ) );
    mMainUi->SpeechThdMaxLabel->setText( QString::number( RawSignalHandler::SPEECH_THD.maxDb ) );
    int speechThdMin = RawSignalHandler::SPEECH_THD.minDb * RawSignalHandler::SPEECH_THD.scale;
    int speechThdMax = RawSignalHandler::SPEECH_THD.maxDb * RawSignalHandler::SPEECH_THD.scale;
    int speechThd = mRawSignalHandler->getSpeechThdDb() * RawSignalHandler::SPEECH_THD.scale;
    mMainUi->SpeechThdSlider->setMinimum( speechThdMin );
    mMainUi->SpeechThdSlider->setMaximum( speechThdMax );
    mMainUi->SpeechThdSlider->setValue( speechThd );
    mMainUi->SpeechThdValue->setText( QString::number( speechThd / RawSignalHandler::SPEECH_THD.scale, 'f', 1 ) );
    connect( mMainUi->SpeechThdSlider, SIGNAL( valueChanged(int)), this, SLOT( handleChangedSpeechThd(int) ) );

    //****************************************
    // Overlap-Add gain
    //****************************************
    mMainUi->OverlapAddGainMinLabel->setText( QString::number( RawSignalHandler::OLA_GAIN.min ) );
    mMainUi->OverlapAddGainMaxLabel->setText( QString::number( RawSignalHandler::OLA_GAIN.max ) );
    int olaGainMin = RawSignalHandler::OLA_GAIN.min * RawSignalHandler::OLA_GAIN.scale;
    int olaGainMax = RawSignalHandler::OLA_GAIN.max * RawSignalHandler::OLA_GAIN.scale;
    int olaGain = ( olaGainMin + olaGainMax ) / 2;
    mMainUi->OverlapAddGainSlider->setMinimum( olaGainMin );
    mMainUi->OverlapAddGainSlider->setMaximum( olaGainMax );
    mMainUi->OverlapAddGainSlider->setValue( olaGain );
    mMainUi->OverlapAddGainValue->setText( QString::number( olaGain / RawSignalHandler::OLA_GAIN.scale, 'f', 2 ) );
    connect( mMainUi->OverlapAddGainSlider, SIGNAL( valueChanged(int)), this, SLOT( handleChangedOverlapAddGain(int) ) );

    //****************************************
    // surrounding speakers
    //****************************************
    mMainUi->SpeakersNrSpinBox->setRange( MultiSourceHandler::MIN_NR_OF_SPEAKERS, MultiSourceHandler::MAX_NR_OF_SPEAKERS );
    mMainUi->SpeakersNrSpinBox->setValue( mMultiSrcHndl.getNrOfSpeakers() );
    connect( mMainUi->SpeakersNrSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( handleChangedSpeakersNr(int) ) );

    mMainUi->MicBoardSpeakerRingDial->setNotchTarget( 360.0 / mMultiSrcHndl.getNrOfSpeakers() );
    int convertedFirstSpeakerAngleDeg = static_cast<int>( -mMultiSrcHndl.getFirstSpeakerAngleDeg() );
    mMainUi->MicBoardSpeakerRingDial->setValue( convertedFirstSpeakerAngleDeg );
    connect( mMainUi->MicBoardSpeakerRingDial, SIGNAL( valueChanged(int)), this, SLOT( handleChangedSpeakerRingAngle(int) ) );

    mMainUi->SpeakersAngleSpinBox->setMinimum( static_cast<int>( MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MIN ) );
    mMainUi->SpeakersAngleSpinBox->setMaximum( static_cast<int>( MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MAX ) );
    mMainUi->SpeakersAngleSpinBox->setValue( mMultiSrcHndl.getFirstSpeakerAngleDeg() );
    connect( mMainUi->SpeakersAngleSpinBox, SIGNAL( valueChanged(int)), this, SLOT( handleChangedSpeakerAngleSpinBox(int) ) );

    //****************************************
    // mic array init
    //****************************************
    mMicArrayConfigThread = new MicArrayConfigThread( this );
    connect( mMicArrayConfigThread, SIGNAL( micArrayConfigComputeDone( MicArray ) ), this, SLOT( receiveNewMicArrayConfig( MicArray ) ) );

    //****************************************
    // beampattern config parameters
    //****************************************
    mBeampatternConfig.type = Beamforming::BEAMPATTERN_TYPE_DAS;
    mBeampatternConfig.radialApodization = false;
    mBeampatternConfig.doaDeg = 0;
    mBeampatternConfig.frequency = 0;
    mBeampatternConfig.frequencyCenter = 0;
    double fMin = 0;
    double fMax = 0;
    double fC = 0;

    if( mAcousticsInstance )
    {
        AcousticsHandler::FrequencyRange fRange = mAcousticsInstance->getFrequencyRange();
        fMin = fRange.min;
        fMax = fRange.max;
        fC = 0.5 * ( fRange.min + fRange.max );

        if( fMin <= Beamforming::DEFAULT_CENTER_FREQUENCY
         && fMax >= Beamforming::DEFAULT_CENTER_FREQUENCY )
        {
            fC = Beamforming::DEFAULT_CENTER_FREQUENCY;
        }

        mBeampatternConfig.frequency = fC;
        mBeampatternConfig.frequencyCenter = fC;
    }

    mBeampatternConfig.radialApodization = false;

    //****************************************
    // beampattern controls init
    //****************************************
    mMainUi->MicBoardBeampatternDial->setNotchTarget( 360.0 );
    int convertedDoaDeg = static_cast<int>( -mBeampatternConfig.doaDeg );
    mMainUi->MicBoardBeampatternDial->setValue( convertedDoaDeg );
    connect( mMainUi->MicBoardBeampatternDial, SIGNAL( valueChanged(int)), this, SLOT( handleChangedBeampatternDoaDial(int) ) );

    for( const auto& i : Beamforming::BEAMPATTERN_TYPE_NAMES )
    {
        mMainUi->BeampatternTypeComboBox->addItem( QString::fromStdString( i.second ) );
    }

    mMainUi->BeampatternTypeComboBox->setCurrentIndex( mBeampatternConfig.type );
    connect( mMainUi->BeampatternTypeComboBox, SIGNAL( currentIndexChanged(int)), this, SLOT( handleChangedBeampatternType(int) ) );

    mMainUi->BeampatternDoaSpinBox->setMinimum( static_cast<int>( MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MIN ) );
    mMainUi->BeampatternDoaSpinBox->setMaximum( static_cast<int>( MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MAX ) );
    mMainUi->BeampatternDoaSpinBox->setValue( mBeampatternConfig.doaDeg );
    connect( mMainUi->BeampatternDoaSpinBox, SIGNAL( valueChanged(int)), this, SLOT( handleChangedBeampatternDoaSpinbox(int) ) );

    mMainUi->BeampatternFrequencySpinBox->setMinimum( fMin );
    mMainUi->BeampatternFrequencySpinBox->setMaximum( fMax );
    mMainUi->BeampatternFrequencySpinBox->setSingleStep( 1.0 );
    mMainUi->BeampatternFrequencySpinBox->setValue( mBeampatternConfig.frequency );
    connect( mMainUi->BeampatternFrequencySpinBox, SIGNAL( valueChanged(double)), this, SLOT( handleChangedBeampatternFrequency(double) ) );

    mMainUi->BeampatternFreqCenterSpinBox->setMinimum( fMin );
    mMainUi->BeampatternFreqCenterSpinBox->setMaximum( fMax );
    mMainUi->BeampatternFreqCenterSpinBox->setSingleStep( 1.0 );
    mMainUi->BeampatternFreqCenterSpinBox->setValue( mBeampatternConfig.frequencyCenter );
    mMainUi->BeampatternFreqCenterSpinBox->setEnabled( Beamforming::BEAMPATTERN_TYPE_ADAPTIVE == mBeampatternConfig.type );
    connect( mMainUi->BeampatternFreqCenterSpinBox, SIGNAL( valueChanged(double)), this, SLOT( handleChangedBeampatternFreqCenter(double) ) );

    mMainUi->BeampatternRadApodCheckBox->setEnabled( Beamforming::BEAMPATTERN_TYPE_DAS == mBeampatternConfig.type );
    mMainUi->BeampatternRadApodCheckBox->setChecked( mBeampatternConfig.radialApodization );
    connect( mMainUi->BeampatternRadApodCheckBox, SIGNAL( toggled(bool)), this, SLOT( handleChangedBeampatternRadialApodization(bool) ) );

    updateDasOnlyControls();

    mMainUi->BeampatternShowSpeakersCheckBox->setChecked( mBeampatternShowSpeakers );
    mMainUi->MicBoardBeampatternDial->setShowSpeakers( mBeampatternShowSpeakers, mMultiSrcHndl.getFirstSpeakerAngleDeg(), mMultiSrcHndl.getNrOfSpeakers() );
    connect( mMainUi->BeampatternShowSpeakersCheckBox, SIGNAL( toggled(bool)), this, SLOT( handleChangedBeampatternShowSpeakers(bool) ) );

    //****************************************
    // find optimum mic array and trigger beampattern calculation
    //****************************************
    mBeampatternThread = new BeampatternThread( this );
    connect( mBeampatternThread, SIGNAL( beampatternComputeDone( CxVector ) ), this, SLOT( receiveNewBeampattern( CxVector ) ) );

    mActiveWeightsThreadsVec = std::vector<WeightsThread>( MultiSourceHandler::MAX_NR_OF_SPEAKERS );

    for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
    {
        connect( &mActiveWeightsThreadsVec.at( crtSpeaker ), SIGNAL( weightsComputeDone( CxMatrix, int ) ), this->mRawSignalHandler, SLOT( receiveNewSpeakerWeights( CxMatrix, int ) ) );
    }

    findOptimumMicArray(); // will trigger updateActiveBeamformers()

    //****************************************
    // ASR
    //****************************************
    for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
    {
        mAsrHandlerVec.push_back( new AsrHandler( crtSpeaker, AsrModel::LANGUAGE_ENGLISH ) );
    }

    initAsrLanguageControls();

    if( mRawSignalHandler )
    {
        for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
        {
            connect( mRawSignalHandler, SIGNAL( haveNewSpeakerAudio( AudioChannelData, int ) ), mAsrHandlerVec.at( crtSpeaker ), SLOT( receiveNewSpeakerAudio( AudioChannelData, int ) ) );
        }
    }

    for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
    {
        connect( mAsrHandlerVec.at( crtSpeaker ), SIGNAL( haveNewString( QString, int ) ), this, SLOT( receiveAsrString( QString, int ) ) );
        connect( mAsrHandlerVec.at( crtSpeaker ), SIGNAL( changedRecognizer( bool ) ), this, SLOT( setSpeakerLanguagesChange( bool ) ) );
    }

    updateAsrVadControls();

    //****************************************
    // VAD
    //****************************************
    if( mRawSignalHandler )
    {
        std::vector<VoiceActivityDetection*>& vadSpeakersVec = mRawSignalHandler->getVadSpeakersVec();

        for( size_t crtSpeaker = 0; crtSpeaker < MultiSourceHandler::MAX_NR_OF_SPEAKERS; crtSpeaker++ )
        {
            connect( vadSpeakersVec.at( crtSpeaker ), &VoiceActivityDetection::haveVoiceDetectionChanged, this, &Aures::receiveNewVad, Qt::QueuedConnection );
        }
    }

    mMainUi->Vad1Label->setVisible( false );
    mMainUi->Vad2Label->setVisible( false );
    mMainUi->Vad3Label->setVisible( false );
    mMainUi->Vad4Label->setVisible( false );
    mMainUi->Vad5Label->setVisible( false );
    mMainUi->Vad6Label->setVisible( false );
    mMainUi->Vad7Label->setVisible( false );


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

    for( size_t i = 0; i < mAsrHandlerVec.size(); i++ )
    {
        delete mAsrHandlerVec.at( i );
        mAsrHandlerVec.at( i ) = nullptr;
    }

    if( mBeampatternThread )
    {
        if( mBeampatternThread->isRunning() )
        {
            mBeampatternThread->quit();
        }

        delete mBeampatternThread;
        mBeampatternThread = nullptr;
    }

    if( mMicArrayConfigThread )
    {
        if( mMicArrayConfigThread->isRunning() )
        {
            mMicArrayConfigThread->quit();
        }

        delete mMicArrayConfigThread;
        mMicArrayConfigThread = nullptr;
    }

    if( mRawSignalHandler )
    {
        if( mRawSignalThread )
        {
            if( mRawSignalThread->isRunning() )
            {
                mRawSignalThread->quit();
                mRawSignalThread->wait();
            }

            delete mRawSignalThread;
            mRawSignalThread = nullptr;
        }

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

    if( mAcousticsInstance )
    {
        mAcousticsInstance->destroyInstance();
        mAcousticsInstance = nullptr;
    }     

    delete mMainUi;
}


//!************************************************************************
//! Calculate the active weights based on the current configurations
//!
//! @returns nothing
//!************************************************************************
void Aures::calculateActiveWeights()
{
    if( mMultiSrcHndl.getNrOfSpeakers() == mActiveBeamformersConfigsVec.size() )
    {
        for( size_t crtSpeaker = 0; crtSpeaker < mMultiSrcHndl.getNrOfSpeakers(); crtSpeaker++ )
        {
            mActiveWeightsThreadsVec.at( crtSpeaker ).compute( mActiveBeamformersConfigsVec.at( crtSpeaker ), mMicArrayOptimum, crtSpeaker );
        }
    }
}


//!************************************************************************
//! Calculate the displayed beampattern based on the current configuration
//!
//! @returns nothing
//!************************************************************************
void Aures::calculateBeampattern()
{
    if( mBeampatternThread )
    {
        mBeampatternThread->compute( mBeampatternConfig, mMicArrayOptimum );
    }
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
    std::cout << "Unloading ASR data, please wait.." << std::endl << std::flush;
    QApplication::quit();
    aEvent->accept();
}


//!************************************************************************
//! Set the changing status for ASR language options for active speakers
//!
//! @returns nothing
//!************************************************************************
void Aures::setSpeakerLanguagesChange
    (
    bool aStatus    //!< status
    )
{
    uint8_t nrOfSpeakers = mMultiSrcHndl.getNrOfSpeakers();

    mMainUi->Speaker1LangComboBox->setEnabled( aStatus );

    if( nrOfSpeakers >= 2 )
    {
        mMainUi->Speaker2LangComboBox->setEnabled( aStatus );
    }

    if( nrOfSpeakers >= 3 )
    {
        mMainUi->Speaker3LangComboBox->setEnabled( aStatus );
    }

    if( nrOfSpeakers >= 4 )
    {
        mMainUi->Speaker4LangComboBox->setEnabled( aStatus );
    }

    if( nrOfSpeakers >= 5 )
    {
        mMainUi->Speaker5LangComboBox->setEnabled( aStatus );
    }

    if( nrOfSpeakers >= 6 )
    {
        mMainUi->Speaker6LangComboBox->setEnabled( aStatus );
    }

    if( nrOfSpeakers >= 7 )
    {
        mMainUi->Speaker7LangComboBox->setEnabled( aStatus );
    }
}


//!************************************************************************
//! Find an optimum mic array for a given number of speakers
//!
//! @returns nothing
//!************************************************************************
void Aures::findOptimumMicArray()
{
    if( mRawSignalHandler && mNumericInstance && mMicArrayConfigThread && mAcousticsInstance )
    {
        double angleRad = mNumericInstance->deg2Rad( mMultiSrcHndl.getFirstSpeakerAngleDeg() );
        mMicArrayConfigThread->compute( mMultiSrcHndl.getNrOfSpeakers(), angleRad, mAcousticsInstance->getFrequencyRange() );

        QApplication::setOverrideCursor( Qt::BusyCursor );
    }
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
//! Handle for changing the DoA for the shown beampattern using the dial
//! Does *NOT* affect the active beamformers.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedBeampatternDoaDial
    (
    int aValue          //!< value
    )
{
    if( aValue >= MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MIN
     && aValue <= MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MAX )
    {
        int convertedVal = -aValue;

        mBeampatternConfig.doaDeg = convertedVal;               
        mMainUi->BeampatternDoaSpinBox->setValue( convertedVal );

        calculateBeampattern();
        // do NOT calculate active beampatterns

        updateDasOnlyValues();
    }
}


//!************************************************************************
//! Handle for changing the DoA for the shown beampattern using the spinbox
//! Does *NOT* affect the active beamformers.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedBeampatternDoaSpinbox
    (
    int aValue          //!< value
    )
{
    if( aValue >= MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MIN
     && aValue <= MultiSourceHandler::FIRST_SPEAKER_ANGLE_DEG_MAX )
    {
        mBeampatternConfig.doaDeg = aValue;

        int convertedVal = -aValue;
        mMainUi->MicBoardBeampatternDial->setValue( convertedVal );

        calculateBeampattern();
        // do NOT calculate active beampatterns

        updateDasOnlyValues();
    }
}


//!************************************************************************
//! Handle for changing the frequency [Hz] for the beampattern
//! It applies to the shown beampattern *only*.
//! The active beamformers weights depend on all FFT bins.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedBeampatternFrequency
    (
    double aValue       //!< value
    )
{
    if( mAcousticsInstance )
    {
        AcousticsHandler::FrequencyRange fRange = mAcousticsInstance->getFrequencyRange();

        if( aValue >= fRange.min && aValue <= fRange.max )
        {
            mBeampatternConfig.frequency = aValue;
            updateActiveBeamformersConfig();

            calculateBeampattern();

            updateDasOnlyValues();
        }
    }
}


//!************************************************************************
//! Handle for changing the center frequency [Hz] for the beampattern
//! It applies to the shown beampattern and all active beamformers.
//! Regarding their type, it applies to adaptive *only*.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedBeampatternFreqCenter
    (
    double aValue       //!< value
    )
{
    if( mAcousticsInstance )
    {
        AcousticsHandler::FrequencyRange fRange = mAcousticsInstance->getFrequencyRange();

        if( aValue >= fRange.min && aValue <= fRange.max )
        {
            mBeampatternConfig.frequencyCenter = aValue;
            updateActiveBeamformersConfig();

            calculateBeampattern();
            calculateActiveWeights();
        }
    }
}


//!************************************************************************
//! Handle for changing the radial apodization parameter for the beampattern
//! It applies to the shown beampattern and all active beamformers.
//! Regarding their type, it applies to DAS *only*.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedBeampatternRadialApodization
    (
    bool aEnabled       //!< state
    )
{
    mBeampatternConfig.radialApodization = aEnabled;

    updateDasOnlyControls();

    updateActiveBeamformersConfig();

    calculateBeampattern();
    calculateActiveWeights();
}


//!************************************************************************
//! Handle for changing if speaker directions are shown on the beampattern
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedBeampatternShowSpeakers
    (
    bool aEnabled       //!< state
    )
{
    mBeampatternShowSpeakers = aEnabled;
    mMainUi->MicBoardBeampatternDial->setShowSpeakers( mBeampatternShowSpeakers,
                                                       mMultiSrcHndl.getFirstSpeakerAngleDeg(),
                                                       mMultiSrcHndl.getNrOfSpeakers() );
}


//!************************************************************************
//! Handle for changing the beampattern type
//! It applies to the shown beampattern and all active beamformers.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedBeampatternType
    (
    int aIndex          //!< index
    )
{
    mBeampatternConfig.type = static_cast<Beamforming::BeampatternType>( aIndex );
    updateActiveBeamformersConfig();

    mMainUi->BeampatternFreqCenterSpinBox->setEnabled( Beamforming::BEAMPATTERN_TYPE_ADAPTIVE == mBeampatternConfig.type );
    mMainUi->BeampatternRadApodCheckBox->setEnabled( Beamforming::BEAMPATTERN_TYPE_DAS == mBeampatternConfig.type );

    updateDasOnlyControls();

    calculateBeampattern();
    calculateActiveWeights();
}


//!************************************************************************
//! Handle for changing the frequency range
//! It applies to the shown beampattern and all active beamformers.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedFrequencyRange
    (
    int aIndex          //!< index
    )
{
    if( aIndex >= 0
     && aIndex < AcousticsHandler::FREQUENCY_RANGE_LAST
     && mAcousticsInstance)
    {
        if( mAcousticsInstance->setFrequencyRangeOptionIndex( static_cast<AcousticsHandler::FrequencyRangeOption>( aIndex ) ) )
        {
            prepareRedimensioning(); // due to closing findOptimumMicArray()

            AcousticsHandler::FrequencyRange fRange = mAcousticsInstance->getFrequencyRange();
            double fMin = fRange.min;
            double fMax = fRange.max;

            mMainUi->BeampatternFrequencySpinBox->setMinimum( fMin );
            mMainUi->BeampatternFrequencySpinBox->setMaximum( fMax );

            mMainUi->BeampatternFreqCenterSpinBox->setMinimum( fMin );
            mMainUi->BeampatternFreqCenterSpinBox->setMaximum( fMax );

            double fC = 0.5 * ( fRange.min + fRange.max );

            if( aIndex > AcousticsHandler::FREQUENCY_RANGE_CENTERED_4000 )
            {
                if( fMin <= Beamforming::DEFAULT_CENTER_FREQUENCY
                 && fMax >= Beamforming::DEFAULT_CENTER_FREQUENCY )
                {
                    fC = Beamforming::DEFAULT_CENTER_FREQUENCY;
                }
            }
            else
            {
                switch( aIndex )
                {
                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_500:
                        fC = 500;
                        break;

                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_750:
                        fC = 750;
                        break;

                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_1000:
                        fC = 1000;
                        break;

                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_1500:
                        fC = 1500;
                        break;

                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_2000:
                        fC = 2000;
                        break;

                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_2500:
                        fC = 2500;
                        break;

                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_3000:
                        fC = 3000;
                        break;

                    case AcousticsHandler::FREQUENCY_RANGE_CENTERED_4000:
                        fC = 4000;
                        break;

                    default:
                        break;
                }
            }

            mBeampatternConfig.frequency = fC;
            mBeampatternConfig.frequencyCenter = fC;
            updateActiveBeamformersConfig();

            mMainUi->BeampatternFrequencySpinBox->setValue( mBeampatternConfig.frequency );
            mMainUi->BeampatternFreqCenterSpinBox->setValue( mBeampatternConfig.frequencyCenter );

            // find optimum mic array and trigger beampattern calculations
            findOptimumMicArray(); // will trigger updateActiveBeamformers()
        }
    }
}


//!************************************************************************
//! Handle for changing the noise threshold
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedNoiseThd
    (
    int aValue          //!< value
    )
{
    if( mRawSignalHandler->setNoiseThd( aValue / RawSignalHandler::NOISE_THD.scale ) )
    {
        mMainUi->NoiseThdValue->setText( QString::number( aValue / RawSignalHandler::NOISE_THD.scale, 'f', 1 ) );
    }
}


//!************************************************************************
//! Handle for changing the Overlap-Add gain
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedOverlapAddGain
    (
    int aValue          //!< value
    )
{
    if( mRawSignalHandler->setOverlapAddGain( aValue / RawSignalHandler::OLA_GAIN.scale ) )
    {
        mMainUi->OverlapAddGainValue->setText( QString::number( aValue / RawSignalHandler::OLA_GAIN.scale, 'f', 2 ) );
    }
}



//!************************************************************************
//! Handle for changing the ASR language for speaker 1
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeaker1Lang
    (
    int aValue          //!< value
    )
{
    if( aValue >= 0
     && aValue < AsrModel::LANGUAGE_COUNT )
    {
        AsrModel::Language crtLanguage = mAsrHandlerVec.at( 0 )->getLanguage();
        AsrModel::Language targetLanguage = static_cast<AsrModel::Language>( aValue );

        if( targetLanguage != crtLanguage )
        {
            mMainUi->Speaker1Asr->clear();
            setSpeakerLanguagesChange( false );
            mAsrHandlerVec.at( 0 )->setLanguage( targetLanguage );
        }
    }
}

//!************************************************************************
//! Handle for changing the ASR language for speaker 2
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeaker2Lang
    (
    int aValue          //!< value
    )
{
    if( aValue >= 0
     && aValue < AsrModel::LANGUAGE_COUNT )
    {
        AsrModel::Language crtLanguage = mAsrHandlerVec.at( 1 )->getLanguage();
        AsrModel::Language targetLanguage = static_cast<AsrModel::Language>( aValue );

        if( targetLanguage != crtLanguage )
        {
            mMainUi->Speaker2Asr->clear();
            setSpeakerLanguagesChange( false );
            mAsrHandlerVec.at( 1 )->setLanguage( targetLanguage );
        }
    }
}


//!************************************************************************
//! Handle for changing the ASR language for speaker 3
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeaker3Lang
    (
    int aValue          //!< value
    )
{
    if( aValue >= 0
     && aValue < AsrModel::LANGUAGE_COUNT )
    {
        AsrModel::Language crtLanguage = mAsrHandlerVec.at( 2 )->getLanguage();
        AsrModel::Language targetLanguage = static_cast<AsrModel::Language>( aValue );

        if( targetLanguage != crtLanguage )
        {
            mMainUi->Speaker3Asr->clear();
            setSpeakerLanguagesChange( false );
            mAsrHandlerVec.at( 2 )->setLanguage( targetLanguage );
        }
    }
}


//!************************************************************************
//! Handle for changing the ASR language for speaker 4
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeaker4Lang
    (
    int aValue          //!< value
    )
{
    if( aValue >= 0
     && aValue < AsrModel::LANGUAGE_COUNT )
    {
        AsrModel::Language crtLanguage = mAsrHandlerVec.at( 3 )->getLanguage();
        AsrModel::Language targetLanguage = static_cast<AsrModel::Language>( aValue );

        if( targetLanguage != crtLanguage )
        {
            mMainUi->Speaker4Asr->clear();
            setSpeakerLanguagesChange( false );
            mAsrHandlerVec.at( 3 )->setLanguage( targetLanguage );
        }
    }
}


//!************************************************************************
//! Handle for changing the ASR language for speaker 5
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeaker5Lang
    (
    int aValue          //!< value
    )
{
    if( aValue >= 0
     && aValue < AsrModel::LANGUAGE_COUNT )
    {
        AsrModel::Language crtLanguage = mAsrHandlerVec.at( 4 )->getLanguage();
        AsrModel::Language targetLanguage = static_cast<AsrModel::Language>( aValue );

        if( targetLanguage != crtLanguage )
        {
            mMainUi->Speaker5Asr->clear();
            setSpeakerLanguagesChange( false );
            mAsrHandlerVec.at( 4 )->setLanguage( targetLanguage );
        }
    }
}


//!************************************************************************
//! Handle for changing the ASR language for speaker 6
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeaker6Lang
    (
    int aValue          //!< value
    )
{
    if( aValue >= 0
     && aValue < AsrModel::LANGUAGE_COUNT )
    {
        AsrModel::Language crtLanguage = mAsrHandlerVec.at( 5 )->getLanguage();
        AsrModel::Language targetLanguage = static_cast<AsrModel::Language>( aValue );

        if( targetLanguage != crtLanguage )
        {
            mMainUi->Speaker6Asr->clear();
            setSpeakerLanguagesChange( false );
            mAsrHandlerVec.at( 5 )->setLanguage( targetLanguage );
        }
    }
}


//!************************************************************************
//! Handle for changing the ASR language for speaker 7
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeaker7Lang
    (
    int aValue          //!< value
    )
{
    if( aValue >= 0
     && aValue < AsrModel::LANGUAGE_COUNT )
    {
        AsrModel::Language crtLanguage = mAsrHandlerVec.at( 6 )->getLanguage();
        AsrModel::Language targetLanguage = static_cast<AsrModel::Language>( aValue );

        if( targetLanguage != crtLanguage )
        {
            mMainUi->Speaker7Asr->clear();
            setSpeakerLanguagesChange( false );
            mAsrHandlerVec.at( 6 )->setLanguage( targetLanguage );
        }
    }
}


//!************************************************************************
//! Handle for changing the 1st speaker angle with spinbox
//! It applies to the shown beampattern and all active beamformers.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeakerAngleSpinBox
    (
    int aValue          //!< value
    )
{
    if( mMultiSrcHndl.setFirstSpeakerAngleDeg( aValue ) )
    {
        prepareRedimensioning(); // due to closing findOptimumMicArray()

        updateActiveBeamformersConfig();

        int convertedVal = -aValue;
        mMainUi->MicBoardSpeakerRingDial->setValue( convertedVal );
        mMainUi->MicBoardBeampatternDial->setShowSpeakers( mBeampatternShowSpeakers,
                                                           mMultiSrcHndl.getFirstSpeakerAngleDeg(),
                                                           mMultiSrcHndl.getNrOfSpeakers() );

        updateAsrSpeakerLabels();

        // find optimum mic array and trigger beampattern calculations
        findOptimumMicArray(); // will trigger updateActiveBeamformers()
    }
}


//!************************************************************************
//! Handle for changing the 1st speaker angle with ring control
//! It applies to the shown beampattern and all active beamformers.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeakerRingAngle
    (
    int aValue          //!< value
    )
{
    int convertedVal = -aValue;

    if( mMultiSrcHndl.setFirstSpeakerAngleDeg( convertedVal ) )
    {
        prepareRedimensioning(); // due to closing findOptimumMicArray()

        updateActiveBeamformersConfig();

        mMainUi->SpeakersAngleSpinBox->setValue( convertedVal );
        mMainUi->MicBoardBeampatternDial->setShowSpeakers( mBeampatternShowSpeakers,
                                                           mMultiSrcHndl.getFirstSpeakerAngleDeg(),
                                                           mMultiSrcHndl.getNrOfSpeakers() );

        updateAsrSpeakerLabels();

        // find optimum mic array and trigger beampattern calculations
        findOptimumMicArray(); // will trigger updateActiveBeamformers()
    }
}


//!************************************************************************
//! Handle for changing the number of speakers
//! It applies to the shown beampattern and all active beamformers.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeakersNr
    (
    int aNrOfSpeakers   //!< number of speakers
    )
{
    if( mMultiSrcHndl.setNrOfSpeakers( aNrOfSpeakers ) )
    {
        prepareRedimensioning(); // due to closing findOptimumMicArray()

        mMainUi->MicBoardSpeakerRingDial->setNotchTarget( 360.0 / mMultiSrcHndl.getNrOfSpeakers() );
        mMainUi->MicBoardBeampatternDial->setShowSpeakers( mBeampatternShowSpeakers,
                                                           mMultiSrcHndl.getFirstSpeakerAngleDeg(),
                                                           mMultiSrcHndl.getNrOfSpeakers() );

        // trigger updateActiveBeamformers() here due to running threads
        updateActiveBeamformers( false );

        // find optimum mic array and trigger beampattern calculations
        // it will trigger updateActiveBeamformers() a second time, which is on purpose here
        findOptimumMicArray();

        updateAsrVadControls();
    }
}


//!************************************************************************
//! Handle for changing the speech threshold
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleChangedSpeechThd
    (
    int aValue          //!< value
    )
{
    if( mRawSignalHandler->setSpeechThd( aValue / RawSignalHandler::SPEECH_THD.scale ) )
    {
        mMainUi->SpeechThdValue->setText( QString::number( aValue / RawSignalHandler::SPEECH_THD.scale, 'f', 1 ) );
    }
}


//!************************************************************************
//! Handle for exit event
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::handleExit()
{
    mStatusbarLabel.setText( " Unloading ASR data, please wait.." );
    QApplication::processEvents();
    QApplication::quit();
}


//!************************************************************************
//! Reset the noise floor as new reference
//!
//! @returns nothing
//!************************************************************************
/* slot */ void Aures::handleResetNoiseFloor()
{
    if( mRawSignalHandler )
    {
        QMessageBox msgBox( this );
        msgBox.setText( "This procedure will reset the noise level known to each microphone belonging to the array.\n\n"
                        "Continue only if during next second the audible content is at a minimum "
                        "(there is no speech, music, ambiental background noise etc).\n" );
        msgBox.setIcon( QMessageBox::Information );
        QPushButton* okButton = msgBox.addButton( "OK", QMessageBox::ActionRole );
        msgBox.addButton( QMessageBox::Abort );
        msgBox.exec();

        if( msgBox.clickedButton() == okButton)
        {
            mRawSignalHandler->setSigProcStartNoisePwr();
        }
    }
    else
    {        
        QMessageBox::warning( this, "Aures", "Cannot reset the noise level.", QMessageBox::Ok );
    }
}


//!************************************************************************
//! Sound levels window
//!
//! @returns nothing
//!************************************************************************
/* slot */ void Aures::handleSoundLevels()
{
    mMicBoardRmsUi->setupUi( &mSoundLevelsDlg );
    connect( mMicBoardRmsUi->CloseButton, SIGNAL( clicked() ), this, SLOT( handleSoundLevelsClose() ) );

    if( mRawSignalHandler )
    {
        mRawSignalHandler->setRmsWindowShown( true );
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
        mRawSignalHandler->setRmsWindowShown( false );
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

    if( mMicBoardRmsUi && mSoundLevelsDlg.isVisible() )
    {
        switch( aRmsInfo.channel )
        {
            case 1:
                mMicBoardRmsUi->mic01Value->setText( rmsStr );
                break;

            case 2:
                mMicBoardRmsUi->mic02Value->setText( rmsStr );
                break;

            case 3:
                mMicBoardRmsUi->mic03Value->setText( rmsStr );
                break;

            case 4:
                mMicBoardRmsUi->mic04Value->setText( rmsStr );
                break;

            case 5:
                mMicBoardRmsUi->mic05Value->setText( rmsStr );
                break;

            case 6:
                mMicBoardRmsUi->mic06Value->setText( rmsStr );
                break;

            case 7:
                mMicBoardRmsUi->mic07Value->setText( rmsStr );
                break;

            case 8:
                mMicBoardRmsUi->mic08Value->setText( rmsStr );
                break;

            case 9:
                mMicBoardRmsUi->mic09Value->setText( rmsStr );
                break;

            case 10:
                mMicBoardRmsUi->mic10Value->setText( rmsStr );
                break;

            case 11:
                mMicBoardRmsUi->mic11Value->setText( rmsStr );
                break;

            case 12:
                mMicBoardRmsUi->mic12Value->setText( rmsStr );
                break;

            case 13:
                mMicBoardRmsUi->mic13Value->setText( rmsStr );
                break;

            case 14:
                mMicBoardRmsUi->mic14Value->setText( rmsStr );
                break;

            case 15:
                mMicBoardRmsUi->mic15Value->setText( rmsStr );
                break;

            default:
                break;
        }
    }
}


//!************************************************************************
//! Initialize the ASR controls related to language selection
//!
//! @returns: nothing
//!************************************************************************
void Aures::initAsrLanguageControls()
{
    for( uint8_t i = 0; i < AsrModel::LANGUAGE_COUNT; i++ )
    {
        mMainUi->Speaker1LangComboBox->addItem( QString::fromStdString( AsrModel::LANGUAGE_NAMES.at( static_cast<AsrModel::Language>( i ) ) ) );
        mMainUi->Speaker2LangComboBox->addItem( QString::fromStdString( AsrModel::LANGUAGE_NAMES.at( static_cast<AsrModel::Language>( i ) ) ) );
        mMainUi->Speaker3LangComboBox->addItem( QString::fromStdString( AsrModel::LANGUAGE_NAMES.at( static_cast<AsrModel::Language>( i ) ) ) );
        mMainUi->Speaker4LangComboBox->addItem( QString::fromStdString( AsrModel::LANGUAGE_NAMES.at( static_cast<AsrModel::Language>( i ) ) ) );
        mMainUi->Speaker5LangComboBox->addItem( QString::fromStdString( AsrModel::LANGUAGE_NAMES.at( static_cast<AsrModel::Language>( i ) ) ) );
        mMainUi->Speaker6LangComboBox->addItem( QString::fromStdString( AsrModel::LANGUAGE_NAMES.at( static_cast<AsrModel::Language>( i ) ) ) );
        mMainUi->Speaker7LangComboBox->addItem( QString::fromStdString( AsrModel::LANGUAGE_NAMES.at( static_cast<AsrModel::Language>( i ) ) ) );
    }

    mMainUi->Speaker1LangComboBox->setCurrentIndex( mAsrHandlerVec.at( 0 )->getLanguage() );
    mMainUi->Speaker2LangComboBox->setCurrentIndex( mAsrHandlerVec.at( 1 )->getLanguage() );
    mMainUi->Speaker3LangComboBox->setCurrentIndex( mAsrHandlerVec.at( 2 )->getLanguage() );
    mMainUi->Speaker4LangComboBox->setCurrentIndex( mAsrHandlerVec.at( 3 )->getLanguage() );
    mMainUi->Speaker5LangComboBox->setCurrentIndex( mAsrHandlerVec.at( 4 )->getLanguage() );
    mMainUi->Speaker6LangComboBox->setCurrentIndex( mAsrHandlerVec.at( 5 )->getLanguage() );
    mMainUi->Speaker7LangComboBox->setCurrentIndex( mAsrHandlerVec.at( 6 )->getLanguage() );

    connect( mMainUi->Speaker1LangComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedSpeaker1Lang(int) ) );
    connect( mMainUi->Speaker2LangComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedSpeaker2Lang(int) ) );
    connect( mMainUi->Speaker3LangComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedSpeaker3Lang(int) ) );
    connect( mMainUi->Speaker4LangComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedSpeaker4Lang(int) ) );
    connect( mMainUi->Speaker5LangComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedSpeaker5Lang(int) ) );
    connect( mMainUi->Speaker6LangComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedSpeaker6Lang(int) ) );
    connect( mMainUi->Speaker7LangComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( handleChangedSpeaker7Lang(int) ) );
}


//!************************************************************************
//! Signalize redimensioning by stopping signal processing threads
//!
//! @returns: nothing
//!************************************************************************
void Aures::prepareRedimensioning()
{
    if( mRawSignalHandler )
    {
        mRawSignalHandler->setIsRedimensioning( true );
        std::vector<SignalProcessingWorker*> signalProcessingWorkersVec = mRawSignalHandler->getSignalProcessingWorkersVec();

        for( size_t crtMic = 0; crtMic < signalProcessingWorkersVec.size(); crtMic++ )
        {
            signalProcessingWorkersVec.at( crtMic )->setPaused( true );
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
    QString aString,    //!< ASR string
    int     aSpeakerId  //!< speakerId
    )
{
    if( aString.size() )
    {
        const uint8_t NR_SPEAKERS = mMultiSrcHndl.getNrOfSpeakers();

        if( 0 == aSpeakerId )
        {
            mMainUi->Speaker1Asr->setText( aString );
        }
        else if( 1 == aSpeakerId && NR_SPEAKERS >= 2 )
        {
            mMainUi->Speaker2Asr->setText( aString );
        }
        else if( 2 == aSpeakerId && NR_SPEAKERS >= 3 )
        {
            mMainUi->Speaker3Asr->setText( aString );
        }
        else if( 3 == aSpeakerId && NR_SPEAKERS >= 4 )
        {
            mMainUi->Speaker4Asr->setText( aString );
        }
        else if( 4 == aSpeakerId && NR_SPEAKERS >= 5 )
        {
            mMainUi->Speaker5Asr->setText( aString );
        }
        else if( 5 == aSpeakerId && NR_SPEAKERS >= 6 )
        {
            mMainUi->Speaker6Asr->setText( aString );
        }
        else if( 6 == aSpeakerId && NR_SPEAKERS >= 7 )
        {
            mMainUi->Speaker7Asr->setText( aString );
        }

#if BUILD_ROS
        if( mRosPublisher )
        {
            if( 0 == aSpeakerId )
            {
                mRosPublisher->publishMessage( formatRosMessage( aString.toStdString() ) );
            }
        }
#endif
    }
}


//!************************************************************************
//! Receive new beampattern data
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::receiveNewBeampattern
    (
    CxVector aVector                //!< beampattern values
    )
{
    mBeampatternCxValues = aVector;
    mMainUi->MicBoardBeampatternDial->updateBeampatternPlot( mBeampatternCxValues );
}


//!************************************************************************
//! Receive a new microphone array configuration
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::receiveNewMicArrayConfig
    (
    MicArray aMicArray              //!< microphone array
    )
{
    mMicArrayOptimum = aMicArray;
    mMainUi->MicBoardSpeakerRingDial->updateMicArray( mMicArrayOptimum );

    QApplication::setOverrideCursor( Qt::ArrowCursor );

    updateActiveBeamformers( true );

    calculateBeampattern();
    calculateActiveWeights();

    updateDasOnlyValues();
}


//!************************************************************************
//! Receive a new noise spectral covariance matrix
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void Aures::receiveNewRnn
    (
    const Cx3Matrix& aRnnMatrix //!< Rnn matrix
    )
{
    Q_UNUSED( aRnnMatrix );

    if( Beamforming::BEAMPATTERN_TYPE_MVDR == mBeampatternConfig.type
     || Beamforming::BEAMPATTERN_TYPE_ADAPTIVE == mBeampatternConfig.type )
    {
        calculateBeampattern();
        calculateActiveWeights();
    }
}


//!************************************************************************
//! Receive a new VAD event
//!
//! @returns: nothing
//!************************************************************************
void Aures::receiveNewVad
    (
    bool                            aIsVoice,       //!< true if voice is detected
    int                             aIndex,         //!< index
    VoiceActivityDetection::Source  aSource         //!< source
    )
{
    if( VoiceActivityDetection::SOURCE_SPEAKER == aSource )

    switch( aIndex )
    {
        case 0:
            mMainUi->Vad1Label->setVisible( aIsVoice );
            break;

        case 1:
            mMainUi->Vad2Label->setVisible( aIsVoice );
            break;

        case 2:
            mMainUi->Vad3Label->setVisible( aIsVoice );
            break;

        case 3:
            mMainUi->Vad4Label->setVisible( aIsVoice );
            break;

        case 4:
            mMainUi->Vad5Label->setVisible( aIsVoice );
            break;

        case 5:
            mMainUi->Vad6Label->setVisible( aIsVoice );
            break;

        case 6:
            mMainUi->Vad7Label->setVisible( aIsVoice );
            break;

        default:
            break;
    }
}


//!************************************************************************
//! Update the active beamformers
//! It triggers the update of their configurations.
//!
//! @returns nothing
//!************************************************************************
void Aures::updateActiveBeamformers
    (
    bool aRedimensionWeightsMatrix      //!< if redimensioning the weights matrix
    )
{
    if( mMultiSrcHndl.getNrOfSpeakers() != mActiveBeamformersConfigsVec.size() )
    {
        mActiveBeamformersConfigsVec.clear();
        mActiveBeamformersConfigsVec.resize( mMultiSrcHndl.getNrOfSpeakers() );
    }

    updateActiveBeamformersConfig();

    if( aRedimensionWeightsMatrix )
    {
        mRawSignalHandler->redimensionWeightsMatrix( mMultiSrcHndl.getNrOfSpeakers(), mMicArrayOptimum );
    }
}


//!************************************************************************
//! Update the configurations for all active beamformers
//!
//! @returns nothing
//!************************************************************************
void Aures::updateActiveBeamformersConfig()
{
    const double FIRST_SPEAKER_ANGLE_DEG = mMultiSrcHndl.getFirstSpeakerAngleDeg();
    const double ANGLE_STEP_DEG = 360.0 / mMultiSrcHndl.getNrOfSpeakers();

    for( size_t i = 0; i < mActiveBeamformersConfigsVec.size(); i++ )
    {
        mActiveBeamformersConfigsVec.at( i ) = mBeampatternConfig;
        mActiveBeamformersConfigsVec.at( i ).doaDeg = FIRST_SPEAKER_ANGLE_DEG + i * ANGLE_STEP_DEG;
    }
}


//!************************************************************************
//! Update the ASR and VAD elements
//!
//! @returns nothing
//!************************************************************************
void Aures::updateAsrVadControls()
{
    const uint8_t NR_SPEAKERS = mMultiSrcHndl.getNrOfSpeakers();

    mMainUi->Speaker2Label->setEnabled( NR_SPEAKERS >= 2 );
    mMainUi->Speaker2Asr->setEnabled( NR_SPEAKERS >= 2 );
    mMainUi->Speaker2LangComboBox->setEnabled( NR_SPEAKERS >= 2 );

    mMainUi->Speaker3Label->setEnabled( NR_SPEAKERS >= 3 );
    mMainUi->Speaker3Asr->setEnabled( NR_SPEAKERS >= 3 );
    mMainUi->Speaker3LangComboBox->setEnabled( NR_SPEAKERS >= 3 );

    mMainUi->Speaker4Label->setEnabled( NR_SPEAKERS >= 4 );
    mMainUi->Speaker4Asr->setEnabled( NR_SPEAKERS >= 4 );
    mMainUi->Speaker4LangComboBox->setEnabled( NR_SPEAKERS >= 4 );

    mMainUi->Speaker5Label->setEnabled( NR_SPEAKERS >= 5 );
    mMainUi->Speaker5Asr->setEnabled( NR_SPEAKERS >= 5 );
    mMainUi->Speaker5LangComboBox->setEnabled( NR_SPEAKERS >= 5 );

    mMainUi->Speaker6Label->setEnabled( NR_SPEAKERS >= 6 );
    mMainUi->Speaker6Asr->setEnabled( NR_SPEAKERS >= 6 );
    mMainUi->Speaker6LangComboBox->setEnabled( NR_SPEAKERS >= 6 );

    mMainUi->Speaker7Label->setEnabled( NR_SPEAKERS >= 7 );
    mMainUi->Speaker7Asr->setEnabled( NR_SPEAKERS >= 7 );
    mMainUi->Speaker7LangComboBox->setEnabled( NR_SPEAKERS >= 7 );

    updateAsrSpeakerLabels();

    const QString LABEL_ENABLED = "font-weight: bold; color: rgb(255,0,0);";
    const QString LABEL_DISABLED = "font-weight: normal;";

    const QString ASR_ENABLED = "background-color: rgb(36,31,49); color: rgb(255,255,255);";
    const QString ASR_DISABLED = "background-color: rgb(222,221,218);";

    mMainUi->Speaker1Label->setStyleSheet( LABEL_ENABLED );
    mMainUi->Speaker1Asr->setStyleSheet( ASR_ENABLED );

    if( NR_SPEAKERS >= 2 )
    {
        mMainUi->Speaker2Label->setStyleSheet( LABEL_ENABLED );
        mMainUi->Speaker2Asr->setStyleSheet( ASR_ENABLED );
    }
    else
    {
        mMainUi->Speaker2Label->setText( "Speaker 2" );
        mMainUi->Speaker2Label->setStyleSheet( LABEL_DISABLED );
        mMainUi->Speaker2Asr->setStyleSheet( ASR_DISABLED );
        mMainUi->Speaker2Asr->clear();

        mMainUi->Vad2Label->setVisible( false );
    }

    if( NR_SPEAKERS >= 3 )
    {
        mMainUi->Speaker3Label->setStyleSheet( LABEL_ENABLED );
        mMainUi->Speaker3Asr->setStyleSheet( ASR_ENABLED );
    }
    else
    {
        mMainUi->Speaker3Label->setText( "Speaker 3" );
        mMainUi->Speaker3Label->setStyleSheet( LABEL_DISABLED );
        mMainUi->Speaker3Asr->setStyleSheet( ASR_DISABLED );
        mMainUi->Speaker3Asr->clear();

        mMainUi->Vad3Label->setVisible( false );
    }

    if( NR_SPEAKERS >= 4 )
    {
        mMainUi->Speaker4Label->setStyleSheet( LABEL_ENABLED );
        mMainUi->Speaker4Asr->setStyleSheet( ASR_ENABLED );
    }
    else
    {
        mMainUi->Speaker4Label->setText( "Speaker 4" );
        mMainUi->Speaker4Label->setStyleSheet( LABEL_DISABLED );
        mMainUi->Speaker4Asr->setStyleSheet( ASR_DISABLED );
        mMainUi->Speaker4Asr->clear();

        mMainUi->Vad4Label->setVisible( false );
    }

    if( NR_SPEAKERS >= 5 )
    {
        mMainUi->Speaker5Label->setStyleSheet( LABEL_ENABLED );
        mMainUi->Speaker5Asr->setStyleSheet( ASR_ENABLED );
    }
    else
    {
        mMainUi->Speaker5Label->setText( "Speaker 5" );
        mMainUi->Speaker5Label->setStyleSheet( LABEL_DISABLED );
        mMainUi->Speaker5Asr->setStyleSheet( ASR_DISABLED );
        mMainUi->Speaker5Asr->clear();

        mMainUi->Vad5Label->setVisible( false );
    }

    if( NR_SPEAKERS >= 6 )
    {
        mMainUi->Speaker6Label->setStyleSheet( LABEL_ENABLED );
        mMainUi->Speaker6Asr->setStyleSheet( ASR_ENABLED );
    }
    else
    {
        mMainUi->Speaker6Label->setText( "Speaker 6" );
        mMainUi->Speaker6Label->setStyleSheet( LABEL_DISABLED );
        mMainUi->Speaker6Asr->setStyleSheet( ASR_DISABLED );
        mMainUi->Speaker6Asr->clear();

        mMainUi->Vad6Label->setVisible( false );
    }

    if( NR_SPEAKERS >= 7 )
    {
        mMainUi->Speaker7Label->setStyleSheet( LABEL_ENABLED );
        mMainUi->Speaker7Asr->setStyleSheet( ASR_ENABLED );
    }
    else
    {
        mMainUi->Speaker7Label->setText( "Speaker 7" );
        mMainUi->Speaker7Label->setStyleSheet( LABEL_DISABLED );
        mMainUi->Speaker7Asr->setStyleSheet( ASR_DISABLED );
        mMainUi->Speaker7Asr->clear();

        mMainUi->Vad7Label->setVisible( false );
    }
}


//!************************************************************************
//! Update the ASR labels for existing speakers
//!
//! @returns nothing
//!************************************************************************
void Aures::updateAsrSpeakerLabels()
{
    const uint8_t NR_SPEAKERS = mMultiSrcHndl.getNrOfSpeakers();
    const double FIRST_SPEAKER_ANGLE_DEG = mMultiSrcHndl.getFirstSpeakerAngleDeg();
    const double SPEAKER_ANGLE_STEP_DEG = 360.0 / NR_SPEAKERS;
    const QString DEG_SUFFIX = QString::fromUtf8( "\u00BA" );

    double angleDeg = FIRST_SPEAKER_ANGLE_DEG;
    mMainUi->Speaker1Label->setText( "Speaker 1 at " + QString::number( angleDeg ) + DEG_SUFFIX );

    if( NR_SPEAKERS >= 2 )
    {
        angleDeg += SPEAKER_ANGLE_STEP_DEG;
        angleDeg = angleDeg < 360 ? angleDeg : angleDeg - 360;
        mMainUi->Speaker2Label->setText( "Speaker 2 at " + QString::number( angleDeg ) + DEG_SUFFIX );
    }

    if( NR_SPEAKERS >= 3 )
    {
        angleDeg += SPEAKER_ANGLE_STEP_DEG;
        angleDeg = angleDeg < 360 ? angleDeg : angleDeg - 360;
        mMainUi->Speaker3Label->setText( "Speaker 3 at " + QString::number( angleDeg ) + DEG_SUFFIX );
    }

    if( NR_SPEAKERS >= 4 )
    {
        angleDeg += SPEAKER_ANGLE_STEP_DEG;
        angleDeg = angleDeg < 360 ? angleDeg : angleDeg - 360;
        mMainUi->Speaker4Label->setText( "Speaker 4 at " + QString::number( angleDeg ) + DEG_SUFFIX );
    }

    if( NR_SPEAKERS >= 5 )
    {
        angleDeg += SPEAKER_ANGLE_STEP_DEG;
        angleDeg = angleDeg < 360 ? angleDeg : angleDeg - 360;
        mMainUi->Speaker5Label->setText( "Speaker 5 at " + QString::number( angleDeg ) + DEG_SUFFIX );
    }

    if( NR_SPEAKERS >= 6 )
    {
        angleDeg += SPEAKER_ANGLE_STEP_DEG;
        angleDeg = angleDeg < 360 ? angleDeg : angleDeg - 360;
        mMainUi->Speaker6Label->setText( "Speaker 6 at " + QString::number( angleDeg ) + DEG_SUFFIX );
    }

    if( NR_SPEAKERS >= 7 )
    {
        angleDeg += SPEAKER_ANGLE_STEP_DEG;
        angleDeg = angleDeg < 360 ? angleDeg : angleDeg - 360;
        mMainUi->Speaker7Label->setText( "Speaker 7 at " + QString::number( angleDeg ) + DEG_SUFFIX );
    }
}


//!************************************************************************
//! Update the controls related to DAS-only (no radial apodization)
//!
//! @returns nothing
//!************************************************************************
void Aures::updateDasOnlyControls()
{
    bool dasOnlyEnabled = ( Beamforming::BEAMPATTERN_TYPE_DAS == mBeampatternConfig.type )
            && !mBeampatternConfig.radialApodization;
    mMainUi->DfLabel->setEnabled( dasOnlyEnabled );
    mMainUi->DfValue->setEnabled( dasOnlyEnabled );
}


//!************************************************************************
//! Update the values related to DAS-only (no radial apodization)
//! - DF (directivity factor) [deg]
//! - HPBW (half-power beamwidth) [deg]
//!
//! @returns nothing
//!************************************************************************
void Aures::updateDasOnlyValues()
{
    if( mAcousticsInstance )
    {
        double doaToMic5Rad = mNumericInstance->deg2Rad( mBeampatternConfig.doaDeg );
        double doaToMic3Rad = doaToMic5Rad - 0.5 * Numeric::PI;
        double dfRad = mAcousticsInstance->computeDirectivityFactor( mMicArrayOptimum, doaToMic3Rad, mBeampatternConfig.frequency );
        double dfDeg = mNumericInstance->rad2Deg( dfRad );
        mMainUi->DfValue->setText( QString::number( dfDeg, 'f', 1 ) + " deg" );
    }
}
