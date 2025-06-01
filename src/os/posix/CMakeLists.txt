# Specify the module name based on the directory
get_filename_component(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(MODULE_NAME ${MODULE_NAME} NAME)

# Define the include directories (if they exist)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

# Collect all .cpp files in the current src directory
file(GLOB MODULE_SOURCES *.cpp)

message(MODULE_SOURCES="${MODULE_SOURCES}")

# Create a library or executable from the sources
add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES})

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
