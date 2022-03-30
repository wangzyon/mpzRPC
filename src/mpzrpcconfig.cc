#include "mpzrpcconfig.h"
#include <nlohmann/json.hpp>
#include <fstream>

void MpzrpcConfig::LoadConfigFromFile(const std::string &config_file)
{
    std::ifstream i(config_file);
    nlohmann::json j;
    i >> j;

    m_rpcserverip = j["rpcserverip"];
    m_rpcserverport = j["rpcserverport"];
    m_zookeeperip = j["zookeeperip"];
    m_zookeeperport = j["zookeeperport"];
    m_muduoThreadNum = j["muduothreadnum"];
}