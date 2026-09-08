#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

//提供了两个进程的本地方法，Login和GetFriendsList，分别用于登录和获取好友列表

class UserService : public fixbug::UserServiceRpc
 {
public:
    // 登录方法，接受用户名和密码作为参数
    bool Login(std::string name, std::string pwd) 
       {
        std::cout << "doing local service:Login: "<< std::endl;
        std::cout<<"name:"<<name<<"pwd:"<<pwd<<std::endl;
        return true;
    }

    bool Register(int id, std::string name, std::string pwd)
    {
        std::cout << "doing local service:Register: "<< std::endl;
        std::cout<<"id:"<<id<<"name:"<<name<<"pwd:"<<pwd<<std::endl;
        return true;

    }



void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
       //框架给业务上报了请求参数LoginRequest，应用获取相应数据，做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();
        //做本地业务
        bool login_result = Login(name, pwd);
        //把响应写入
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg(" ");
        response->set_sucess(login_result);
        //执行回调操作
        done->Run();
       }
       void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
{
    uint32_t id = request->id();
    std::string name = request->name();
    std::string pwd = request->pwd();

    bool ret = Register(id, name, pwd);

    response->mutable_result()->set_errcode(0);
    response->mutable_result()->set_errmsg("");
    response->set_sucess(ret);

    done->Run();
}

};



int main(int argc, char** argv) {

    //调用框架的初始化操作
    MprpcApplication::Init(argc, argv);

    //把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    provider.Run(); //启动rpc服务节点，开始提供rpc远程网络调用服务  







    return 0;
}
