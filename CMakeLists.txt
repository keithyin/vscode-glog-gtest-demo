cmake_minimum_required(VERSION 2.8.8)
cmake_policy (SET CMP0048 NEW)

project(DictParser VERSION 1.0.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
enable_testing()

add_subdirectory(googletest)
add_subdirectory(glog)
add_subdirectory(test)
include_directories(include)
set(Sources main.cc)
add_library(${PROJECT_NAME}  ${Sources})

add_executable(${PROJECT_NAME}_bin ${Sources})

target_link_libraries(${PROJECT_NAME} PUBLIC glog)
target_link_libraries(${PROJECT_NAME}_bin PUBLIC glog)


