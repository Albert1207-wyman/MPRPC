#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "friend.pb.h"
#include "mprpcapplication.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

// 成功请求数
static std::atomic<uint64_t> g_success{0};

// 失败请求数
static std::atomic<uint64_t> g_failed{0};

// 所有成功请求的总耗时，单位：微秒
static std::atomic<uint64_t> g_total_latency_us{0};


// Benchmark 配置
struct BenchConfig
{
    // 客户端线程数
    int threads = 8;

    // 测试持续时间
    int duration_sec = 10;
};


// 每个线程执行的测试任务
void WorkerThread(const BenchConfig& config)
{
    // 每个线程创建自己的 RPC Channel
    MprpcChannel channel;

    // 创建 FriendService RPC Stub
    fixbug::FriendServiceRpc_Stub stub(&channel);

    // 构造固定请求
    fixbug::GetFriendsListRequest request;

    request.set_userid(1000);


    // 当前线程的测试结束时间
    auto deadline =
        std::chrono::steady_clock::now()
        + std::chrono::seconds(config.duration_sec);


    // 持续发送 RPC 请求
    while (std::chrono::steady_clock::now() < deadline)
    {
        // 每次 RPC 都创建新的响应对象
        fixbug::GetFriendsListResponse response;

        // RPC Controller
        MprpcController controller;


        // 开始统计时间
        auto start =
            std::chrono::steady_clock::now();


        // 发起 RPC 调用
        stub.GetFriendsList(
            &controller,
            &request,
            &response,
            nullptr);


        // 计算本次 RPC 耗时
        auto elapsed =
            std::chrono::duration_cast<
                std::chrono::microseconds>(
                    std::chrono::steady_clock::now()
                    - start)
                .count();


        // 判断 RPC 是否成功
        if (!controller.Failed() &&
            response.result().errcode() == 0)
        {
            // 成功数 +1
            g_success.fetch_add(
                1,
                std::memory_order_relaxed);


            // 累加耗时
            g_total_latency_us.fetch_add(
                static_cast<uint64_t>(elapsed),
                std::memory_order_relaxed);
        }
        else
        {
            // 失败数 +1
            g_failed.fetch_add(
                1,
                std::memory_order_relaxed);
        }
    }
}


int main(int argc, char* argv[])
{
    // 初始化 MPRPC
    //
    // 当前框架要求：
    //
    // ./bench_echo -i ./bin/test.conf
    //
    MprpcApplication::Init(argc, argv);


    // 创建 Benchmark 配置
    //
    // 当前先固定：
    // 8 个线程
    // 测试 10 秒
    //
    // 等基线跑通以后，
    // 我们再考虑是否开放更多参数。
    BenchConfig config;


    // 输出测试配置
    std::cout
        << "========================================"
        << std::endl;

    std::cout
        << "        MPRPC Benchmark - S0"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl;

    std::cout
        << "Threads : "
        << config.threads
        << std::endl;

    std::cout
        << "Duration: "
        << config.duration_sec
        << "s"
        << std::endl;


    // 创建测试线程
    std::vector<std::thread> workers;

    // Benchmark 实际开始时间
    auto start =
        std::chrono::steady_clock::now();


    // 创建线程
    for (int i = 0;
         i < config.threads;
         ++i)
    {
        workers.emplace_back(
            WorkerThread,
            std::cref(config));
    }


    // 等待所有线程结束
    for (auto& worker : workers)
    {
        worker.join();
    }


    // 计算实际测试时间
    auto elapsed_ms =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                std::chrono::steady_clock::now()
                - start)
            .count();


    double elapsed_sec =
        elapsed_ms / 1000.0;


    // 获取统计结果
    uint64_t success =
        g_success.load(
            std::memory_order_relaxed);

    uint64_t failed =
        g_failed.load(
            std::memory_order_relaxed);


    uint64_t total =
        success + failed;


    // 计算 QPS
    //
    // QPS = 成功请求数 / 实际测试时间
    double qps =
        elapsed_sec > 0.0
            ? static_cast<double>(success)
                / elapsed_sec
            : 0.0;


    // 计算平均延迟
    double avg_latency_ms =
        success > 0
            ? static_cast<double>(
                  g_total_latency_us.load(
                      std::memory_order_relaxed))
              / success
              / 1000.0
            : 0.0;


    // 计算错误率
    double error_rate =
        total > 0
            ? static_cast<double>(failed)
                * 100.0
                / total
            : 0.0;


    // 输出结果
    std::cout
        << std::endl
        << "========================================"
        << std::endl;

    std::cout
        << "                Result"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl;

    std::cout
        << "Total        : "
        << total
        << std::endl;

    std::cout
        << "Success      : "
        << success
        << std::endl;

    std::cout
        << "Failed       : "
        << failed
        << std::endl;

    std::cout
        << "QPS          : "
        << qps
        << std::endl;

    std::cout
        << "Avg Latency  : "
        << avg_latency_ms
        << " ms"
        << std::endl;

    std::cout
        << "Error Rate   : "
        << error_rate
        << "%"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl;


    return 0;
}