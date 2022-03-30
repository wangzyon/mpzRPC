#pragma once

#include <iostream>
#include <string>

class MpzrpcConfig
{
public:
    void LoadConfigFromFile(const std::string &config_file);

    std::string &getRpcServerIp() { return m_rpcserverip; };
    int &getRpcServerPort() { return m_rpcserverport; };
    std::string &getZooKeeperIp() { return m_zookeeperip; };
    int &getZooKeeperPort() { return m_zookeeperport; };
    int &getMuduoThreadNum() { return m_muduoThreadNum; };

private:
    std::string m_rpcserverip;
    int m_rpcserverport;
    std::string m_zookeeperip;
    int m_zookeeperport;
    int m_muduoThreadNum;
};