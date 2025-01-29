cmake_minimum_required(VERSION 3.10)
project(Lab5)

set(CMAKE_CXX_STANDARD 11)
include_directories(${CMAKE_SOURCE_DIR})

find_package(SQLite3 REQUIRED)
if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 library not found")
endif()

find_package(Threads REQUIRED)

add_executable(main main.cpp com.hpp)
add_executable(server server.cpp com.hpp)
add_executable(simulator simulator.cpp com.hpp)

if(WIN32)
    target_link_libraries(main ws2_32 ${SQLite3_LIBRARIES} Threads::Threads)
    target_link_libraries(server ws2_32 ${SQLite3_LIBRARIES} Threads::Threads)
    target_link_libraries(simulator ws2_32 ${SQLite3_LIBRARIES} Threads::Threads)
else()
    target_link_libraries(main ${SQLite3_LIBRARIES} Threads::Threads)
    target_link_libraries(server ${SQLite3_LIBRARIES} Threads::Threads)
    target_link_libraries(simulator ${SQLite3_LIBRARIES} Threads::Threads)
endif()

set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(simulator PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_custom_target(install_python_deps ALL
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR} pip install flask matplotlib requests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Installing Python dependencies"
)
