# MPRPC

基于 C++ 的轻量级 RPC 框架学习与改造项目。

本项目基于 **Protobuf、Muduo 和 ZooKeeper** 实现基本的 RPC 服务调用、服务注册与服务发现，并针对原有短连接通信模型进行 **TCP 长连接复用优化**，通过 Benchmark 对优化前后的吞吐、延迟和错误率进行量化测试。

项目主要围绕 RPC 调用链、TCP 网络通信、服务注册与发现以及性能优化展开实践。

---

## 项目简介

MPRPC 是一个轻量级 C++ RPC 框架学习与改造项目，主要用于实践以下技术：

* C++ 网络编程
* Protobuf 序列化与 RPC 接口定义
* Muduo 网络库
* ZooKeeper 服务注册与发现
* RPC Stub / Channel / Provider 调用链
* TCP 长连接
* TCP 字节流下的消息边界处理
* 并发编程与性能测试
* CMake 构建

项目重点不在于堆积复杂功能，而是通过实际代码分析和 Benchmark 测试理解 RPC 通信过程中的开销，并针对关键路径进行优化。

---

## 整体架构

```text
                         ZooKeeper
                             │
                   服务注册 / 服务发现
                             │
                             ▼
┌───────────────────────────────────────────────────────────┐
│                         RPC Client                        │
│                                                           │
│  Protobuf Stub                                            │
│       │                                                   │
│       ▼                                                   │
│  MprpcChannel                                              │
│       │                                                   │
│       ├────────── ZooKeeper GetData() ─────────────┐      │
│       │                                             │      │
│       │                                  Provider IP:Port│
│       │                                             │      │
│       ▼                                             │      │
│  RPC Request Serialize                              │      │
│       │                                             │      │
│       └────────────── TCP 长连接 ──────────────────┘      │
└───────────────────────────┬───────────────────────────────┘
                            │
                            ▼
                     RpcProvider
                            │
                   Service / Method 查找
                            │
                            ▼
                 Protobuf Request 反序列化
                            │
                            ▼
                    Business Service
                            │
                            ▼
                 Protobuf Response 序列化
                            │
                            ▼
                     TCP 长连接返回
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

## RPC 调用流程

一次 RPC 调用的核心流程：

```text
Protobuf Stub
      ↓
MprpcChannel::CallMethod()
      ↓
获取 Service / Method
      ↓
序列化 Request
      ↓
构造 RpcHeader
      ↓
ZooKeeper 查询 Provider 地址
      ↓
首次调用建立 TCP 长连接
      ↓
SendAll()
      ↓
RpcProvider::OnMessage()
      ↓
解析 RPC Header
      ↓
查找 Service / Method
      ↓
反序列化 Request
      ↓
调用实际业务方法
      ↓
序列化 Response
      ↓
SendRpcResponse()
      ↓
RecvAll()
      ↓
反序列化 Response
      ↓
RPC 调用完成
```

后续 RPC 调用在连接正常的情况下直接复用已经建立的 TCP 长连接。

---

## 核心模块

### MprpcApplication

负责 RPC 框架初始化以及配置文件读取。

### MprpcChannel

继承：

```cpp
google::protobuf::RpcChannel
```

负责客户端 RPC 调用的核心流程：

* 获取 Service / Method 信息
* 通过 ZooKeeper 查询 Provider 地址
* 序列化 RPC Request
* 构造 RPC Header
* 建立 TCP 连接
* 发送 RPC 请求
* 接收 RPC 响应
* 反序列化 RPC Response
* 复用 TCP 长连接进行后续 RPC 调用

### RpcProvider

负责 RPC 服务端：

* 注册 RPC Service
* 保存 Service / Method 信息
* 向 ZooKeeper 注册服务地址
* 接收客户端 RPC 请求
* 解析 RPC Header
* 查找 Service / Method
* 创建 Request / Response
* 调用具体业务方法
* 返回 RPC 响应

### ZooKeeper

用于实现 RPC 服务注册与服务发现。

Provider 启动时，将 Service / Method 和自身 IP、Port 注册到 ZooKeeper。

例如：

```text
/FriendServiceRpc
    └── GetFriendsList
            └── 127.0.0.1:8000
```

Consumer 调用 RPC 时，通过 Service / Method 对应路径查询 Provider 地址。

Method 节点使用 ZooKeeper 临时节点，Provider 的 ZooKeeper Session 消失后，对应注册信息会自动删除。

### Protobuf

用于：

* 定义 RPC Service
* 定义 Request / Response
* RPC 数据序列化与反序列化
* 生成客户端 Stub
* 生成服务端 Service 接口

---

## ZooKeeper 服务注册与发现

### 服务注册

Provider 启动后：

```text
RpcProvider::Run()
        ↓
ZkClient::Start()
        ↓
连接 ZooKeeper
        ↓
RegisterServiceToZk()
        ↓
创建 Service 节点
        ↓
创建 Method 节点
        ↓
保存 Provider IP:Port
```

例如：

```text
/FriendServiceRpc
    └── GetFriendsList
            └── 127.0.0.1:8000
