#pragma once
#include<mprpcconfig.h>
#include<mprpcchannel.h>
#include<mprpccontroller.h>

// mprpc框架的基础类，负责框架的一些初始化操作        单例的设计模式
class MprpcApplication
{
public:
    static void Init(int argc,char **argv);
    static MprpcApplication& GetInstance();  //必须加&，拷贝函数已经删除。
    MprpcConfig &GetConfig(); 
private:
    static MprpcConfig m_config;

    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete; //删除拷贝构造函数。
    MprpcApplication(MprpcApplication&&) = delete;  //删除移动构造函数，即move constructor禁止。
};


