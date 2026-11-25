cmake_minimum_required(VERSION 3.16)
project(calibracion_stereo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)

add_executable(calibracion_stereo calibracion_stereo.cpp)
target_link_libraries(calibracion_stereo PRIVATE ${OpenCV_LIBS})
