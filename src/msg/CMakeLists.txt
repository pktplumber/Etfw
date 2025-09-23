
file(GLOB SOURCES *.cpp)

add_library(MSG_FW STATIC ${SOURCES})
target_link_libraries(MSG_FW PUBLIC
    Os
    etl::etl
)

target_include_directories(MSG_FW PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIB_CLASSES}/Os
)
