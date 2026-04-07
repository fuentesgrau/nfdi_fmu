cmake_minimum_required(VERSION 3.10)

project(gRPC_FMU_server)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

add_library(proto-objects OBJECT "${CMAKE_CURRENT_LIST_DIR}/include/Proto/villas.proto" "${CMAKE_CURRENT_LIST_DIR}/include/Proto/FMU.proto")
target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf gRPC::grpc++ gRPC::grpc++_reflection)
#target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf gRPC::grpc++)

protobuf_generate(
    TARGET proto-objects 
    LABGUAGE cpp
)

protobuf_generate(
    TARGET proto-objects
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)

add_subdirectory(src)
