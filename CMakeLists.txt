
# ~~~~~~ Project ~~~~~~
cmake_minimum_required(VERSION 3.15)
project(etfw VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall")

# ~~~~~~ Vars ~~~~~~
set(ETFW_INC "${CMAKE_CURRENT_SOURCE_DIR}/inc/")
set(ETFW_SVC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/svcs")
set(ETFW_OSAL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/os")
set(ETFW_MSG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/msg")
set(DEFAULT_OSAL_SRC_DIR "${ETFW_OSAL_SRC}/posix")
set(ETL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/etl")
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")

# ~~~~~~ Osal option ~~~~~~
set(OSAL_SRC_DIR "" CACHE PATH "Path to the b-specific source directory to compile")
if (OSAL_SRC_DIR STREQUAL "")
    set(OSAL_SRC_DIR "${DEFAULT_OSAL_SRC_DIR}")
endif()
if (NOT EXISTS "${OSAL_SRC_DIR}")
    message(FATAL_ERROR "Provided b-specific source directory does not exist: ${OSAL_SRC_DIR}")
endif()
message(STATUS "Using OSAL dir : ${OSAL_SRC_DIR}")

# ~~~~~~ Options ~~~~~~
option(ENABLE_UNIT_TESTS "Enable building unit tests" OFF)
option(EXAMPLES "Compile examples" OFF)

# ~~~~~~ Fetch unit test content ~~~~~~

# ~~~~~~ Inc ~~~~~~
set(PUBLIC_INCLUDE_DIR "${ETFW_INC}")
include_directories(${ETL_DIR}/include)
include_directories(${PUBLIC_INCLUDE_DIR})

# ~~~~~~ Src ~~~~~~
add_subdirectory(${ETL_DIR})

file(GLOB_RECURSE OSAL_SRCS
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/os/*.cpp)

file(GLOB_RECURSE FW_SRCS
    CONFIGURE_DEPENDS
    ${ETFW_SVC_SRC}/*.cpp)

file(GLOB_RECURSE MSG_SRCS
    CONFIGURE_DEPENDS
    ${ETFW_MSG_SRC}/*.cpp)

# ~~~~~~ Targets ~~~~~~
add_library(osal_impl STATIC ${OSAL_SRCS})
target_include_directories(osal_impl PUBLIC "${PUBLIC_INCLUDE_DIR}/etfw")
target_compile_features(osal_impl PUBLIC cxx_std_17)

add_library(svcs_impl STATIC ${FW_SRCS})
target_include_directories(svcs_impl PUBLIC "${PUBLIC_INCLUDE_DIR}/etfw")
target_compile_features(svcs_impl PUBLIC cxx_std_17)

add_library(msg_impl STATIC ${MSG_SRCS})
target_include_directories(msg_impl PUBLIC "${PUBLIC_INCLUDE_DIR}/etfw")
target_compile_features(msg_impl PUBLIC cxx_std_17)

add_library(etfw STATIC)
target_sources(etfw PRIVATE
    $<TARGET_OBJECTS:svcs_impl>
    $<TARGET_OBJECTS:osal_impl>
    $<TARGET_OBJECTS:msg_impl>
)

# Public include directories
target_include_directories(etfw PUBLIC
    $<BUILD_INTERFACE:${PUBLIC_INCLUDE_DIR}/etfw>
)

target_link_libraries(etfw
    PUBLIC etl::etl)

# Set C++ standard
target_compile_features(etfw PUBLIC cxx_std_17)

# Add tests if enabled
if(ENABLE_UNIT_TESTS)
    message(STATUS "Adding unit tests")
    # Add test executable to build
    enable_testing()
    add_subdirectory(${TEST_DIR})
endif()

# Compile examples if included
if(EXAMPLES)
    message(STATUS "Compiling example executables")
    add_subdirectory(examples)
endif()
