#include "mprpccontroller.h"

// 构造函数：初始化状态
MprpcController::MprpcController()
{
    m_failed = false;
    m_errText = "";
}

// 重置控制器状态
void MprpcController::Reset()
{
    m_failed = false;
    m_errText = "";
}

// 检查是否执行失败
bool MprpcController::Failed() const
{
    return m_failed;
}

// 获取错误文本信息
std::string MprpcController::ErrorText() const
{
    return m_errText;
}

// 设置失败状态及原因
void MprpcController::SetFailed(const std::string& reason)
{
    m_failed = true;
    m_errText = reason;
}

// 目前未实现具体的功能 (空实现)
void MprpcController::StartCancel() {}

// 检查是否已取消 (目前默认返回 false)
bool MprpcController::IsCanceled() const 
{ 
    return false; 
}

// 通知取消时的回调 (空实现)
void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback) {}