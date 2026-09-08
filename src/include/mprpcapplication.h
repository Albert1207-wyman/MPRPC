#pragma once
#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

//mprpc框架初始化
class MprpcApplication
{
public:
    //框架的初始化操作
    static void Init(int argc, char **argv);
    static MprpcApplication &GetInstance();
    static MprpcConfig &GetConfig();

private:
    static MprpcConfig m_config; // 配置文件对象


    MprpcApplication() {} //单例模式，禁止外部构造
    MprpcApplication(const MprpcApplication &) = delete; //禁止拷贝构造
    MprpcApplication(MprpcApplication &&) = delete; //禁止移动构造


};