cmake_minimum_required(VERSION 3.15.0)
project(active_with_children)

include_directories(${CMAKE_SOURCE_DIR}/inc)

set(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/ActiveWithChildren.cpp)

add_executable(active_with_children ${SRC_FILES})

target_link_libraries(active_with_children PUBLIC etfw)

target_include_directories(active_with_children
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set_property(TARGET active_with_children PROPERTY CXX_STANDARD 17)
