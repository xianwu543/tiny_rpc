#include"test.pb.h"
#include<iostream>
#include<string>
using namespace fixbug;

int main()
{
    // fixbug::LoginResponse rsp;
    // fixbug::ResultCode *rc = rsp.mutable_result();
    // rc->set_errcode(1);
    // rc->set_errmsg("登陆处理失败了");
     
    fixbug::GetFriendListResponse rsp;
    fixbug::ResultCode *rc = rsp.mutable_result();
    rc->set_errcode(0);

    User *user1 = rsp.add_friend_list();
    user1->set_name("zz");
    user1->set_age(62);
    user1->set_sex(User::WOMAN);

    User *user2 = rsp.add_friend_list();
    user2->set_name("yj");
    user2->set_age(22);
    user2->set_sex(User::MAN);

    std::cout<<rsp.friend_list_size()<<std::endl;

    return 0;
}



int main1()
{
    // 封装了login请求对象的数据
    fixbug::LoginRequest req;
    req.set_name("zz");
    req.set_pwd("zzpwd");

    // 对象数据序列化=》 char*
    std::string send_str;
    if(req.SerializeToString(&send_str))
    {
        std::cout<<send_str.c_str()<<std::endl;
    }

    // 从send_str反序列化一个login请求对象
    fixbug::LoginRequest reqB;
    if(reqB.ParseFromString(send_str))
    {
        std::cout<<reqB.name()<<std::endl;
        std::cout<<reqB.pwd()<<std::endl;
    }

    return 0;
}