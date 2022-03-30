#pragma once
#include <google/protobuf/service.h>

#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>

#include <unordered_map>
#include <string>
#include <google/protobuf/descriptor.h>

class MpzrpcProvider
{
public:
    void publishService(::google::protobuf::Service *service);

    void run();

    void onConnectionCallback(const muduo::net::TcpConnectionPtr &conn)
    {
        if (!conn->connected())
        {
            // 和rpc client的连接断开了
            conn->shutdown();
        }
    };

    void onMessageCallback(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buffer,
                           muduo::Timestamp receiveTime);

    void SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response);

private:
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodmap;
    };

    std::unordered_map<std::string, ServiceInfo> m_servicemap;
};