cmake_minimum_required(VERSION 3.9)
project(AppIO VERSION 0.1 DESCRIPTION "Library for creating decoupled applications")
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_REQUIRED_FLAGS -std=c++17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17")


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config")

find_package(Boost 1.48 COMPONENTS system date_time thread chrono regex random program_options REQUIRED)
find_package(ZeroMQ 4.0 REQUIRED)
find_package(Azmq 1.0 REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

include_directories(${Boost_INCLUDE_DIRS}
        ${ZeroMQ_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}
        ${Azmq_INCLUDE_DIRS})


add_subdirectory(examples)


install(DIRECTORY ${PROJECT_SOURCE_DIR}/AppIO
        DESTINATION include)
#install(TARGETS AppIO
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})