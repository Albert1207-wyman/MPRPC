#include "rpcprovider.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>

#include "logger.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"


// ======================================================
// NotifyService
// ======================================================

void RpcProvider::NotifyService(
    google::protobuf::Service* service)
{
    ServiceInfo service_info;


    // ==================================================
    // 获取 Service 描述
    // ==================================================

    const google::protobuf::ServiceDescriptor*
        service_desc =
            service->GetDescriptor();


    std::string service_name =
        service_desc->name();


    std::cout
        << "serviceName: "
        << service_name
        << std::endl;


    LOG_INFO(
        "service_name:%s",
        service_name.c_str());


    // ==================================================
    // 获取方法数量
    // ==================================================

    int method_count =
        service_desc->method_count();


    // ==================================================
    // 注册所有 RPC 方法
    // ==================================================

    for (int i = 0;
         i < method_count;
         ++i)
    {
        const google::protobuf::MethodDescriptor*
            method_desc =
                service_desc->method(i);


        std::string method_name =
            method_desc->name();


        std::cout
            << "methodName: "
            << method_name
            << std::endl;


        service_info.m_methodMap.insert(
            {
                method_name,
                method_desc
            });


        LOG_INFO(
            "method_name:%s",
            method_name.c_str());
    }


    // ==================================================
    // 保存 Service 对象
    // ==================================================

    service_info.m_service =
        service;


    // ==================================================
    // 保存到 Service Map
    // ==================================================

    m_serviceMap.insert(
        {
            service_name,
            service_info
        });
}


// ======================================================
// RegisterServiceToZk
// ======================================================

void RpcProvider::RegisterServiceToZk(
    const std::string& ip,
    uint16_t port)
{
    // ==================================================
    // Provider 地址
    //
    // 例如：
    //
    // 127.0.0.1:8000
    // ==================================================

    std::string server_address =
        ip
        + ":"
        + std::to_string(port);


    // ==================================================
    // 遍历所有 Service
    // ==================================================

    for (auto service_it =
             m_serviceMap.begin();
         service_it != m_serviceMap.end();
         ++service_it)
    {
        const std::string& service_name =
            service_it->first;


        ServiceInfo& service_info =
            service_it->second;


        // ==================================================
        // 创建 Service 节点
        //
        // /UserServiceRpc
        // ==================================================

        std::string service_path =
            "/"
            + service_name;


        m_zkClient.Create(
            service_path.c_str(),
            "",
            0,
            0);


        // ==================================================
        // 创建 Method 节点
        //
        // /UserServiceRpc/Login
        //
        // 节点数据：
        //
        // 127.0.0.1:8000
        // ==================================================

        for (auto method_it =
                 service_info.m_methodMap.begin();
             method_it != service_info.m_methodMap.end();
             ++method_it)
        {
            const std::string& method_name =
                method_it->first;


            std::string method_path =
                service_path
                + "/"
                + method_name;


            m_zkClient.Create(
                method_path.c_str(),
                server_address.c_str(),
                static_cast<int>(
                    server_address.size()),
                ZOO_EPHEMERAL);
        }
    }
}


// ======================================================
// Run
// ======================================================

void RpcProvider::Run()
{
    // ==================================================
    // 1. 获取 RPC Server IP
    // ==================================================

    std::string ip =
        MprpcApplication::GetInstance()
            .GetConfig()
            .Load("rpcserverip");


    // ==================================================
    // 2. 获取 RPC Server Port
    // ==================================================

    uint16_t port =
        static_cast<uint16_t>(
            atoi(
                MprpcApplication::GetInstance()
                    .GetConfig()
                    .Load("rpcserverport")
                    .c_str()));


    // ==================================================
    // 3. 连接 ZooKeeper
    // ==================================================

    m_zkClient.Start();


    // ==================================================
    // 4. 注册 Service
    // ==================================================

    RegisterServiceToZk(
        ip,
        port);


    // ==================================================
    // 5. 创建 Muduo TcpServer
    // ==================================================

    muduo::net::InetAddress address(
        ip,
        port);


    muduo::net::TcpServer server(
        &m_eventloop,
        address,
        "RpcProvider");


    // ==================================================
    // 6. 连接回调
    // ==================================================

    server.setConnectionCallback(
        std::bind(
            &RpcProvider::OnConnection,
            this,
            std::placeholders::_1));


    // ==================================================
    // 7. 消息回调
    // ==================================================

    server.setMessageCallback(
        std::bind(
            &RpcProvider::OnMessage,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3));


    // ==================================================
    // 8. IO 线程数
    // ==================================================

    server.setThreadNum(4);


    std::cout
        << "RpcProvider start service at ip:"
        << ip
        << " port:"
        << port
        << std::endl;


    // ==================================================
    // 9. 启动 Server
    // ==================================================

    server.start();


    // ==================================================
    // 10. EventLoop
    // ==================================================

    m_eventloop.loop();
}


// ======================================================
// OnConnection
// ======================================================

