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
RosWorker.cpp

This file contains the sources for the ROS worker.
*/

#include "RosWorker.h"


//!************************************************************************
//! Constructor
//!************************************************************************
RosWorker::RosWorker()
{
    mNode = std::make_shared<RosListener>();
    mExecutor = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    mExecutor->add_node( mNode );
}


//!************************************************************************
//! Get the ROS node
//!
//! @returns shared pointer to the ROS node
//!************************************************************************
std::shared_ptr<RosListener> RosWorker::getNode()
{
    return mNode;
}


//!************************************************************************
//! Start the process
//!
//! @returns nothing
//!************************************************************************
void RosWorker::process()
{
    mExecutor->spin();
}


//!************************************************************************
//! Stop the process
//!
//! @returns nothing
//!************************************************************************
void RosWorker::stop()
{
    mExecutor->cancel();
}
