#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "zookeeperutil.h"


class RpcProvider
{
public:

    // ==================================================
    // 发布 RPC 服务
    // ==================================================

    void NotifyService(
        google::protobuf::Service* service);


    // ==================================================
    // 启动 RPC 服务
    // ==================================================

    void Run();


private:

    // ==================================================
    // Service 信息
    // ==================================================

    struct ServiceInfo
    {
        google::protobuf::Service* m_service;

        std::unordered_map<
            std::string,
            const google::protobuf::MethodDescriptor*>
            m_methodMap;
    };


    // ==================================================
    // 所有注册 Service
    // ==================================================

    std::unordered_map<
        std::string,
        ServiceInfo>
        m_serviceMap;


    // ==================================================
    // Muduo EventLoop
    // ==================================================

    muduo::net::EventLoop m_eventloop;


    // ==================================================
    // ZooKeeper Client
    // ==================================================

    ZkClient m_zkClient;


private:

    // ==================================================
    // 注册服务到 ZooKeeper
    // ==================================================

    void RegisterServiceToZk(
        const std::string& ip,
        uint16_t port);


    // ==================================================
    // 新连接回调
    // ==================================================

    void OnConnection(
        const muduo::net::TcpConnectionPtr& conn);


    // ==================================================
    // 消息回调
    // ==================================================

    void OnMessage(
        const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buffer,
        muduo::Timestamp time);


    // ==================================================
    // RPC 响应
    // ==================================================

    void SendRpcResponse(
        const muduo::net::TcpConnectionPtr& conn,
        google::protobuf::Message* response);
};