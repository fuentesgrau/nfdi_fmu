#include <grpcpp/impl/service_type.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/sync_stream.h>
#include <iostream>
#include <thread>
#include <iterator>
#include <string>
#include <memory>
#include <dlfcn.h>
#include <signal.h>
#include <chrono>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/status.h>
#include <grpcpp/grpcpp.h>

#include "../include/gRPC_server.hpp"
#include "../include/Proto/FMU.pb.h"
#include "../include/Proto/FMU.grpc.pb.h"
#include "../include/Proto/FMU.pb.h"

int FMU_serviceImp::FMU_Load(std::string name, std::string path) {
    // Load FMU library
    std::cout << "Loading FMU ... " << std::endl;

    handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load FMU shared library: " << dlerror() << std::endl;
        return -1;
    }
    fmi3InstantiateCoSimulation = (fmi3InstantiateCoSimulationTYPE*)dlsym(handle, "fmi3InstantiateCoSimulation");
    fmi3EnterInitializationMode = (fmi3EnterInitializationModeTYPE*)dlsym(handle, "fmi3EnterInitializationMode");
    fmi3ExitInitializationMode = (fmi3ExitInitializationModeTYPE*)dlsym(handle, "fmi3ExitInitializationMode");
    fmi3DoStep = (fmi3DoStepTYPE*)dlsym(handle, "fmi3DoStep");
    fmi3FreeInstance = (fmi3FreeInstanceTYPE*)dlsym(handle, "fmi3FreeInstance");
    if (!fmi3InstantiateCoSimulation) {
        std::cerr << " Failed to resolve fmi3InstantiateCoSimulation " << std::endl;
        FMU_Close();
        return -1;
    }
    if (!fmi3EnterInitializationMode) {
        std::cerr << "Failed to resolve fmi3EnterInitializationMode " << std::endl;
        FMU_Close();
        return -1;
    }
    if (!fmi3ExitInitializationMode) {
        std::cerr << "Failed to resolve fmi3ExitInitializationMode " << std::endl;
        FMU_Close();
        return -1;
    }
    if (!fmi3DoStep) {
        std::cerr << "Failed to resolve fmi3DoStep " << std::endl;
        FMU_Close();
        return -1;
    }
    if (!fmi3FreeInstance) {
        std::cerr << "Failed to resolve fmi3FreeInstance " << std::endl;
        FMU_Close();
        return -1;
    }


    fmi3GetFloat64 = (fmi3GetFloat64TYPE*)dlsym(handle, "fmi3GetFloat64");

    if (!fmi3GetFloat64) {
        std::cerr << "Second: Failed to resolve one or more FMI functions.(1)" << std::endl;
        return -1;
    }
    return 0;
}

int FMU_serviceImp::FMU_Close() {
    // Close FMU
    std::cout << "Closing FMU ... " << std::endl;
    if (fmu) {
        fmi3FreeInstance(fmu);
    }
    if (handle) {
        dlclose(handle);
    }
    return 0;
}

int FMU_serviceImp::FMU_QuickSetup() {
    std::cout << "Setting up FMU ... " << std::endl;
    fmu = fmi3InstantiateCoSimulation(  "instance1", 
                                        guid.c_str(),  //guid.c_str() its the instantiationToken
                                        NULL, 
                                        fmi3False, 
                                        fmi3True, 
                                        fmi3False, 
                                        fmi3False, 
                                        NULL, 
                                        0, 
                                        nullptr, 
                                        nullptr,
                                        nullptr);
    if (!fmu) {
        std::cerr << "server Failed to instantiate FMU." << std::endl;
        FMU_Close();
        return 1;
    }
    fmi3EnterInitializationMode(fmu, false, 0.0, 0.0, true, 10.0);
    stoptimedefined = true;
    stoptime = 100.0;
    stepsize = 1;
    currenttime = 0;
    fmi3ExitInitializationMode(fmu);
    return 0;
}

grpc::Status FMU_serviceImp::FMU_DoStep(grpc::ServerContext* context,
                                        const villas::node::Message* request,
                                        villas::node::Message* reply) {
    if (stoptimedefined && currenttime + stepsize > stoptime) {
        // reply->set_s("Stop time reach");
    } else {
        fmi3DoStep(fmu, currenttime, stepsize, fmi3True, &eventHandlingNeeded, &terminateSimulation, &earlyReturn, &lastSuccessfulTime);
        // reply->set_s("Perform a step");
    }
    return grpc::Status::OK;
};

grpc::Status FMU_serviceImp::FMU_GetData(grpc::ServerContext* context,
                                         const villas::node::Message* request,
                                         villas::node::Message* reply) {
    std::cout << "FMU_GetData" << std::endl;
    villas::node::Sample* d_s = reply->add_samples();
    d_s->set_type(villas::node::Sample::Type::Sample_Type_DATA);
    villas::node::Value* d_v = d_s->add_values();
    fmi3ValueReference r[2] = {268435455,603979776};
    fmi3Float64 d[2];
    fmi3GetFloat64(fmu, r, 2, d, 2);
    std::cout << "server: FMU time: " << d[0] << ", data: " << d[1] << std::endl;
    d_v->set_f(d[1]);
    std::cout << "Get data done" << std::endl;
    return grpc::Status::OK;
};


FMU_serviceImp service;

void my_handler(int s){
    printf("Caught signal %d\n",s);
    service.FMU_Close();
    exit(1); 
}

void RunServer() {
    // FMU_serviceImp service;
    std::string server_address("0.0.0.0:50051");
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on: " << server_address << std::endl;
    server->Wait();
}

int main() {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    std::string name = "Pe_KH2_FMU3_Linux";
    std::string path = "FMU/binaries/x86_64-linux/Pe_KH2_FMU3_Linux.so";
    service.FMU_Load(name, path);
    service.FMU_QuickSetup();
    RunServer();
    service.FMU_Close();
}

