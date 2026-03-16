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
RosPublisher.cpp

This file contains the sources for the ROS publisher.
*/

#include "RosPublisher.h"


const std::string RosPublisher::TOPIC_NAME = "aures_chatter";

//!************************************************************************
//! Constructor
//!************************************************************************
RosPublisher::RosPublisher()
{
    mNode = rclcpp::Node::make_shared( "aures_node" );
    mPublisher = mNode->create_publisher<std_msgs::msg::String>( TOPIC_NAME, 10 );
}


//!************************************************************************
//! Publish a ROS message
//!
//! @returns nothing
//!************************************************************************
void RosPublisher::publishMessage
    (
    const std::string& aMessage     //!< message
    )
{
    auto message = std_msgs::msg::String();
    message.data = aMessage;
    mPublisher->publish( message );
}


//!************************************************************************
//! Start actions
//!
//! @returns nothing
//!************************************************************************
void RosPublisher::start()
{
    mExecutor.add_node( mNode );
    mExecutor.spin();
}
