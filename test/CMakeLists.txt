aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_LISTS)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/test)

add_executable(client performance_client.cc)
add_executable(server performance_server.cc ${SRC_LISTS})
target_link_libraries(server PRIVATE fmt spdlog muduo_net muduo_base pthread)