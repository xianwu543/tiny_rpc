#include"mprpcchannel.h"

/*
header_size + [service_name method_name args_size] + args
*/
// 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据序列化和网络发送。
void MrprcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller, 
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response, 
                        google::protobuf::Closure* done)
{
    // 填充得到字符流。

    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();  // service_name
    std::string method_name = method->name();   // method_name
    
    // 获取参数的序列化字符串长度 args_size
    int args_size = 0;
    std::string args_str;
    if(request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return ; 
    }

    // 定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return ; 
    }

    // 组织待发送的rpc请求的字符串
    std::string send_rpc_str((char*)&header_size,4);    // 填写前四字节 header_size
    send_rpc_str += rpc_header_str;     // rpcheader
    send_rpc_str += args_str;

    // 使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1==clientfd)
    {
        char errtxt[512]={0};
        sprintf(errtxt,"create socket error! errno:%d",errno);
        controller->SetFailed(errtxt);
        return;
    }

    // // 读取配置文件rpcserver的信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    ///////////////////////////////////////////////////////////////////////////////////
    // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    ZkClient zkCli;
    zkCli.Start();  // 连接上了zk服务端
    // UserServiceRpc/Login
    std::string method_path = "/" + service_name+"/"+method_name;
    // 127.0.0.1:8000
    std::string host_data = zkCli.GetData(method_path.c_str());
    if(host_data=="")
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    int idx = host_data.find(":");
    if(idx==-1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0,idx);
    uint16_t port = atoi(host_data.substr(idx+1,host_data.size()-idx).c_str());
    ///////////////////////////////////////////////////////////////////////////////////

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if(-1==connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr)))
    {
        char errtxt[512]={0};
        sprintf(errtxt,"connect error! errno:%d",errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    // 发送rpc请求
    if(-1==send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0))
    {
        char errtxt[512]={0};
        sprintf(errtxt,"send error! errno:%d",errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return ;
    }

    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if(-1==(recv_size = recv(clientfd,recv_buf,sizeof(recv_buf),0)))
    {
        char errtxt[512]={0};
        sprintf(errtxt,"recv error! errno:%d",errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return ;
    }

    // 反序列化rpc调用的响应数据
    std::string response_str(recv_buf,recv_size);
    if(!response->ParseFromString(response_str))
    {
        char errtxt[512]={0};
        sprintf(errtxt,"parse error! response_str:%s",response_str.c_str());
        controller->SetFailed(errtxt);
        close(clientfd);
        return ;
    }

    close(clientfd);

}