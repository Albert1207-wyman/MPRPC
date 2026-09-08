#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{


    //整个程序启动后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    //演示调用远程发布的rpc方法
    
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    //rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("林夕");
    request.set_pwd("1207");
    //rpc方法的调用
    fixbug::LoginResponse response;
    //发起rpc方法的调用 同步rpc调用过程
    stub.Login(nullptr,&request,&response,nullptr); // RpcChnnal->RpcCchannel::callMethod 集中来做所有的rpc方法调用的参数序列化和网络发送


    //一次rpc调用完成，读调用结果
    if(0==response.result().errcode())
    {
    std::cout << "rpc login response success:" << response.sucess() << std::endl;
    }
    else{
    std::cout << "rpc login response srrpr:" << response.result().errmsg() << std::endl;
    }


    //演示调用远程发布的rpc方法register
    fixbug::RegisterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("77777");
    fixbug::RegisterResponse rsp;

    //以同步的发送发起rpc调用请求，等待返回结果
    stub.Register(nullptr, &req, &rsp, nullptr);

    //一次rpc调用完成，读调用结果
    if(0==response.result().errcode())
    {
    std::cout << "rpc resgiter response success:" << rsp.sucess() << std::endl;
    }
    else{
    std::cout << "rpc register response srrpr:" << rsp.result().errmsg() << std::endl;
    }

    return 0;
}