#include"mprpcconfig.h"
#include<iostream>
#include <string.h>
// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *fp = fopen(config_file,"r");
    if(fp==nullptr)
    {
        std::cout<<"configfile not exist!"<<std::endl;
        exit(EXIT_FAILURE);
    }
    while(!feof(fp))
    {
        char buf[512]={0};
        fgets(buf,512,fp);
        // printf("%d,%s",strlen(buf),buf);    //检查到最后有一个换行符\n需要删掉。
        
        /////////////////////////////////////////////////////////
        // 保证读取配置文件的健壮性
        // 1.注释 2.正确的配置项 =   3.去掉开头结尾多余的空格

        // 去掉最后的换行符
        std::string string_buf = std::string(buf);
        if(string_buf.find('\n')!=-1)   //有换行那么就删除。
            string_buf = string_buf.substr(0,string_buf.size()-1);
        
        Trim(string_buf);

        //去掉注释和空白行
        if(string_buf[0]=='#'||string_buf.empty())  continue;   //说明是注释或者空白行，跳过不进行加载处理。

        // 解析配置项
        int idx = string_buf.find('=');
        if(idx==-1) continue;   //配置不合法
        /////////////////////////////////////////////////////////
        // 开始解析配置参数
        std::string key;
        std::string value;
        key = string_buf.substr(0,idx);
        value = string_buf.substr(idx+1,string_buf.size());
        Trim(key);  Trim(value);    //去掉kv两边的空格。

// std::cout<<"|"<<key<<":"<<value<<"|"<<std::endl;
        m_configMap.insert({key,value});
    }
    // // map容器测试
    // for(auto aa:m_configMap)
    //     std::cout<<aa.first<<":"<<aa.second<<std::endl;
}

// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if(it==m_configMap.end()) return "";    //没找到就return空
    return it->second;
}

// 去掉字符串前后多余的空格
void MprpcConfig::Trim(std::string &string_buf)
{
    // 去掉开头的空格
    int idx = string_buf.find_first_not_of(' ');
    if(idx!=-1)
    {
        string_buf = string_buf.substr(idx,string_buf.size());
    }

    //去掉结尾的空格
    idx = string_buf.find_last_not_of(' ');
    if(idx!=-1)
    {
        string_buf = string_buf.substr(0,idx+1);
    }
}