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
RosWorker.h

This file contains the definitions for the ROS worker.
*/

#ifndef RosWorker_h
#define RosWorker_h

#include "RosListener.h"

#include <rclcpp/rclcpp.hpp>

#include <memory>

#include <QObject>


//************************************************************************
// Class for handling the ROS worker
//************************************************************************
class RosWorker : public QObject
{
    Q_OBJECT

    //************************************************************************
    // functions
    //************************************************************************
    public:
        RosWorker();

        std::shared_ptr<RosListener> getNode();

    public slots:
        void process();

        void stop();


    //************************************************************************
    // variables
    //************************************************************************
    private:
        std::shared_ptr<RosListener>                                mNode;      //!< node
        std::shared_ptr<rclcpp::executors::SingleThreadedExecutor>  mExecutor;  //!< executor
};

#endif // RosWorker_h
