project(proxy)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/.. SRC_LIST)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

add_executable(proxy ${SRC_LIST})
