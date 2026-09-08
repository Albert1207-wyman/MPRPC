# MPRPC

基于 C++ 的轻量级 RPC 框架学习与改造项目。

本项目基于 Protobuf、Muduo 和 ZooKeeper 实现基本的 RPC 服务调用、服务注册与发现，并针对原有短连接通信模型进行长连接复用优化，通过 Benchmark 对优化前后的性能进行对比。

## 项目简介

MPRPC 是一个轻量级 C++ RPC 框架，主要用于学习和实践以下技术：

* C++ 网络编程
* Protobuf 序列化与 RPC 接口定义
* Muduo 网络库
* ZooKeeper 服务注册与发现
* RPC Stub / Channel / Provider 调用链
* TCP 长连接
* 并发与性能测试

项目重点不在于堆积复杂功能，而是通过实际性能测试分析 RPC 通信过程中的开销，并对关键路径进行针对性优化。

---

## 整体架构

```text
            RPC Client
                │
                ▼
        Protobuf Stub
                │
                ▼
          MprpcChannel
                │
        RPC Request Serialize
                │
                ▼
              TCP
                │
                ▼
          RpcProvider
                │
        Service / Method 查找
                │
                ▼
          Protobuf Deserialize
                │
                ▼
          Business Service
                │
                ▼
          RPC Response
                │
                ▼
              TCP
                │
                ▼
          MprpcChannel
                │
        Response Deserialize
                │
                ▼
            RPC Client
```

---

## 核心模块

### MprpcApplication

负责 RPC 框架初始化以及配置文件读取。

### MprpcChannel

继承 `google::protobuf::RpcChannel`，负责：

* 获取 Service / Method 信息
* 序列化 RPC 请求
* 构造 RPC Header
* 建立 TCP 连接
* 发送 RPC 请求
* 接收 RPC 响应
* 反序列化 RPC 响应

### RpcProvider

负责 RPC 服务端：

* 注册 RPC Service
* 保存 Service / Method 信息
* 接收客户端请求
* 解析 RPC Header
* 创建 Request / Response
* 调用具体业务方法
* 返回 RPC 响应

### ZooKeeper

用于 RPC 服务节点的注册与发现。

### Protobuf

用于：

* 定义 RPC Service
* 定义 Request / Response
* RPC 数据序列化与反序列化
* 生成客户端 Stub 和服务端接口

---

## 性能优化

### 简单版：短连接基线

原始通信模型：

```text
RPC 1:
socket
  ↓
connect
  ↓
send
  ↓
recv
  ↓
close

RPC 2:
socket
  ↓
connect
  ↓
send
  ↓
recv
  ↓
close
```

每次 RPC 都重新建立和关闭 TCP 连接。

Benchmark：

```text
Threads       : 8
Duration      : 10s
Server        : 127.0.0.1

QPS           : 3058.57
Avg Latency   : 2.612 ms
Success       : 33941
Failed        : 0
Error Rate    : 0%
```

---

### 升级版：TCP 长连接复用

针对短连接模型进行优化，将通信方式调整为长连接复用：

```text
线程启动
   ↓
建立 TCP 连接
   ↓
RPC 1
   ↓
RPC 2
   ↓
RPC 3
   ↓
...
   ↓
测试结束
   ↓
关闭连接
```

同时对长连接下的消息边界进行处理，使同一 TCP 连接可以连续完成多次 RPC 调用。

Benchmark：

```text
Threads       : 8
Duration      : 10s
Server        : 127.0.0.1

QPS           : 44623.5
Avg Latency   : 0.177 ms
Success       : 446324
Failed        : 0
Error Rate    : 0%
```

相比 S0：

```text
QPS:
3058.57
   ↓
44623.5

约提升 14.6 倍
```

平均延迟：

```text
2.612 ms
   ↓
0.177 ms
```

降低约 93%。

> 注：以上数据为本地 127.0.0.1 环境、8 线程、10 秒测试结果，具体性能与 CPU、系统环境、网络栈及测试负载有关。

---

## Benchmark

Benchmark 程序位于：

```text
example/caller/bench_echo.cc
```

运行：

```bash
./bin/bench_echo -i ./bin/test.conf
```

Benchmark 主要统计：

* Total Requests
* Success Requests
* Failed Requests
* QPS
* Average Latency
* Error Rate

---

## 项目结构

```text
MPRPC
├── src
│   ├── include
│   │   ├── lockqueue.h
│   │   ├── logger.h
│   │   ├── mprpcapplication.h
│   │   ├── mprpcchannel.h
│   │   ├── mprpcconfig.h
│   │   ├── mprpccontroller.h
│   │   ├── rpcprovider.h
│   │   └── zookeeperutil.h
│   │
│   ├── mprpcapplication.cc
│   ├── mprpcchannel.cc
│   ├── mprpcconfig.cc
│   ├── mprpccontroller.cc
│   ├── rpcprovider.cc
│   ├── zookeeperutil.cc
│   └── ...
│
├── example
│   ├── caller
│   │   ├── bench_echo.cc
│   │   ├── callfriendservice.cc
│   │   └── calluserservice.cc
│   │
│   ├── callee
│   │   ├── friendservice.cc
│   │   ├── userservice.cc
│   │   └── echoservice.cc
│   │
│   ├── friend.proto
│   ├── user.proto
│   └── echo.proto
│
├── test
├── CMakeLists.txt
├── autobuild.sh
└── README.md
```

---

## 技术栈

```text
C++
Protobuf
Muduo
ZooKeeper
CMake
Linux
TCP
Git
```



## 学习与改造重点

本项目主要围绕以下问题进行学习和实践：

1. RPC 调用从客户端到服务端的完整调用链
2. Protobuf 生成代码与 RPC Stub / Service 的关系
3. RpcChannel 如何完成 RPC 请求封装和发送
4. RpcProvider 如何完成服务注册、方法查找和调用
5. ZooKeeper 如何参与服务注册与发现
6. TCP 长连接与短连接的性能差异
7. TCP 字节流下的消息边界问题
8. Benchmark 如何对 RPC 吞吐和延迟进行量化


## 说明

本项目主要用于 C++ 网络编程、RPC 框架和性能优化方向的学习与实践，并在已有基础实现上进行代码理解、重构和性能改造。
