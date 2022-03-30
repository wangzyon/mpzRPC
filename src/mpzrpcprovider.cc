#include "mpzrpcprovider.h"
#include "mpzrpcapplication.h"
#include <stdlib.h>
#include <functional>
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

void MpzrpcProvider::run()
{
    // 读取配置文件rpcserver的信息
    std::string ip = MpzrpcApplication::getApp().getConfig().getRpcServerIp();
    uint16_t port = MpzrpcApplication::getApp().getConfig().getRpcServerPort();
    int muduoThreadum = MpzrpcApplication::getApp().getConfig().getMuduoThreadNum();

    // 创建TcpServer对象
    muduo::net::InetAddress address(ip, port);
    muduo::net::EventLoop loop;
    muduo::net::TcpServer server(&loop, address, "RpcProvider");

    // 绑定连接回调和消息读写回调方法  分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&MpzrpcProvider::onConnectionCallback, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&MpzrpcProvider::onMessageCallback,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3));
    // 设置muduo库的线程数量
    server.setThreadNum(muduoThreadum);

    ZkClient zkCli;
    zkCli.Start();
    for (auto &sp : m_servicemap)
    {
        // /service_name   /UserServiceRpc
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodmap)
        {
            // /service_name/method_name   /UserServiceRpc/Login 存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // rpc服务端准备启动，打印信息
    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

    // 启动subEventLoop用户事件处理服务，启动网络服务接受用户连接
    server.start();
    loop.loop();
};

void MpzrpcProvider::publishService(::google::protobuf::Service *service)
{
    ServiceInfo servic_info;

    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *service_des = service->GetDescriptor();
    // 获取服务的名字
    std::string service_name = service_des->name();
    // 获取服务对象service的方法的数量
    int methodnum = service_des->method_count();
    for (int i = 0; i < methodnum; ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor *method_des = service_des->method(i);
        servic_info.m_methodmap.insert({method_des->name(), method_des});
    }

    servic_info.m_service = service;
    m_servicemap.insert({service_name, servic_info});
};

void MpzrpcProvider::onMessageCallback(const muduo::net::TcpConnectionPtr &conn,
                                      muduo::net::Buffer *buffer,
                                      muduo::Timestamp receiveTime)
{
    /*
        约定通信协议

        header_size  |    header string    |   args string
           4 bytes   |  header_size bytes  |  args_size bytes
        */
    std::cout << "trigger tcpconnection onMessage callback" << std::endl;
    // 取网络发送过来的调用rpc指令
    std::string recv_str = buffer->retrieveAllAsString();

    std::cout << "recv_str: " << recv_str << "##" << std::endl;

    // 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    recv_str.copy((char *)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string header_str = recv_str.substr(4, header_size);
    rpcheader::rpcheader header;
    if (!header.ParseFromString(header_str))
    {
        LOG_ERR("%s", "header str deserialization failed");
        return;
    }

    std::string service_name = header.service_name();
    std::string method_name = header.method_name();
    uint32_t args_size = header.args_size();

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_str.substr(4 + header_size, args_size);
    auto service_it = m_servicemap.find(service_name);
    if (service_it == m_servicemap.end())
    {
        LOG_ERR("%s", "unknown service");
        return;
    }

    // 获取method描述符
    ServiceInfo service_info = service_it->second;
    auto method_it = service_info.m_methodmap.find(method_name);
    if (method_it == service_info.m_methodmap.end())
    {
        LOG_ERR("%s", "unknown method");
        return;
    }

    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "============================================" << std::endl;

    // 生成rpc方法调用的请求request和响应response参数
    const google::protobuf::MethodDescriptor *method_des = method_it->second;
    google::protobuf::Message *request = service_info.m_service->GetRequestPrototype(method_des).New();
    if (!request->ParseFromString(args_str))
    {
        LOG_ERR("%s", "message request deserialization failed");
        return;
    }
    google::protobuf::Message *response = service_info.m_service->GetResponsePrototype(method_des).New();

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure *done = google::protobuf::NewCallback<MpzrpcProvider,
                                                                    const muduo::net::TcpConnectionPtr &,
                                                                    google::protobuf::Message *>(this,
                                                                                                 &MpzrpcProvider::SendRpcResponse,
                                                                                                 conn,
                                                                                                 response);

    // Service类callmethod通过method des找到Service重载的对应方法，并传递controller/request/response/done                                                                             conn, response);
    service_info.m_service->CallMethod(method_des, nullptr, request, response, done);
};

void MpzrpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string send_str;
    if (!response->SerializeToString(&send_str))
    {
        LOG_ERR("%s", "message response serialization failed");
    }
    else
    {
        // 序列化成功后，通过网络把rpc方法执行的结果发送会rpc的调用方
        conn->send(send_str);
    }
    conn->shutdown(); // 模拟http的短链接服务，由rpcprovider主动断开连接
}