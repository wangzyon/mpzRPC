#include <string>
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mpzrpcchannel.h"
#include "logger.h"
#include "rpcheader.pb.h"
#include "mpzrpcapplication.h"
#include "zookeeperutil.h"

void MpzrpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller,
                              const google::protobuf::Message *request,
                              google::protobuf::Message *response,
                              google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *service_des = method->service();
    std::string service_name = service_des->name();
    std::string method_name = method->name();

    std::string args_str;
    if (!request->SerializeToString(&args_str))
    {
        LOG_ERR("%s", "message request serialization failed");
        return;
    }

    rpcheader::rpcheader header;
    header.set_service_name(service_name);
    header.set_method_name(method_name);
    header.set_args_size(args_str.size());

    std::string header_str;
    if (!header.SerializeToString(&header_str))
    {
        LOG_ERR("%s", "message header_str serialization failed");
        return;
    }

    uint32_t header_size = header_str.size();
    std::string send_str;
    send_str.insert(0, std::string((char *)&header_size, 4));
    send_str += header_str + args_str;

    // 使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    ZkClient zkCli;
    zkCli.Start();
    // 获取待调用的服务名称 /UserServiceRpc/Login
    std::string method_path = "/" + service_name + "/" + method_name;
    // 获取服务对应的url
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if (-1 == connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    std::cout << "connect success" << std::endl;

    // 发送rpc请求
    if (-1 == send(clientfd, send_str.c_str(), send_str.size(), 0))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 反序列化rpc调用的响应数据
    // std::string response_str(recv_buf, 0, recv_size); //question bug出现问题，recv_buf中遇到\0后面的数据就存不下来了，导致反序列化失败
    // if (!response->ParseFromString(response_str))
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }

    close(clientfd);
}