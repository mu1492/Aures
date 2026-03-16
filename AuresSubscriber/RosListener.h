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
RosListener.h

This file contains the definitions for the ROS listener.
*/

#ifndef RosListener_h
#define RosListener_h

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <QObject>
#include <QString>


//************************************************************************
// Class for handling the ROS listener
//************************************************************************
class RosListener : public QObject, public rclcpp::Node
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    private:
        static const std::string TOPIC_NAME;     //!< topic name - must match the publisher's


    //************************************************************************
    // functions
    //************************************************************************
    public:
        RosListener();

    private:
        void topicCallback
            (
            const std_msgs::msg::String::SharedPtr aMessage     //!< ROS message
            );

    signals:
        void haveNewString
            (
            QString aString         //!< string
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mSubscription;   //!< subscription
};

#endif // RosListener_h
