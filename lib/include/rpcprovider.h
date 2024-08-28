#pragma once
#include<google/protobuf/service.h>
#include<memory>
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<muduo/net/InetAddress.h>
#include<string>
#include<mprpcapplication.h>
#include<google/protobuf/descriptor.h>
#include<unordered_map>
#include"logger.h"
#include"zookeeperutil.h"

// 框架提供的专门服务发布rpc服务的网络对象类
class RpcProvider
{
public:
    // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    // 启动rpc服务节点，开始提供rpc网络调用服务
    void Run();

    // 新的socket连接回调
    void onConnection(const muduo::net::TcpConnectionPtr&);
    // 已建立连接用户的读写事件回调
    void onMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buffer,muduo::Timestamp time);
    // Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&,google::protobuf::Message*);

private:
    // // 组合了TcpServer   在Run()中创建吧
    // std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr; 
    // 组合了EventLoop
    muduo::net::EventLoop m_eventLoop;

    // 存放服务名和服务方法的Map容器
    struct ServiceInfo
    {
        google::protobuf::Service* m_service;   //保存服务对象
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor *> m_methodMap;
    };
    // 存储注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string,struct ServiceInfo> m_serviceMap;

};