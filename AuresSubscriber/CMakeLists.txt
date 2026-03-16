cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(AuresSubscriber LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(libstatistics_collector REQUIRED)
find_package(statistics_msgs REQUIRED)

include_directories(SYSTEM)
include_directories( "/opt/ros/humble/include/builtin_interfaces" )
include_directories( "/opt/ros/humble/include/libstatistics_collector" )
include_directories( "/opt/ros/humble/include/rcl" )
include_directories( "/opt/ros/humble/include/rcl_interfaces" )
include_directories( "/opt/ros/humble/include/rcl_yaml_param_parser" )
include_directories( "/opt/ros/humble/include/rclcpp" )
include_directories( "/opt/ros/humble/include/rcpputils" )
include_directories( "/opt/ros/humble/include/rcutils" )
include_directories( "/opt/ros/humble/include/rmw" )
include_directories( "/opt/ros/humble/include/rosidl_runtime_c" )
include_directories( "/opt/ros/humble/include/rosidl_runtime_cpp" )
include_directories( "/opt/ros/humble/include/rosidl_typesupport_interface" )
include_directories( "/opt/ros/humble/include/statistics_msgs" )
include_directories( "/opt/ros/humble/include/tracetools" )
include_directories( "/opt/ros/humble/include/std_msgs" )

link_directories( "/opt/ros/humble/lib" )

set(PRJ_SOURCES_LIST
        main.cpp
        AuresSubscriber.cpp
        AuresSubscriber.h
        AuresSubscriber.ui
        RosListener.cpp
        RosListener.h
        RosWorker.cpp
        RosWorker.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AuresSubscriber MANUAL_FINALIZATION ${PRJ_SOURCES_LIST})
else()
    add_executable(AuresSubscriber ${PRJ_SOURCES_LIST})
endif()

target_link_libraries(AuresSubscriber PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
                                              rcl
                                              rclcpp
                                              rcutils
                                              rmw
                                              std_msgs__rosidl_typesupport_cpp
                                              tracetools
                                              libstatistics_collector
                                              statistics_msgs__rosidl_typesupport_cpp
                                              )

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AuresSubscriber)
endif()
