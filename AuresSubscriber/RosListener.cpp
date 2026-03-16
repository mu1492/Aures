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
RosListener.cpp

This file contains the sources for the ROS listener.
*/

#include "RosListener.h"

#include <functional>


const std::string RosListener::TOPIC_NAME = "aures_chatter";

//!************************************************************************
//! Constructor
//!************************************************************************
RosListener::RosListener()
    : Node( "aures_node" )
{
    mSubscription = this->create_subscription<std_msgs::msg::String>
            ( TOPIC_NAME, 10, std::bind( &RosListener::topicCallback, this, std::placeholders::_1 ) );
}


//!************************************************************************
//! Listener callback
//!
//! @returns nothing
//!************************************************************************
void RosListener::topicCallback
    (
    const std_msgs::msg::String::SharedPtr aMessage     //!< ROS message
    )
{
    QString msgStr = QString::fromStdString( aMessage->data );

    if( msgStr.size() )
    {
        emit haveNewString( msgStr );
    }
}
