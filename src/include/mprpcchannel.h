#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "zookeeperutil.h"


class MprpcChannel
    : public google::protobuf::RpcChannel
{
public:

    MprpcChannel();

    ~MprpcChannel() override;


    // ==================================================
    // 所有 Stub RPC 调用最终进入这里
    // ==================================================

    void CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done) override;


private:

    // ==================================================
    // 从 ZooKeeper 查询 Provider
    // ==================================================

    bool ResolveProvider(
        const std::string& service_name,
        const std::string& method_name,
        std::string& ip,
        uint16_t& port,
        google::protobuf::RpcController* controller);


    // ==================================================
    // 建立 TCP 长连接
    // ==================================================

    bool Connect(
        const std::string& ip,
        uint16_t port,
        google::protobuf::RpcController* controller);


    // ==================================================
    // 完整发送
    // ==================================================

    bool SendAll(
        const char* data,
        size_t size,
        google::protobuf::RpcController* controller);


    // ==================================================
    // 完整接收
    // ==================================================

    bool RecvAll(
        char* data,
        size_t size,
        google::protobuf::RpcController* controller);


private:

    // ==================================================
    // ZooKeeper Client
    // ==================================================

    ZkClient zkclient_;


    // ==================================================
    // TCP 连接
    // ==================================================

    int clientfd_;


    // ==================================================
    // 连接状态
    // ==================================================

    bool connected_;


    // ==================================================
    // 当前 Provider 地址
    // ==================================================

    std::string current_ip_;

    uint16_t current_port_;
};