#include "zookeeperutil.h"

#include <cstring>
#include <iostream>
#include <semaphore.h>


// ======================================================
// ZooKeeper 全局 watcher
// ======================================================

void global_watcher(
    zhandle_t* zh,
    int type,
    int state,
    const char* path,
    void* watcherCtx)
{
    (void)path;
    (void)watcherCtx;


    // ==================================================
    // ZooKeeper Session 连接成功
    // ==================================================

    if (type == ZOO_SESSION_EVENT &&
        state == ZOO_CONNECTED_STATE)
    {
        const sem_t* sem_ptr =
            static_cast<const sem_t*>(
                zoo_get_context(zh));


        sem_t* sem =
            const_cast<sem_t*>(
                sem_ptr);


        if (sem != nullptr)
        {
            sem_post(sem);
        }
    }
}


// ======================================================
// ZkClient 构造函数
// ======================================================

ZkClient::ZkClient()
    : m_zhandle(nullptr)
{
}


// ======================================================
// ZkClient 析构函数
// ======================================================

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle);

        m_zhandle = nullptr;
    }
}


// ======================================================
// Start
// ======================================================

void ZkClient::Start()
{
    // ==================================================
    // 1. 初始化信号量
    // ==================================================

    sem_t sem;

    sem_init(
        &sem,
        0,
        0);


    // ==================================================
    // 2. 连接 ZooKeeper Server
    // ==================================================

    m_zhandle =
        zookeeper_init(
            "127.0.0.1:2181",
            global_watcher,
            30000,
            nullptr,
            &sem,
            0);


    if (m_zhandle == nullptr)
    {
        std::cout
            << "zookeeper_init error!"
            << std::endl;

        sem_destroy(&sem);

        return;
    }


    // ==================================================
    // 3. 等待连接成功
    // ==================================================

    sem_wait(&sem);


    std::cout
        << "zookeeper connect success!"
        << std::endl;


    // ==================================================
    // 4. 销毁信号量
    // ==================================================

    sem_destroy(&sem);
}


// ======================================================
// Create
// ======================================================

void ZkClient::Create(
    const char* path,
    const char* data,
    int datalen,
    int state)
{
    if (m_zhandle == nullptr)
    {
        std::cout
            << "zookeeper client is not started!"
            << std::endl;

        return;
    }


    // ==================================================
    // 1. 判断节点是否已经存在
    // ==================================================

    int exists_rc =
        zoo_exists(
            m_zhandle,
            path,
            0,
            nullptr);


    if (exists_rc == ZOK)
    {
        std::cout
            << "znode already exists: "
            << path
            << std::endl;

        return;
    }


    // ==================================================
    // 2. 创建节点
    // ==================================================

    char path_buffer[256] = {0};


    int rc =
        zoo_create(
            m_zhandle,
            path,
            data,
            datalen,
            &ZOO_OPEN_ACL_UNSAFE,
            state,
            path_buffer,
            sizeof(path_buffer));


    if (rc != ZOK)
    {
        std::cout
            << "zoo_create error!"
            << " path="
            << path
            << " rc="
            << rc
            << std::endl;

        return;
    }


    std::cout
        << "znode create success: "
        << path_buffer
        << std::endl;
}


// ======================================================
// GetData
// ======================================================

std::string ZkClient::GetData(
    const char* path)
{
    if (m_zhandle == nullptr)
    {
        std::cout
            << "zookeeper client is not started!"
            << std::endl;

        return "";
    }


    char buffer[1024] = {0};

    int buffer_len =
        sizeof(buffer);


    int rc =
        zoo_get(
            m_zhandle,
            path,
            0,
            buffer,
            &buffer_len,
            nullptr);


    if (rc != ZOK)
    {
        std::cout
            << "zoo_get error!"
            << " path="
            << path
            << " rc="
            << rc
            << std::endl;

        return "";
    }


    return std::string(
        buffer,
        buffer_len);
}