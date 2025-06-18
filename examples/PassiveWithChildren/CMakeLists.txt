cmake_minimum_required(VERSION 3.15.0)
project(passive_with_children)

include_directories(${CMAKE_SOURCE_DIR}/inc)

set(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/PassiveWithChildren.cpp)

add_executable(passive_with_children ${SRC_FILES})

target_link_libraries(passive_with_children PUBLIC etfw)

target_include_directories(passive_with_children
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set_property(TARGET passive_with_children PROPERTY CXX_STANDARD 17)