```

可以通过 ZooKeeper CLI 查看：

```bash
~/package/zookeeper/bin/zkCli.sh -server 127.0.0.1:2181
```

然后：

```text
ls /FriendServiceRpc
get /FriendServiceRpc/GetFriendsList
```

当前测试结果：

```text
/FriendServiceRpc/GetFriendsList
        ↓
127.0.0.1:8000
```

### 服务发现

Consumer 第一次调用 RPC 时：

```text
MprpcChannel::CallMethod()
        ↓
ResolveProvider()
        ↓
ZooKeeper GetData()
        ↓
/FriendServiceRpc/GetFriendsList
        ↓
127.0.0.1:8000
        ↓
Connect()
        ↓
建立 TCP 长连接
```

后续 RPC 调用直接复用已经建立的 TCP 连接。

因此 ZooKeeper 主要负责：

```text
“找到 Provider”
```

而 RPC 数据传输仍然通过：

```text
TCP 长连接
```

完成。

---

# 性能优化

## 简单版本：短连接基线

原始通信模型中，每次 RPC 都重新创建和关闭 TCP 连接：

```text
RPC 1

socket
  ↓
connect
  ↓
send
  ↓
recv
  ↓
close

RPC 2

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

每次 RPC 都需要重复执行 TCP 连接建立和关闭。

Benchmark：

```text
Threads      : 8
Duration     : 10s
Server       : 127.0.0.1

QPS          : 3058.57
Avg Latency  : 2.612 ms
Success      : 33941
Failed       : 0
Error Rate   : 0%
```

---

## 升级版本：TCP 长连接复用

针对短连接模型进行优化，为每个 Benchmark 线程维护一个 TCP 长连接：

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

同时针对 TCP 字节流特性处理 RPC 消息边界，使同一个 TCP 连接能够连续完成多次 RPC 调用。

Benchmark：

```text
Threads      : 8
Duration     : 10s
Server       : 127.0.0.1

QPS          : 44623.5
Avg Latency  : 0.177 ms
Success      : 446324
Failed       : 0
Error Rate   : 0%
```

---

## 优化结果

### QPS

短连接：

```text
3058.57 QPS
```

长连接：

```text
44623.5 QPS
```

提升约：

```text
44623.5 / 3058.57 ≈ 14.6 倍
```

### 平均延迟

```text
2.612 ms
   ↓
0.177 ms
```

平均延迟降低约 **93%**。

### 优化原因

长连接主要减少了每次 RPC 重复执行：

```text
socket()
connect()
close()
```

带来的连接管理开销。

原来的连接建立开销发生在每一次 RPC 中；优化后，连接建立开销转移到了连接生命周期的开始阶段，后续请求可以复用已有 TCP 连接。

---

## ZooKeeper 接入后的 Benchmark

在长连接版本基础上接入 ZooKeeper 服务注册与发现后，当前本地测试结果为：

```text
Threads      : 8
Duration     : 10s
Server       : 127.0.0.1

QPS          : 42051.7
Avg Latency  : 0.187245 ms
Success      : 422073
Failed       : 0
Error Rate   : 0%
```

该测试用于验证 **ZooKeeper 服务发现功能与长连接 RPC 的结合效果**。

ZooKeeper 只参与服务发现，正常 RPC 数据传输仍然使用 TCP 长连接。

> 注：以上性能数据均为本地 `127.0.0.1` 环境、8 线程、10 秒测试结果，主要用于不同通信模型之间的对比。实际性能会受到 CPU、操作系统、网络栈以及测试负载等因素影响。

---

## Benchmark

Benchmark 程序位于：

```text
example/caller/bench_echo.cc
```

当前 Benchmark 配置：

```text
Threads  : 8
Duration : 10s
```

运行：

```bash
./bin/bench_echo -i ./bin/test.conf
```

Benchmark 主要统计：

```text
Total Requests
Success Requests
Failed Requests
QPS
Average Latency
Error Rate
```

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

---

## 学习与改造重点

本项目主要围绕以下问题进行学习与实践：

1. RPC 从客户端到服务端的完整调用链
2. Protobuf 生成代码与 RPC Stub / Service 的关系
3. `RpcChannel` 如何完成 RPC 请求封装和发送
4. `RpcProvider` 如何完成 Service / Method 注册、查找和调用
5. ZooKeeper 如何实现 RPC 服务注册与服务发现
6. TCP 长连接与短连接的性能差异
7. TCP 字节流下的消息边界与半包 / 粘包处理
8. Benchmark 如何量化 RPC 吞吐、延迟和错误率
9. 在增加服务发现机制后保持 RPC 数据面的长连接通信

---

## 说明

本项目主要用于 C++ 网络编程、RPC 框架和性能优化方向的学习与实践，并在基础实现上进行代码理解、重构和功能改造。

当前版本重点实现：

```text
Protobuf RPC
     +
Muduo
     +
ZooKeeper 服务注册与发现
     +
TCP 长连接
     +
Benchmark 性能测试
```

项目中的性能数据为本地测试环境结果，主要用于不同通信模型之间的相对比较，不代表生产环境下的实际性能。
