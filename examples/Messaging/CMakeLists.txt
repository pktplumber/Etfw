cmake_minimum_required(VERSION 3.15.0)
project(messaging_ex)

include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Messaging.cpp)

add_executable(messaging_ex ${SRC_FILES})

target_link_libraries(messaging_ex PUBLIC etfw)

target_include_directories(messaging_ex
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set_property(TARGET messaging_ex PROPERTY CXX_STANDARD 17)
