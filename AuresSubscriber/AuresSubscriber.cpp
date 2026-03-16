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
AuresSubscriber.cpp

This file contains the sources for the Aures ROS2 subscriber.
*/

#include "AuresSubscriber.h"
#include "./ui_AuresSubscriber.h"


//!************************************************************************
//! Constructor
//!************************************************************************
AuresSubscriber::AuresSubscriber
    (
    QWidget*    aParent  //!< parent widget
    )
    : QMainWindow( aParent )
    , mMainUi( new Ui::AuresSubscriber )
    , mRosWorker( new RosWorker() )
{
    mMainUi->setupUi( this );

    if( mRosWorker )
    {
        mRosWorker->moveToThread( &mRosThread );

        connect( mRosWorker->getNode().get(), SIGNAL( haveNewString( QString ) ), this, SLOT( receiveNewString( QString ) ) );

        connect( &mRosThread, &QThread::started, mRosWorker, &RosWorker::process );
        connect( &mRosThread, &QThread::finished, mRosWorker, &QObject::deleteLater );

        mRosThread.start();
    }
}


//!************************************************************************
//! Destructor
//!************************************************************************
AuresSubscriber::~AuresSubscriber()
{ 
    if( mRosThread.isRunning() )
    {
        mRosThread.quit();
        mRosThread.wait();
    }

    delete mMainUi;
}


//!************************************************************************
//! Close event handler
//!
//! @returns nothing
//!************************************************************************
void AuresSubscriber::closeEvent
    (
    QCloseEvent*    aEvent      //!< close event
    )
{
    QApplication::quit();
    aEvent->accept();
}


//!************************************************************************
//! Receive a new string
//!
//! @returns nothing
//!************************************************************************
/* slot */ void AuresSubscriber::receiveNewString
    (
    QString aString     //!< string
    )
{
    mMainUi->stringValue->setText( aString );
}
