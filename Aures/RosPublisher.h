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
RosPublisher.h

This file contains the definitions for the ROS publisher.
*/

#ifndef RosPublisher_h
#define RosPublisher_h

#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <QObject>


//************************************************************************
// Class for handling the ROS publisher
//************************************************************************
class RosPublisher : public QObject
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    private:
        static const std::string TOPIC_NAME;    //!< topic name - also used by listeners


    //************************************************************************
    // functions
    //************************************************************************
    public:
        RosPublisher();

        void publishMessage
            (
            const std::string& aMessage     //!< message
            );

    public slots:
        void start();


    //************************************************************************
    // variables
    //************************************************************************
    private:
        rclcpp::Node::SharedPtr                             mNode;          //!< ROS node
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mPublisher;     //!< ROS publisher
        rclcpp::executors::SingleThreadedExecutor           mExecutor;      //!< ROS executor
};

#endif // RosPublisher_h
