cmake_minimum_required(VERSION 3.15.0)
project(runtime_executor)

include_directories(${CMAKE_SOURCE_DIR}/inc)

set(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeExecutor.cpp)

add_executable(runtime_executor ${SRC_FILES})

target_link_libraries(runtime_executor PUBLIC etfw)

target_include_directories(runtime_executor
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set_property(TARGET runtime_executor PROPERTY CXX_STANDARD 17)
