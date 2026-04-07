FROM fedora:latest

RUN dnf install -y \
    git \
    cmake \
    gcc-c++ \
    make \
    protobuf-devel \
    grpc-devel \
    grpc-plugins \
    which \
    openssl-devel

WORKDIR /app
COPY . .
RUN protoc -I./include/Proto --grpc_out=./include/Proto --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin ./include/Proto/villas.proto && \
    protoc -I./include/Proto --cpp_out=./include/Proto ./include/Proto/villas.proto && \
    protoc -I./include/Proto --grpc_out=./include/Proto --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin ./include/Proto/FMU.proto && \
    protoc -I./include/Proto --cpp_out=./include/Proto ./include/Proto/FMU.proto

RUN cmake . && make

CMD [ "./src/asynserver" ]
#CMD [ "./src/server" ]


