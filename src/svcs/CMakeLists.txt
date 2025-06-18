
file(GLOB SOURCES *.cpp)

add_library(SVC_FW STATIC ${SOURCES})
target_link_libraries(SVC_FW PUBLIC 
    Os
    etl::etl
)

target_include_directories(SVC_FW PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIB_CLASSES}/Os
)