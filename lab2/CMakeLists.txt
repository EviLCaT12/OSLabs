cmake_minimum_required(VERSION 3.5.0)

project(task2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_SRCS test.cpp)
set(LIB_SRCS background.cpp)
set(LIB_HDRS background.hpp)

add_library(background SHARED ${LIB_HDRS} ${LIB_SRCS})


add_executable(test ${APP_SRCS})
target_link_libraries(test background)


add_executable(subprogram subprogram.cpp)
add_executable(error error.cpp)
add_executable(long_task long_task.cpp)  

if (WIN32)
    set_target_properties(background PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()