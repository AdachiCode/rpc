cmake_minimum_required(VERSION 3.5)
project(rpc)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wthread-safety -fsanitize=thread")
set(CMAKE_BUILD_TYPE DEBUG)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(NETLIB_INCLUDE_DIRS /home/spanoc/netlib/include)
set(NETLIB_LIB /home/spanoc/netlib/lib/libnetlib_static.a)

find_package(Protobuf REQUIRED) # cmake提供了环境变量 ${PROTOBUF_LIBRARIES}等

include_directories(${PROJECT_SOURCE_DIR} ${NETLIB_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS})

aux_source_directory(${PROJECT_SOURCE_DIR} SRCS)
# add_executable(${PROJECT_NAME} ${SRCS})
# target_link_libraries(${PROJECT_NAME} ${NETLIB_LIB} ${PROTOBUF_LIBRARIES})
# add_executable(client ${SRCS} test/login_client.cc)
# target_link_libraries(client ${NETLIB_LIB} ${PROTOBUF_LIBRARIES})

add_library(rpc_static ${SRCS})
target_link_libraries(rpc_static ${NETLIB_LIB} ${PROTOBUF_LIBRARIES})
