aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)

add_library(mpzrpc ${SRC})

target_link_libraries(mpzrpc muduo_net muduo_base pthread zookeeper_mt)