void RpcProvider::OnConnection(
    const muduo::net::TcpConnectionPtr& conn)
{
    if (!conn->connected())
    {
        return;
    }
}


// ======================================================
// OnMessage
// ======================================================

void RpcProvider::OnMessage(
    const muduo::net::TcpConnectionPtr& conn,
    muduo::net::Buffer* buffer,
    muduo::Timestamp)
{
    while (true)
    {
        // ==================================================
        // 1. header_size
        // ==================================================

        if (buffer->readableBytes()
            < sizeof(uint32_t))
        {
            return;
        }


        // ==================================================
        // 2. 读取 header_size
        // ==================================================

        uint32_t header_size = 0;


        std::memcpy(
            &header_size,
            buffer->peek(),
            sizeof(header_size));


        // ==================================================
        // 3. 判断 Header 是否完整
        // ==================================================

        if (buffer->readableBytes()
            < sizeof(uint32_t) + header_size)
        {
            return;
        }


        // ==================================================
        // 4. 获取 Header
        // ==================================================

        const char* header_ptr =
            buffer->peek()
            + sizeof(uint32_t);


        std::string rpc_header_str(
            header_ptr,
            header_size);


        mprpc::RpcHeader rpc_header;


        if (!rpc_header.ParseFromString(
                rpc_header_str))
        {
            std::cout
                << "rpc header parse error!"
                << std::endl;


            buffer->retrieveAll();


            return;
        }


        // ==================================================
        // 5. 获取 RPC 元信息
        // ==================================================

        std::string service_name =
            rpc_header.service_name();


        std::string method_name =
            rpc_header.method_name();


        uint32_t args_size =
            rpc_header.args_size();


        // ==================================================
        // 6. 判断完整 RPC
        // ==================================================

        size_t total_size =
            sizeof(uint32_t)
            + header_size
            + args_size;


        if (buffer->readableBytes()
            < total_size)
        {
            return;
        }


        // ==================================================
        // 7. 取出 RPC
        // ==================================================

        buffer->retrieve(
            sizeof(uint32_t));


        buffer->retrieve(
            header_size);


        std::string args_str =
            buffer->retrieveAsString(
                args_size);


        // ==================================================
        // 8. 查找 Service
        // ==================================================

        auto service_it =
            m_serviceMap.find(
                service_name);


        if (service_it ==
            m_serviceMap.end())
        {
            std::cout
                << service_name
                << " is not exist!"
                << std::endl;


            continue;
        }


        // ==================================================
        // 9. 查找 Method
        // ==================================================

        auto method_it =
            service_it->second.m_methodMap.find(
                method_name);


        if (method_it ==
            service_it->second.m_methodMap.end())
        {
            std::cout
                << service_name
                << ":"
                << method_name
                << " is not exist!"
                << std::endl;


            continue;
        }


        // ==================================================
        // 10. 获取 Service / Method
        // ==================================================

        google::protobuf::Service* service =
            service_it->second.m_service;


        const google::protobuf::MethodDescriptor*
            method =
                method_it->second;


        // ==================================================
        // 11. 创建 Request
        // ==================================================

        google::protobuf::Message* request =
            service
                ->GetRequestPrototype(method)
                .New();


        if (!request->ParseFromString(
                args_str))
        {
            std::cout
                << "request parse error!"
                << std::endl;


            delete request;


            continue;
        }


        // ==================================================
        // 12. 创建 Response
        // ==================================================

        google::protobuf::Message* response =
            service
                ->GetResponsePrototype(method)
                .New();


        // ==================================================
        // 13. 创建 Closure
        // ==================================================

        google::protobuf::Closure* done =
            google::protobuf::NewCallback<
                RpcProvider,
                const muduo::net::TcpConnectionPtr&,
                google::protobuf::Message*>(
                    this,
                    &RpcProvider::SendRpcResponse,
                    conn,
                    response);


        // ==================================================
        // 14. 调用 RPC 方法
        // ==================================================

        service->CallMethod(
            method,
            nullptr,
            request,
            response,
            done);


        delete request;
    }
}


// ======================================================
// SendRpcResponse
// ======================================================

void RpcProvider::SendRpcResponse(
    const muduo::net::TcpConnectionPtr& conn,
    google::protobuf::Message* response)
{
    // ==================================================
    // 1. 序列化
    // ==================================================

    std::string response_str;


    if (!response->SerializeToString(
            &response_str))
    {
        std::cout
            << "Serialize response error!"
            << std::endl;


        delete response;


        return;
    }


    // ==================================================
    // 2. [response_size][response]
    // ==================================================

    uint32_t response_size =
        static_cast<uint32_t>(
            response_str.size());


    std::string send_data;


    send_data.append(
        reinterpret_cast<const char*>(
            &response_size),
        sizeof(response_size));


    send_data +=
        response_str;


    // ==================================================
    // 3. 发送
    // ==================================================

    conn->send(send_data);


    // ==================================================
    // 4. 长连接
    //
    // 不调用 shutdown()
    // ==================================================

    delete response;
}