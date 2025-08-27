aux_source_directory(. SRC_LIST)

add_library(TcpServer SHARED ${SRC_LIST})

target_link_libraries(TcpServer PRIVATE fmt spdlog)