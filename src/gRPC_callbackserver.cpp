#include <grpcpp/support/server_callback.h>
#include <grpcpp/support/status.h>
#include <signal.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <thread>

#include "../include/gRPC_server.hpp"
#include "../include/Proto/FMU.pb.h"


int FMU_callbackserviceImp::FMU_Load(std::string name, std::string path) {
    // Load FMU library
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
    if (!fmi3InstantiateCoSimulation || !fmi3EnterInitializationMode || !fmi3ExitInitializationMode || 
        !fmi3DoStep || !fmi3FreeInstance) {
        std::cerr << "First: Failed to resolve one or more FMI functions.(1)" << std::endl;
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

int FMU_callbackserviceImp::FMU_Close() {
    // Close FMU
    if (fmu) {
        fmi3FreeInstance(fmu);
    }
    if (handle) {
        dlclose(handle);
    }
    return 0;
}

int FMU_callbackserviceImp::FMU_QuickSetup() {
    fmu = fmi3InstantiateCoSimulation(  "instance1", 
                                        guid.c_str(), 
                                        "/app/FMU/resources", 
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
        std::cerr << "callback: Failed to instantiate FMU." << std::endl;
        FMU_Close();
        return 1;
    }
    fmi3EnterInitializationMode(fmu, fmi3False, 0.0, 0.0, fmi3True, 604800);
    stoptimedefined = true;
    stoptime = 604800;
    stepsize = 100;
    currenttime = 0;
    state = fmi3ExitInitializationMode(fmu);
    if(state != fmi3OK) std::cout << "Couldn't exit init mode with error: " << state << std::endl;
    return 0;
}


grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_DoStep(grpc::CallbackServerContext* context,
                                                             const villas::node::Message*  request,
                                                             villas::node::Message* reply) {
    if (stoptimedefined && currenttime + stepsize > stoptime) {
        // reply->set_s("Stop time reach");
    } else {
        villas::node::Sample* d_s = reply->add_samples();
        d_s->set_type(villas::node::Sample::Type::Sample_Type_DATA);
        villas::node::Value* d_v = d_s->add_values();
        fmi3ValueReference r[1] = {603979776};
//        fmi3ValueReference time_vr = 268435455;
        fmi3Float64 d[1];
//        fmi3Float64 time;

        fmi3DoStep(fmu, currenttime, stepsize, fmi3True, &eventHandlingNeeded, &terminateSimulation, &earlyReturn, &lastSuccessfulTime);
        currenttime = currenttime + stepsize;
        fmi3GetFloat64(fmu, r, 1, d, 1);

        if((eventHandlingNeeded || terminateSimulation || earlyReturn)  == fmi3True) std::cout << "Error in DoStep" << std::endl;
        // reply->set_s("Perform a step");
      
    d_v->set_f(d[0]);

    }
    auto* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_GetData(grpc::CallbackServerContext* context,
                                                              const villas::node::Message*  request,
                                                              villas::node::Message* reply) {
    std::cout << "Hello from GetData... " << std::endl;
    villas::node::Sample* d_s = reply->add_samples();
    d_s->set_type(villas::node::Sample::Type::Sample_Type_DATA);
    villas::node::Value* d_v = d_s->add_values();
    fmi3ValueReference vr = 603979776;
    fmi3Float64 d;
    fmi3GetFloat64(fmu, &vr, 1, &d, 1);
      
    std::cout << "callbackserver: data: " << d << std::endl;
    d_v->set_f(d);

    auto* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}


FMU_callbackserviceImp service;

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

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

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
