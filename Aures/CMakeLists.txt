#########################
# cmake, CUDA, and OS requirements
#########################
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

if( NOT UNIX )
    message( FATAL_ERROR "This project is for Linux systems only." )
endif()

#########################
# Project config
#########################
set(USE_ROS OFF)

set(USE_JETSON_ORIN_NANO ON)
set(USE_JETSON_AGX_ORIN OFF)

# check if one platform was enabled
if( NOT USE_JETSON_ORIN_NANO AND NOT USE_JETSON_AGX_ORIN )
    message( FATAL_ERROR "No Jetson platform was enabled." )
elseif( USE_JETSON_ORIN_NANO AND USE_JETSON_AGX_ORIN )
    message( FATAL_ERROR "Multiple Jetson platforms are not supported." )
endif()

#########################
# Build defines
#########################
if(USE_ROS)
    add_definitions( -DBUILD_ROS=1 )
endif()

if(USE_JETSON_ORIN_NANO)
    add_definitions( -DBUILD_JETSON_ORIN_NANO=1 )
endif()

if(USE_JETSON_AGX_ORIN)
    add_definitions( -DBUILD_JETSON_AGX_ORIN=1 )
endif()

#########################
# CUDA flags
#########################
include_directories( "./CUDA" )
set(CMAKE_CUDA_COMPILER         "/usr/local/cuda/bin/nvcc")
set(CMAKE_CUDA_FLAGS            "-gencode=arch=compute_87,code=sm_87 \
                                 -gencode=arch=compute_87,code=compute_87" )
set(CMAKE_CUDA_FLAGS_DEBUG      "-g -G")
set(CMAKE_CUDA_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CUDA_FLAGS_RELEASE    "-O2 -DNDEBUG")

#########################
# Project definitions
#########################
project(Aures LANGUAGES CXX CUDA)

#########################
# Build flags
#########################
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenGL_GL_PREFERENCE LEGACY)

#########################
# Packages
#########################
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGL OpenGLExtensions)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL OpenGLExtensions)
find_package(OpenGL REQUIRED)
find_package(CUDA REQUIRED)

if(USE_ROS)
    find_package(rclcpp REQUIRED)
    find_package(std_msgs REQUIRED)
endif()

#########################
# Include paths
#########################
include_directories(SYSTEM)
include_directories(${CUDA_INCLUDE_DIRS})

if(USE_ROS)
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
endif()

link_directories( "./" )

if(USE_ROS)
    link_directories( "/opt/ros/humble/lib" )
endif()

#########################
# Project files
#########################
set(PRJ_SOURCES_LIST  )
list(APPEND PRJ_SOURCES_LIST
        main.cpp
        Aures.cpp
        Aures.h
        Aures.ui
        Aures.qrc
        About.ui
        Mic.cpp
        Mic.h
        Ics52000.cpp
        Ics52000.h
        MicArray.cpp
        MicArray.h
        AudioChannelData.h
        AudioCaptureThread.cpp
        AudioCaptureThread.h
        AsrModel.cpp
        AsrModel.h
        AsrRecognizer.cpp
        AsrRecognizer.h
        AsrHandler.cpp
        AsrHandler.h
        AsrThread.cpp
        AsrThread.h
        RawSignalHandler.cpp
        RawSignalHandler.h
        RawSignalWorker.cpp
        RawSignalWorker.h
        Numeric.cpp
        Numeric.h
        MicBoard.ui
        cuMl.cu
        cuMl.hpp
        )

if(USE_ROS)
    list(APPEND PRJ_SOURCES_LIST
        RosPublisher.cpp
        RosPublisher.h
        )
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Aures MANUAL_FINALIZATION ${PRJ_SOURCES_LIST})
else()
    add_executable(Aures ${PRJ_SOURCES_LIST})
endif()

set_target_properties(Aures PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_target_properties(Aures PROPERTIES CUDA_ARCHITECTURES "87;")

#########################
# Linker and libraries
#########################
target_compile_options(Aures PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:
                       -std=c++11
                       -m64
                       -O2
                       -Xcompiler
                       >)

set(AUDIO_LIBRARIES "libasound.so" "libvosk.so")
set(CUDA_LIBRARIES "libcuda.so" "libcudart.so" "libcublas.so" "libcusparse.so" "libcurand.so" "libcusolver.so" "libcufft.so")
if(NOT USE_ROS)
    target_link_libraries(Aures PRIVATE  Qt${QT_VERSION_MAJOR}::Widgets
                                         Qt${QT_VERSION_MAJOR}::OpenGL
                                         Qt${QT_VERSION_MAJOR}::OpenGLExtensions
                                         ${AUDIO_LIBRARIES}
                                         ${OPENGL_LIBRARIES}
                                         ${CUDA_LIBRARIES})
else()
    target_link_libraries(Aures PRIVATE  Qt${QT_VERSION_MAJOR}::Widgets
                                         Qt${QT_VERSION_MAJOR}::OpenGL
                                         Qt${QT_VERSION_MAJOR}::OpenGLExtensions
                                         ${AUDIO_LIBRARIES}
                                         ${OPENGL_LIBRARIES}
                                         ${CUDA_LIBRARIES}
                                         rcl
                                         rclcpp
                                         rcutils
                                         rmw
                                         std_msgs__rosidl_typesupport_cpp
                                         tracetools
                                         )
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Aures)
endif()
