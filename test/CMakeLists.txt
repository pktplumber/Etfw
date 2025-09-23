
# Fetch UT framework
message(STATUS "Fetching googletest framework")
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)
message(STATUS "Successfully pulled googletest")

# Gather test sources
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(etfw_tests ${TEST_SOURCES})

target_link_libraries(
    etfw_tests
    GTest::gtest_main
    etfw
)

include(GoogleTest)
gtest_discover_tests(etfw_tests)
