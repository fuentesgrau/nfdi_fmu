#include <grpcpp/impl/service_type.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/support/sync_stream.h>
#include <string>
#include <dlfcn.h>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/status.h>
#include <grpcpp/grpcpp.h>

#include "Proto/villas.pb.h"
#include "fmi3FunctionTypes.h"
#include "fmi3Functions.h"
#include "fmi3PlatformTypes.h"

#include "Proto/FMU.grpc.pb.h"
#include "Proto/FMU.pb.h"

class FMU_serviceImp final : public FMU_server::FMI_service::Service {
public:
    int FMU_Load(std::string name, std::string path);
    int FMU_QuickSetup();
    int FMU_Close();

private:
grpc::Status FMU_Start(grpc::ServerContext* context,
                       const villas::node::Message* request,
                       villas::node::Message* reply ); 
//    grpc::Status FMU_DoStep(grpc::ServerContext* context,
//                            const villas::node::Message* request,
//                            villas::node::Message* reply) override;
//    grpc::Status FMU_GetData(grpc::ServerContext* context,
//                             const villas::node::Message* request,
//                             villas::node::Message* reply) override;
//    grpc::Status FMU_SetData(grpc::ServerContext* context,
//                             const villas::node::Message* request,
//                             villas::node::Message* reply) override;

    // int GetData(villas::node::Message* ref, villas::node::Message* data);

    // int SetData(villas::node::Message* ref, villas::node::Message* data);

    void* handle;
    fmi3Instance fmu;
    fmi3InstantiateCoSimulationTYPE* fmi3InstantiateCoSimulation;
    fmi3EnterInitializationModeTYPE* fmi3EnterInitializationMode;
    fmi3ExitInitializationModeTYPE* fmi3ExitInitializationMode;
    fmi3DoStepTYPE* fmi3DoStep;
    fmi3FreeInstanceTYPE* fmi3FreeInstance;

    fmi3Float64 stoptime;
    fmi3Boolean stoptimedefined;

    fmi3Float64 stepsize;
    fmi3Float64 currenttime;

    fmi3Boolean eventHandlingNeeded;
    fmi3Boolean terminateSimulation;
    fmi3Boolean earlyReturn;
    fmi3Float64 lastSuccessfulTime;

    fmi3GetFloat64TYPE* fmi3GetFloat64;
//    fmi3SetFloat64TYPE* fmi3SetFloat64;

    std::string guid = "{5d2d8180-f7fc-4933-b4ec-7079ef246625}";

//    fmi3GetFloat32TYPE* fmi3GetFloat32;
//    fmi3SetFloat32TYPE* fmi3SetFloat32;
//
//    fmi3SetInt32TYPE* fmi3SetInt32;
//    fmi3GetInt32TYPE* fmi3GetInt32;
//
//    fmi3GetBooleanTYPE* fmi3GetBoolean;
//    fmi3SetBooleanTYPE* fmi3SetBoolean;
};


class FMU_callbackserviceImp final : public FMU_server::FMI_service::CallbackService {
public:
    int FMU_Load(std::string name, std::string path);
    int FMU_QuickSetup();
    int FMU_Close();
private:
      grpc::ServerUnaryReactor* FMU_Start(grpc::CallbackServerContext* context,
                                          const villas::node::Message* request,
                                          villas::node::Message* reply
                                       ); 
//    grpc::ServerUnaryReactor* FMU_DoStep(grpc::CallbackServerContext* context,
//                                         const villas::node::Message*  request,
//                                         villas::node::Message* reply) override;
//    grpc::ServerUnaryReactor* FMU_GetData(grpc::CallbackServerContext* context,
//                                          const villas::node::Message*  request,
//                                          villas::node::Message* reply) override;
//    grpc::ServerUnaryReactor* FMU_SetData(grpc::CallbackServerContext* context,
//                                          const villas::node::Message*  request,
//                                          villas::node::Message* reply) override;
//    grpc::ServerUnaryReactor* FMU_GetV(grpc::CallbackServerContext* context,
//                                       const villas::node::Message*  request,
//                                       villas::node::Message* reply) override;
//    grpc::ServerUnaryReactor* data_echo(grpc::CallbackServerContext* context,
//                                       const villas::node::Message*  request,
//                                       villas::node::Message* reply) override;
//    grpc::ServerUnaryReactor* FMU_GetDataRef(grpc::CallbackServerContext* context,
//                                             const FMU_server::Reference*  request,
//                                             villas::node::Message* reply) override;
    void* handle;
    fmi3Instance fmu;
    fmi3InstantiateCoSimulationTYPE* fmi3InstantiateCoSimulation;
    fmi3EnterInitializationModeTYPE* fmi3EnterInitializationMode;
    fmi3ExitInitializationModeTYPE* fmi3ExitInitializationMode;
    fmi3DoStepTYPE* fmi3DoStep;
    fmi3FreeInstanceTYPE* fmi3FreeInstance;

    fmi3Float64 stoptime;
    fmi3Boolean stoptimedefined;

    fmi3Float64 stepsize;
    fmi3Float64 currenttime;

    fmi3Boolean eventHandlingNeeded;
    fmi3Boolean terminateSimulation;
    fmi3Boolean earlyReturn;
    fmi3Float64 lastSuccessfulTime;

    fmi3GetFloat64TYPE* fmi3GetFloat64;
    std::string guid = "{5d2d8180-f7fc-4933-b4ec-7079ef246625}";
//    fmi3SetFloat64TYPE* fmi3SetFloat64;

//    fmi3GetFloat32TYPE* fmi3GetFloat32;
//    fmi3SetFloat32TYPE* fmi3SetFloat32;
//
//    fmi3SetInt32TYPE* fmi3SetInt32;
//    fmi3GetInt32TYPE* fmi3GetInt32;
//
//    fmi3GetBooleanTYPE* fmi3GetBoolean;
//    fmi3SetBooleanTYPE* fmi3SetBoolean;
};

