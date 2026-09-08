#pragma once

#include <semaphore.h>
#include <zookeeper.h>

#include <string>


// ======================================================
// ZooKeeper 客户端封装
// ======================================================

class ZkClient
{
public:

    ZkClient();

    ~ZkClient();


    // ==================================================
    // 连接 ZooKeeper Server
    // ==================================================

    void Start();


    // ==================================================
    // 创建 znode
    //
    // path     : 节点路径
    // data     : 节点数据
    // datalen  : 数据长度
    // state    : 节点类型
    //
    // 0              -> 持久节点
    // ZOO_EPHEMERAL  -> 临时节点
    // ==================================================

    void Create(
        const char* path,
        const char* data,
        int datalen,
        int state = 0);


    // ==================================================
    // 获取 znode 数据
    // ==================================================

    std::string GetData(
        const char* path);


private:

    // ZooKeeper 客户端句柄
    zhandle_t* m_zhandle;
};