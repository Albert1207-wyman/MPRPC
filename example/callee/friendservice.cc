#include <iostream>
#include <string>
#include <vector>

#include "friend.pb.h"
#include "logger.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

class FriendService : public fixbug::FriendServiceRpc
{
public:
    // 真实业务逻辑
    std::vector<std::string> GetFriendList(uint32_t userid)
    {
        // Benchmark 阶段不要在每次 RPC 时打印日志，
        // 否则终端 IO 会严重影响 QPS。
        //
        // std::cout << "do GetFriendList service!"
        //           << userid << std::endl;

        std::vector<std::string> friends;

        friends.push_back("lin xi");
        friends.push_back("wyman");

        return friends;
    }

    // 重写 protobuf 生成的 RPC 接口
    void GetFriendsList(
        ::google::protobuf::RpcController* controller,
        const ::fixbug::GetFriendsListRequest* request,
        ::fixbug::GetFriendsListResponse* response,
        ::google::protobuf::Closure* done) override
    {
        // 获取用户 ID
        uint32_t userid =
            request->userid();

        // 执行业务逻辑
        std::vector<std::string> friendsList =
            GetFriendList(userid);

        // 设置响应状态
        response->mutable_result()
            ->set_errcode(0);

        response->mutable_result()
            ->set_errmsg("");

        // 填充好友列表
        for (const std::string& name : friendsList)
        {
            std::string* friend_name =
                response->add_friends();

            *friend_name = name;
        }

        // 通知 RPC 框架：
        // RPC 方法执行完成，可以发送响应
        if (done)
        {
            done->Run();
        }
    }
};

int main(int argc, char** argv)
{
    // 保留初始化日志
    LOG_INFO("FriendService RPC Provider starting...");

    // 初始化 MPRPC
    MprpcApplication::Init(argc, argv);

    // 创建 RPC Provider
    RpcProvider provider;

    // 把 FriendService 发布到 RPC 节点
    provider.NotifyService(
        new FriendService());

    // 启动 RPC 服务
    provider.Run();

    return 0;
}