#include<iostream>
#include<string>
#include"friend.pb.h"
#include"mprpcapplication.h"
#include"rpcprovider.h"
#include"logger.h"

/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和Register
*/
class FriendService : public fixbug::FriendServiceRpc
{
public:
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout<<"do GetFriendList service! userid:"<<userid<<std::endl;
        std::vector<std::string> vec;
        vec.push_back("xianwu");
        vec.push_back("zhipeng");
        vec.push_back("yongjie");
        return vec;
    }

    void GetFriendsList(::google::protobuf::RpcController* controller,
                    const ::fixbug::GetFriendsListRequest* request,
                    ::fixbug::GetFriendsListResponse* response,
                    ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取响应数据做本地业务
        uint32_t userid = request->userid();

        std::vector<std::string> friendlist = GetFriendsList(userid);   // 做本地业务

        // 把响应写入
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for(auto &name : friendlist)
        {
            // 实现的功能：将response里面friends修改成了我们的name。
            std::string *p = response->add_friends();
            *p = name;
        }
        
        // 执行回调操作
        done->Run(); 

    }

};

int main(int argc,char **argv)
{
    // LOG_INFO("first log message!");
    // LOG_ERR("%s:%s:%d",__FILE__,__FUNCTION__,__LINE__);
    // 调用框架的初始化操作 provider -i config.conf
    MprpcApplication::Init(argc,argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 启动一个rpc服务发布节点  Run以后，进程进入阻塞状态，等待远程的rpc调用请求。
    provider.Run();

    return 0;
}





