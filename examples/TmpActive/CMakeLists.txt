cmake_minimum_required(VERSION 3.15.0)
project(tmp_active)

include_directories(${CMAKE_SOURCE_DIR}/inc)

set(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/TmpActive.cpp)

add_executable(tmp_active ${SRC_FILES})

target_link_libraries(tmp_active PUBLIC etfw)

target_include_directories(tmp_active
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set_property(TARGET tmp_active PROPERTY CXX_STANDARD 17)
