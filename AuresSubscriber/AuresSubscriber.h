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
AuresSubscriber.h

This file contains the definitions for the Aures ROS2 subscriber.
*/

#ifndef AuresSubscriber_h
#define AuresSubscriber_h

#include "RosWorker.h"

#include <memory>

#include <QCloseEvent>
#include <QMainWindow>
#include <QThread>

QT_BEGIN_NAMESPACE
    namespace Ui
    {
        class AuresSubscriber;
    }
QT_END_NAMESPACE


//************************************************************************
// Class for handling the Aures ROS subscriber
//************************************************************************
class AuresSubscriber : public QMainWindow
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        AuresSubscriber
            (
            QWidget *aParent = nullptr  //!< parent widget
            );

        ~AuresSubscriber();

    protected:
        void closeEvent
            (
            QCloseEvent* aEvent         //!< close event
            );

    private slots:
        void receiveNewString
            (
            QString aString             //!< string
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        Ui::AuresSubscriber*    mMainUi;        //!< main UI

        RosWorker*              mRosWorker;     //!< ROS worker
        QThread                 mRosThread;     //!< ROS thread
};

#endif // AuresSubscriber_h
