cmake_minimum_required(VERSION 3.10)

project(lab2)

add_executable(lab2 background.cpp)


target_include_directories(lab2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    target_link_libraries(lab2 Kernel32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(lab2 ${CMAKE_THREAD_LIBS_INIT})
endif()

set_target_properties(lab2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)