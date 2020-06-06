cmake_minimum_required(VERSION 2.8.8)

cmake_policy (SET CMP0048 NEW)
project(TestGoodCoder VERSION 1.0.0)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(Sources TestGoodCoder.cc)
add_executable(${PROJECT_NAME} ${Sources})

target_link_libraries(${PROJECT_NAME} PUBLIC gtest_main DictParser)

add_test(
    NAME ${PROJECT_NAME}
    COMMAND ${PROJECT_NAME}
    )
