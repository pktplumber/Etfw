
# ~~~~~~ Project ~~~~~~
cmake_minimum_required(VERSION 3.15)
project(etfw VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ~~~~~~ Vars ~~~~~~
set(ETFW_INC "${CMAKE_SOURCE_DIR}/inc/")
set(ETFW_SVC_SRC "${CMAKE_SOURCE_DIR}/src/svcs")
set(ETFW_OSAL_SRC "${CMAKE_SOURCE_DIR}/src/os")
set(DEFAULT_OSAL_SRC_DIR "${ETFW_OSAL_SRC}/posix")
set(ETL_DIR "${CMAKE_SOURCE_DIR}/deps/etl")

# ~~~~~~ Osal option ~~~~~~
set(OSAL_SRC_DIR "" CACHE PATH "Path to the b-specific source directory to compile")
if (OSAL_SRC_DIR STREQUAL "")
    set(OSAL_SRC_DIR "${DEFAULT_OSAL_SRC_DIR}")
endif()
if (NOT EXISTS "${OSAL_SRC_DIR}")
    message(FATAL_ERROR "Provided b-specific source directory does not exist: ${OSAL_SRC_DIR}")
endif()
message(STATUS "Using OSAL dir : ${OSAL_SRC_DIR}")

# ~~~~~~ Inc ~~~~~~
set(PUBLIC_INCLUDE_DIR "${ETFW_INC}")
include_directories(${ETL_DIR}/include)

# ~~~~~~ Src ~~~~~~
add_subdirectory(${ETL_DIR})

file(GLOB_RECURSE OSAL_SRCS
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/os/*.cpp)

file(GLOB_RECURSE FW_SRCS
    CONFIGURE_DEPENDS
    ${ETFW_SVC_SRC}/*.cpp)

# ~~~~~~ Targets ~~~~~~
add_library(osal_impl STATIC ${OSAL_SRCS})
target_include_directories(osal_impl PUBLIC ${PUBLIC_INCLUDE_DIR}/etfw)
target_compile_features(osal_impl PUBLIC cxx_std_17)

add_library(svcs_impl STATIC ${FW_SRCS})
target_include_directories(svcs_impl PUBLIC ${PUBLIC_INCLUDE_DIR}/etfw)
#target_link_libraries(svcs_impl PRIVATE osal_impl)
target_compile_features(svcs_impl PUBLIC cxx_std_17)

add_library(etfw STATIC)
target_sources(etfw PRIVATE
    $<TARGET_OBJECTS:svcs_impl>
    $<TARGET_OBJECTS:osal_impl>
)
#target_link_libraries(etfw
#    PUBLIC etfw_impl
#    PUBLIC osal_impl
#)

# Public include directories
target_include_directories(etfw PUBLIC
    $<BUILD_INTERFACE:${PUBLIC_INCLUDE_DIR}>
)

# Set C++ standard
target_compile_features(etfw PUBLIC cxx_std_17)
