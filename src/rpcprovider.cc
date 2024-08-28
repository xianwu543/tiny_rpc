#include"rpcprovider.h"
#include<string>
#include<mprpcapplication.h>
#include<google/protobuf/descriptor.h>
#include"rpcheader.pb.h"
#include"logger.h"

// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    // 相当于一个注册的功能
    struct ServiceInfo serviceInfo;

    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *serviceDescription = service->GetDescriptor();
    // 获取服务的名字
    const std::string serviceName = serviceDescription->name();
    // 获取服务对象service的方法的数量
    int count = serviceDescription->method_count(); // 方法数量

    // std::cout<<"serviceName:"<<serviceName<<std::endl;
    LOG_INFO("serviceName:%s",serviceName.c_str());

    for(int ii=0;ii<count;ii++)
    {
        // 获取了服务对象指定下标的服务方法的描述（抽象描述 UserService Login）
        const google::protobuf::MethodDescriptor *methodDescription = serviceDescription->method(ii);
        std::string methodName = methodDescription->name();
        serviceInfo.m_methodMap.insert({methodName,methodDescription});

        // std::cout<<"methodName:"<<methodName<<std::endl;
        LOG_INFO("methodName:%s",methodName.c_str());
    }
    serviceInfo.m_service = service;
    m_serviceMap.insert({serviceName,serviceInfo});
    
}

// 启动rpc服务节点，开始提供rpc网络调用服务
void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip,port);

    // 创建一个TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");

    // 绑定连接回调和消息读写回调方法   分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::onConnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

    // 设置muduo库的线程数量
    server.setThreadNum(4);

    ////////////////////////////////////////////////////////////////////////////////////
    // 吧当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    // session timeout 30s      zkclient 网络I/O线程  1/3 * timeout 时间发送ping消息
    ZkClient zkCli;
    zkCli.Start();
    // service_name为永久性节点  mtrhod_name为临时性节点
    for(auto &sp:m_serviceMap)
    {
        // /service_name    /UserServiceRpc
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(),nullptr,0);    // 1.不写信息 2.第四个参数默认永久性节点
        for(auto &mp:sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128]={0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);
            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL); // 将本服务的ip:port作为信息加入zk 第四个参数：临时性节点
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////


    // rpc服务端准备启动，打印信息
    std::cout<<"RpcProvider start service at ip:"<<ip<<" port:"<<port<<std::endl;   
    // LOG_INFO("RpcProvider start service at ip:%s port:%d",ip.c_str(),port);

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}

// 新的socket连接回调
void RpcProvider::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();
    }
}

/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用protobuf数据类型
service_name method_name atgs   定义proto的message类型，进行数据头的序列化和反序列化
                                service_name method_name args_size
16UserServiceLoginzhang san123456

header_size(4字节) + header_str + args_str
*/
// 已建立连接用户的读写事件回调
void RpcProvider::onMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buffer,muduo::Timestamp time)
{
    // 视频没说清楚，但是我猜测数据流为：rpcheader+args，也就是先发送头部再发送参数

    // 网络上接受的远程rpc调用请求的字符流  Login args
    std::string recv_buf = buffer->retrieveAllAsString();

    // 前4字节指定rpcheader长度（解决粘包问题）
    // 从字符流中读取前4字节的内容
    int header_size = 0;
    memcpy(&header_size,recv_buf.c_str(),4);    // 读取前四字节（表示rpcheader长度）

    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4,header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        // 数据头反序列化失败
        // std::cout<<"rpc_header_str:"<<rpc_header_str<<" parse error!"<<std::endl;
        LOG_ERR("rpc_header_str:%s parse errpr!",rpc_header_str.c_str());
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4+header_size,args_size);

    // 打印调试信息
    std::cout<<"================================================"<<std::endl;
    std::cout<<"header_size:"<<header_size<<std::endl;
    std::cout<<"rpc_header_str:"<<rpc_header_str<<std::endl;
    std::cout<<"service_name:"<<service_name<<std::endl;
    std::cout<<"method_name:"<<method_name<<std::endl;
    std::cout<<"args_str:"<<args_str<<std::endl;
    std::cout<<"================================================"<<std::endl;



    // 获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if(it==m_serviceMap.end())
    {
        // std::cout<<service_name<<" is not exist!"<<std::endl;
        LOG_ERR("%s is not exist!",service_name.c_str());
        return ;
    }
    google::protobuf::Service *service = it->second.m_service;    // 获取service对象  new UserService
    
    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end())
    {
        // std::cout<<service_name<<":"<<method_name<<" is not exist!"<<std::endl;
        LOG_ERR("%s:%s is not exist!",service_name.c_str(),method_name.c_str());
        return ;
    }
    const google::protobuf::MethodDescriptor *method = mit->second; // 获取method对象 Login
    
    // 生成rpc方法调用的请求request和响应response参数 
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str))
    {
        // std::cout<<"request parse error,content:"<<args_str<<std::endl;
        LOG_ERR("request parse error,content:%s",args_str.c_str());
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的mthod()方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure *done = 
        google::protobuf::NewCallback<RpcProvider,
            const muduo::net::TcpConnectionPtr&, 
            google::protobuf::Message*>
                (this,
                &RpcProvider::SendRpcResponse,
                conn,
                response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    // new UserService().Login(controller,request,response,done)
    service->CallMethod(method,nullptr,request,response,done);


}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response)
{
    // 在这里实现你的逻辑
    std::string response_str;
    if(response->SerializeToString(&response_str))
    {
        // 序列化成功后，通过网络吧rpc方法执行的结果发送回rpc的调用方
        conn->send(response_str);
    }
    else
    {
        // std::cout<<"serialize response_str__error!"<<std::endl;
        LOG_ERR("serialize response_str__error!");
    }
    conn->shutdown();   // 模拟HTTP的短连接服务，由rpcprovider主动断开连接

}
