cmake_minimum_required(VERSION 3.14)
project(ThermoLogger)               


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS system thread chrono)

add_executable(thermo_logger thermo_logger.cpp)
add_executable(thermometr thermometr.cpp)

target_link_libraries(thermo_logger Boost::system Boost::thread Boost::chrono)
target_link_libraries(thermometr Boost::system Boost::thread)

if (WIN32)
    target_link_libraries(thermo_logger ws2_32)
    target_link_libraries(thermometr ws2_32)
endif()