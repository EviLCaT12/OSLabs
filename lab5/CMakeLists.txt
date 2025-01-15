cmake_minimum_required(VERSION 3.14)  
project(ThermoLogger)  

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread chrono)
find_package(Threads REQUIRED)  
find_package(SQLite3 REQUIRED) 


add_executable(ThermoLogger
    thermo_logger_with_db.cpp
)

target_link_libraries(ThermoLogger
    PRIVATE
        Boost::system
        Boost::thread
        Boost::chrono
        Threads::Threads
        SQLite::SQLite3
)

if(WIN32)
    target_link_libraries(ThermoLogger PRIVATE ws2_32)
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)