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
        std::cerr << "callback: Failed to instantiate FMU." << std::endl;
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


grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_Start(grpc::CallbackServerContext* context,
                                                            const villas::node::Message* request,
                                                            villas::node::Message* reply) {
    std::cout << "Starting FMU ... " << std::endl;
    while (currenttime < stoptime) {
        fmi3DoStep(fmu, currenttime, stepsize, fmi3True, &eventHandlingNeeded, &terminateSimulation, &earlyReturn, &lastSuccessfulTime);
        // Get data in this case h, v;
        double data;
        fmi3ValueReference r[1] = {1};
        fmi3GetFloat64(fmu,r, 1, &data, 1);
        std::cout << "time: " << currenttime << ", data: " << data << std::endl;

        villas::node::Sample* d_s = reply->add_samples();
        d_s->set_type(villas::node::Sample::Type::Sample_Type_DATA);

        villas::node::Value* reply1 = d_s->add_values();
        reply1->set_f(data);
        r[0] = 3;
        fmi3GetFloat64(fmu,r, 1, &data, 1);
        villas::node::Value* reply2 = d_s->add_values();
        reply2->set_f(data);
        currenttime += stepsize;
        // Perform step every 500 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    auto* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}
//grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_DoStep(grpc::CallbackServerContext* context,
//                                                             const villas::node::Message*  request,
//                                                             villas::node::Message* reply) {
//    if (stoptimedefined && currenttime + stepsize > stoptime) {
//        // reply->set_s("Stop time reach");
//    } else {
//        fmi3DoStep(fmu, currenttime, stepsize, fmi3True, &eventHandlingNeeded, &terminateSimulation, &earlyReturn, &lastSuccessfulTime);
//        // reply->set_s("Perform a step");
//    }
//    auto* reactor = context->DefaultReactor();
//    reactor->Finish(grpc::Status::OK);
//    return reactor;
//}

//grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_GetData(grpc::CallbackServerContext* context,
//                                                              const villas::node::Message*  request,
//                                                              villas::node::Message* reply) {
//    // villas::node::Sample s = request->samples(0);
//    villas::node::Sample* d_s = reply->add_samples();
//    // auto* ts = new villas::node::Timestamp;
//    // ts->set_nsec(s.ts_origin().nsec());
//    // ts->set_sec(s.ts_origin().sec());
//    // d_s->set_allocated_ts_origin(ts);
//    d_s->set_type(villas::node::Sample::Type::Sample_Type_DATA);
//    for (unsigned int i = 48; i <= 53; i++) {
//        villas::node::Value* d_v = d_s->add_values();
//        fmi3ValueReference r[1] = {i};
//        fmi3Float64 d;
//        fmi3GetFloat64(fmu, r, 1, &d, 1);
//        d_v->set_f(d);
//    }
//    auto* reactor = context->DefaultReactor();
//    reactor->Finish(grpc::Status::OK);
//    return reactor;
//}

//grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_SetData(grpc::CallbackServerContext* context,
//                                                              const villas::node::Message* request,
//                                                              villas::node::Message* reply) {
//    villas::node::Sample data = request->samples(0);
//    for (int i = 0; i < data.values_size(); i++) {
//        fmi3Float64 d;
//        villas::node::Value d_v = data.values(i);
//        fmi3ValueReference r[1] = {(unsigned int)i};
//        double f = d_v.f();
//        fmi3SetFloat64(fmu, r, 1, &f, 1);
//    }
//    auto* reactor = context->DefaultReactor();
//    reactor->Finish(grpc::Status::OK);
//    return reactor;
//}

//grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_GetV(grpc::CallbackServerContext* context,
//                                                           const villas::node::Message* request,
//                                                           villas::node::Message* reply) {
//    villas::node::Sample s = request->samples(0);
//    villas::node::Sample* d_s = reply->add_samples();
//    auto* ts = new villas::node::Timestamp;
//    ts->set_nsec(s.ts_origin().nsec());
//    ts->set_sec(s.ts_origin().sec());
//    d_s->set_allocated_ts_origin(ts);
//    d_s->set_type(villas::node::Sample::Type::Sample_Type_DATA);
//    for (unsigned int i = 0; i <= 47; i++) {
//        villas::node::Value* d_v = d_s->add_values();
//        fmi3ValueReference r[1] = {i};
//        fmi3Float64 d;
//        fmi3GetFloat64(fmu, r, 1, &d, 1);
//        d_v->set_f(d);
//    }
//    auto* reactor = context->DefaultReactor();
//    reactor->Finish(grpc::Status::OK);
//    return reactor;
//}

//grpc::ServerUnaryReactor* FMU_callbackserviceImp::FMU_GetDataRef(grpc::CallbackServerContext* context,
//                                                                 const FMU_server::Reference*  request,
//                                                                 villas::node::Message* reply) {
//    // villas::node::Sample s = request->samples(0);
//    villas::node::Sample* d_s = reply->add_samples();
//    // auto* ts = new villas::node::Timestamp;
//    // ts->set_nsec(s.ts_origin().nsec());
//    // ts->set_sec(s.ts_origin().sec());
//    // d_s->set_allocated_ts_origin(ts);
//    d_s->set_type(villas::node::Sample::Type::Sample_Type_DATA);
//    // for (unsigned int i = 48; i <= 53; i++) {
//    //     villas::node::Value* d_v = d_s->add_values();
//    //     fmi3ValueReference r[1] = {i};
//    //     fmi3Float64 d;
//    //     fmi3GetFloat64(fmu, r, 1, &d, 1);
//    //     d_v->set_f(d);
//    // }
//    for (int i = 0; i < request->ref_size(); i++) {
//        villas::node::Value* d_v = d_s->add_values();
//        fmi3ValueReference r[1] = {(unsigned int)request->ref(i)};
//        fmi3Float64 d;
//        fmi3GetFloat64(fmu, r, 1, &d, 1);
//        d_v->set_f(d);
//    }
//    // villas::node::Value* d_v = d_s->add_values();
//    // fmi3ValueReference r[1] = {(unsigned int)request->ref()};
//    // fmi3Float64 d;
//    // fmi3GetFloat64(fmu, r, 1, &d, 1);
//    // d_v->set_f(d);
//    auto* reactor = context->DefaultReactor();
//    reactor->Finish(grpc::Status::OK);
//    return reactor;
//}

//grpc::ServerUnaryReactor* FMU_callbackserviceImp::data_echo(grpc::CallbackServerContext* context,
//                                                            const villas::node::Message* request,
//                                                            villas::node::Message* reply) {
//    reply->CopyFrom(*request);
//    std::cout << "Get request: " << request->samples(0).values(0).f() << std::endl;
//    std::cout << "Set reply: " << reply->samples(0).values(0).f() << std::endl;
//    auto* reactor = context->DefaultReactor();
//    reactor->Finish(grpc::Status::OK);
//    return reactor;
//}

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
