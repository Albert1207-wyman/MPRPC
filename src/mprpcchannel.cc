#include "mprpcchannel.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "mprpcapplication.h"
#include "mprpccontroller.h"
#include "rpcheader.pb.h"


// ======================================================
// 构造函数
// ======================================================

MprpcChannel::MprpcChannel()
    : clientfd_(-1),
      connected_(false),
      current_ip_(""),
      current_port_(0)
{
    // ==================================================
    // 连接 ZooKeeper
    // ==================================================

    zkclient_.Start();
}


// ======================================================
// 析构函数
// ======================================================

MprpcChannel::~MprpcChannel()
{
    if (clientfd_ != -1)
    {
        close(clientfd_);

        clientfd_ = -1;
    }


    connected_ = false;
}


// ======================================================
// ResolveProvider
// ======================================================

bool MprpcChannel::ResolveProvider(
    const std::string& service_name,
    const std::string& method_name,
    std::string& ip,
    uint16_t& port,
    google::protobuf::RpcController* controller)
{
    // ==================================================
    // ZooKeeper 节点路径
    //
    // /UserServiceRpc/Login
    // ==================================================

    std::string path =
        "/"
        + service_name
        + "/"
        + method_name;


    // ==================================================
    // 从 ZooKeeper 获取 Provider 地址
    //
    // 127.0.0.1:8000
    // ==================================================

    std::string server_address =
        zkclient_.GetData(
            path.c_str());


    if (server_address.empty())
    {
        controller->SetFailed(
            "service address not found in zookeeper");

        return false;
    }


    // ==================================================
    // 解析最后一个 ':'
    // ==================================================

    size_t pos =
        server_address.rfind(':');


    if (pos == std::string::npos)
    {
        controller->SetFailed(
            "invalid server address");

        return false;
    }


    ip =
        server_address.substr(
            0,
            pos);


    std::string port_str =
        server_address.substr(
            pos + 1);


    // ==================================================
    // 转换端口
    // ==================================================

    try
    {
        int parsed_port =
            std::stoi(port_str);


        if (parsed_port <= 0 ||
            parsed_port > 65535)
        {
            controller->SetFailed(
                "invalid server port");

            return false;
        }


        port =
            static_cast<uint16_t>(
                parsed_port);
    }
    catch (...)
    {
        controller->SetFailed(
            "invalid server port");

        return false;
    }


    return true;
}


// ======================================================
// Connect
// ======================================================

bool MprpcChannel::Connect(
    const std::string& ip,
    uint16_t port,
    google::protobuf::RpcController* controller)
{
    // ==================================================
    // 已经连接到相同 Provider
    // 直接复用
    // ==================================================

    if (connected_ &&
        clientfd_ != -1 &&
        current_ip_ == ip &&
        current_port_ == port)
    {
        return true;
    }


    // ==================================================
    // 地址发生变化
    // 关闭旧连接
    // ==================================================

    if (clientfd_ != -1)
    {
        close(clientfd_);

        clientfd_ = -1;
    }


    connected_ = false;


    // ==================================================
    // 创建 Socket
    // ==================================================

    clientfd_ =
        socket(
            AF_INET,
            SOCK_STREAM,
            0);


    if (clientfd_ == -1)
    {
        controller->SetFailed(
            "create socket error");

        return false;
    }


    // ==================================================
    // Server 地址
    // ==================================================

    sockaddr_in server_addr{};


    server_addr.sin_family =
        AF_INET;


    server_addr.sin_port =
        htons(port);


    server_addr.sin_addr.s_addr =
        inet_addr(ip.c_str());


    // ==================================================
    // 建立连接
    // ==================================================

    if (connect(
            clientfd_,
            reinterpret_cast<sockaddr*>(
                &server_addr),
            sizeof(server_addr)) == -1)
    {
        char errtxt[512] = {0};


        snprintf(
            errtxt,
            sizeof(errtxt),
            "connect error! errno:%d",
            errno);


        controller->SetFailed(
            errtxt);


        close(clientfd_);


        clientfd_ = -1;

        connected_ = false;


        return false;
    }


    // ==================================================
    // 连接成功
    // ==================================================

    connected_ = true;


    current_ip_ =
        ip;


    current_port_ =
        port;


    return true;
}


// ======================================================
// SendAll
// ======================================================

bool MprpcChannel::SendAll(
    const char* data,
    size_t size,
    google::protobuf::RpcController* controller)
{
    size_t sent = 0;


    while (sent < size)
    {
        ssize_t n =
            send(
                clientfd_,
                data + sent,
                size - sent,
                0);


        if (n > 0)
        {
            sent +=
                static_cast<size_t>(n);

            continue;
        }


        if (n == -1 &&
            errno == EINTR)
        {
            continue;
        }


        char errtxt[512] = {0};


        snprintf(
            errtxt,
            sizeof(errtxt),
            "send error! errno:%d",
            errno);


        controller->SetFailed(
            errtxt);


        close(clientfd_);


        clientfd_ = -1;

        connected_ = false;


        return false;
    }


    return true;
}


