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
RawSignalHandler.cpp

This file contains the sources for processing raw audio signals.
*/

#include "RawSignalHandler.h"

#include <iostream>


RawSignalHandler* RawSignalHandler::sInstance = nullptr;

//!************************************************************************
//! Constructor
//!************************************************************************
RawSignalHandler::RawSignalHandler()
    : mMicArrayHandler( nullptr )
    , mMicArraySize( 0 )
{
    mMicArrayHandler = MicArray::getInstance();

    if( mMicArrayHandler )
    {
        mMicArraySize = mMicArrayHandler->getArray().size();

        for( size_t i = 0; i < mMicArraySize; i++ )
        {
            RawSignalWorker* worker = new RawSignalWorker( i );
            mRawSignalWorkersVec.push_back( worker );

            QThread* thread = new QThread();
            mRawSignalThreadsVec.push_back( thread );
        }

        for( size_t i = 0; i < mMicArraySize; i++ )
        {
            mRawSignalWorkersVec.at( i )->moveToThread( mRawSignalThreadsVec.at( i ) );

            connect( this, &RawSignalHandler::sendNewAudio, mRawSignalWorkersVec.at( i ), &RawSignalWorker::processNewAudio, Qt::QueuedConnection );
            connect( mRawSignalWorkersVec.at( i ), &RawSignalWorker::computeRmsDone, this, &RawSignalHandler::handleRms );
            connect( mRawSignalThreadsVec.at( i ), &QThread::finished, mRawSignalWorkersVec.at( i ), &QObject::deleteLater );

            mRawSignalThreadsVec.at( i )->start();
        }

        mSumRms20MsVec.resize( mMicArraySize, 0 );
        mCountRms20MsVec.resize( mMicArraySize, 0 );

        mRms020MsVec.resize( mMicArraySize, 0 );
        mRms100MsVec.resize( mMicArraySize, 0 );
        mRms500MsVec.resize( mMicArraySize, 0 );

        connect( &m20MsTimer, &QTimer::timeout, this, &RawSignalHandler::handleTimer20Ms );
        m20MsTimer.start( 20 );
    }
}


//!************************************************************************
//! Destructor
//!************************************************************************
RawSignalHandler::~RawSignalHandler()
{
    for( size_t i = 0; i < mRawSignalThreadsVec.size(); i++ )
    {
        if( mRawSignalThreadsVec.at( i )->isRunning() )
        {
            mRawSignalThreadsVec.at( i )->quit();
            mRawSignalThreadsVec.at( i )->wait();
        }
    }
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
RawSignalHandler* RawSignalHandler::getInstance()
{
    if( !sInstance )
    {
        sInstance = new RawSignalHandler;
    }

    return sInstance;
}


//!************************************************************************
//! Instance destroyer
//!
//! @returns nothing
//!************************************************************************
void RawSignalHandler::destroyInstance()
{
    delete sInstance;
    sInstance = nullptr;
}


//!************************************************************************
//! Handle the RMS [dBFS] value
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::handleRms
    (
    ChannelValueDouble aRmsInfo //!< RMS [dBFS] information
    )
{
    mCountRms20MsVec.at( aRmsInfo.channel - 1 )++;
    mSumRms20MsVec.at( aRmsInfo.channel - 1 ) += aRmsInfo.value;
}


//!************************************************************************
//! Handle the 20ms timer
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::handleTimer20Ms()
{
    static uint8_t x5Counter = 0;   // 100 ms
    static uint8_t x25Counter = 0;  // 500 ms

    static std::vector<double> sumRms100MsVec( mMicArraySize );
    static std::vector<double> sumRms500MsVec( mMicArraySize );

    //*************
    // 20 ms
    //*************
    for( size_t micIndex = 0; micIndex < mMicArraySize; micIndex++ )
    {
        mRms020MsVec.at( micIndex ) = mSumRms20MsVec.at( micIndex ) / mCountRms20MsVec.at( micIndex );
        mSumRms20MsVec.at( micIndex ) = 0;
        mCountRms20MsVec.at( micIndex ) = 0;
    }

    //*************
    // 100 ms
    //*************
    x5Counter++;

    for( size_t micIndex = 0; micIndex < mMicArraySize; micIndex++ )
    {
        sumRms100MsVec.at( micIndex ) += mRms020MsVec.at( micIndex );
    }

    if( 0 == x5Counter % 5 )
    {
        x5Counter = 0;

        for( size_t micIndex = 0; micIndex < mMicArraySize; micIndex++ )
        {
            mRms100MsVec.at( micIndex ) = sumRms100MsVec.at( micIndex ) / 5;
            sumRms100MsVec.at( micIndex ) = 0;
        }
    }

    //*************
    // 500 ms
    //*************
    x25Counter++;

    for( size_t micIndex = 0; micIndex < mMicArraySize; micIndex++ )
    {
        sumRms500MsVec.at( micIndex ) += mRms020MsVec.at( micIndex );
    }


    if( 0 == x25Counter % 25 )
    {
        x25Counter = 0;

        for( size_t micIndex = 0; micIndex < mMicArraySize; micIndex++ )
        {
            mRms500MsVec.at( micIndex ) = sumRms500MsVec.at( micIndex ) / 25;
            sumRms500MsVec.at( micIndex ) = 0;

            ChannelValueDouble rmsInfo = { static_cast<int>( micIndex + 1 ), mRms500MsVec.at( micIndex ) };
            emit sendRms( rmsInfo );
        }
    }
}


//!************************************************************************
//! Receive new data sent by the audio capture thread
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void RawSignalHandler::receiveNewAudio
    (
    AudioChannelData aData      //!< new data
    )
{
    emit sendNewAudio( aData );
}
