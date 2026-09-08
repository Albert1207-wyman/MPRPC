// example/callee/echoservice.cc
#include "echo.pb.h"
#include <google/protobuf/stubs/common.h>
#include <string>

class EchoService : public fixbug::EchoService {
public:
    void Echo(::google::protobuf::RpcController* controller,
              const ::fixbug::EchoRequest* request,
              ::fixbug::EchoResponse* response,
              ::google::protobuf::Closure* done) override {
        // 纯内存拷贝，零业务开销，零日志
        response->set_payload(request->payload());
        if (done) done->Run();
    }
};