cmake_minimum_required(VERSION 3.15.0)
project(stored_app_executor)

include_directories(${CMAKE_SOURCE_DIR}/inc)

set(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/StoredAppExecutor.cpp)

add_executable(stored_app_executor ${SRC_FILES})

target_link_libraries(stored_app_executor PUBLIC etfw)

target_include_directories(stored_app_executor
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set_property(TARGET stored_app_executor PROPERTY CXX_STANDARD 17)