// ======================================================
// RecvAll
// ======================================================

bool MprpcChannel::RecvAll(
    char* data,
    size_t size,
    google::protobuf::RpcController* controller)
{
    size_t received = 0;


    while (received < size)
    {
        ssize_t n =
            recv(
                clientfd_,
                data + received,
                size - received,
                0);


        if (n > 0)
        {
            received +=
                static_cast<size_t>(n);

            continue;
        }


        if (n == 0)
        {
            controller->SetFailed(
                "server closed connection");


            close(clientfd_);


            clientfd_ = -1;

            connected_ = false;


            return false;
        }


        if (n == -1 &&
            errno == EINTR)
        {
            continue;
        }


        char errtxt[512] = {0};


        snprintf(
            errtxt,
            sizeof(errtxt),
            "recv error! errno:%d",
            errno);


        controller->SetFailed(
            errtxt);


        close(clientfd_);


        clientfd_ = -1;

        connected_ = false;


        return false;
    }


    return true;
}


// ======================================================
// CallMethod
// ======================================================

void MprpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done)
{
    // ==================================================
    // 1. 获取 Service / Method
    // ==================================================

    const google::protobuf::ServiceDescriptor*
        service_desc =
            method->service();


    std::string service_name =
        service_desc->name();


    std::string method_name =
        method->name();


    // ==================================================
    // 2. 序列化 Request
    // ==================================================

    std::string args_str;


    if (!request->SerializePartialToString(
            &args_str))
    {
        controller->SetFailed(
            "serialize request error");

        return;
    }


    uint32_t args_size =
        static_cast<uint32_t>(
            args_str.size());


    // ==================================================
    // 3. 创建 RpcHeader
    // ==================================================

    mprpc::RpcHeader rpc_header;


    rpc_header.set_service_name(
        service_name);


    rpc_header.set_method_name(
        method_name);


    rpc_header.set_args_size(
        args_size);


    std::string rpc_header_str;


    if (!rpc_header.SerializeToString(
            &rpc_header_str))
    {
        controller->SetFailed(
            "serialize rpc header error");

        return;
    }


    uint32_t header_size =
        static_cast<uint32_t>(
            rpc_header_str.size());


    // ==================================================
    // 4. 构造 RPC 数据
    //
    // [header_size]
    // [rpc_header]
    // [args]
    // ==================================================

    std::string send_rpc_str;


    send_rpc_str.append(
        reinterpret_cast<const char*>(
            &header_size),
        sizeof(header_size));


    send_rpc_str +=
        rpc_header_str;


    send_rpc_str +=
        args_str;


    // ==================================================
    // 5. ZooKeeper 服务发现
    //
    // 第一次连接：
    //
    // ZooKeeper -> Provider 地址
    //
    // 后续：
    //
    // 直接复用 TCP 长连接
    // ==================================================

    std::string ip;

    uint16_t port = 0;


    if (!connected_ ||
        clientfd_ == -1)
    {
        if (!ResolveProvider(
                service_name,
                method_name,
                ip,
                port,
                controller))
        {
            return;
        }


        if (!Connect(
                ip,
                port,
                controller))
        {
            return;
        }
    }


    // ==================================================
    // 6. 发送 RPC
    // ==================================================

    if (!SendAll(
            send_rpc_str.data(),
            send_rpc_str.size(),
            controller))
    {
        return;
    }


    // ==================================================
    // 7. 接收 response_size
    // ==================================================

    uint32_t response_size = 0;


    if (!RecvAll(
            reinterpret_cast<char*>(
                &response_size),
            sizeof(response_size),
            controller))
    {
        return;
    }


    // ==================================================
    // 8. 防止超大响应
    // ==================================================

    if (response_size >
        10 * 1024 * 1024)
    {
        controller->SetFailed(
            "response too large");


        close(clientfd_);


        clientfd_ = -1;

        connected_ = false;


        return;
    }


    // ==================================================
    // 9. 接收 Response
    // ==================================================

    std::string response_str;


    response_str.resize(
        response_size);


    if (response_size > 0)
    {
        if (!RecvAll(
                response_str.data(),
                response_size,
                controller))
        {
            return;
        }
    }


    // ==================================================
    // 10. 解析 Response
    // ==================================================

    if (!response->ParseFromString(
            response_str))
    {
        controller->SetFailed(
            "parse response error");

        return;
    }


    // ==================================================
    // 11. 长连接
    //
    // 不关闭 clientfd_
    // 下一次 RPC 继续复用
    // ==================================================
}