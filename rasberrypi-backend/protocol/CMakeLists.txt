find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protocol.proto)

add_library(protocol STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(protocol ${Protobuf_LIBRARIES})

set(PROTOCOL_INC ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

