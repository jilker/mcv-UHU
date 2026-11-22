cmake_minimum_required(VERSION 3.16)
project(marcador CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)

add_executable(marcador marcador.cpp)
target_link_libraries(marcador PRIVATE ${OpenCV_LIBS})
