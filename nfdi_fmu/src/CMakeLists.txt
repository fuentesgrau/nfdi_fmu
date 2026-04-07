add_executable(server gRPC_server.cpp)
target_link_libraries(server proto-objects)

add_executable(asynserver gRPC_callbackserver.cpp)
target_link_libraries(asynserver proto-objects)
