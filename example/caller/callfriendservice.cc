#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"


int main(int argc, char **argv)
{


    //整个程序启动后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    //演示调用远程发布的rpc方法
    
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
    //rpc方法的请求参数
    fixbug::GetFriendsListRequest request;
    request.set_userid(1000);
    // rpc方法的调用
    fixbug::GetFriendsListResponse response;
    //发起rpc方法的调用 同步rpc调用过程
    MprpcController controller;

    stub.GetFriendsList(&controller,&request,&response,nullptr); // RpcChnnal->RpcCchannel::callMethod 集中来做所有的rpc方法调用的参数序列化和网络发送


    //一次rpc调用完成，读调用结果
    if(controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;

    }else
    {
        if(0==response.result().errcode())
    {
        std::cout << "rpc getFriendsList response success:" << std::endl;
        int size = response.friends_size();
        for (int i = 0; i < size; ++i)
        {

        std::cout << "index:" << (i + 1) << "name:" << response.friends(i) << std::endl;
    }
}
    else
    {

    std::cout << "rpc getFriendsList response srrpr:" << response.result().errmsg() << std::endl;
    }
    }
    
    return 0;
}
