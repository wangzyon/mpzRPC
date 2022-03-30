#include <iostream>
#include "example.service.pb.h"
#include "mpzrpcapplication.h"
#include "mpzrpcprovider.h"

class UserService : public example::UserRpcService
{
public:
    bool Login(const std::string &name, const std::string pwd)
    {
        std::cout << "local service: Login" << std::endl;
        std::cout << "name:" << name << "pwd" << std::endl;
        return pwd == "123";
    }

    // Closure关闭，终止
    void Login(::google::protobuf::RpcController *controller,
               const ::example::LoginRequest *request,
               ::example::LoginResponse *response,
               ::google::protobuf::Closure *done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool ret = Login(name, pwd);
        response->set_success(ret);

        // 把响应写入  包括错误码、错误消息、返回值
        example::ResultCode *result_code = response->mutable_result();
        result_code->set_errcode(0);
        result_code->set_errmsg("");

        // 执行回调操作   执行响应对象数据的序列化和网络发送（都是由框架来完成的）
        done->Run();
    };
};

int main(int argc, char **argv)
{
    MpzrpcApplication::init(argc, argv);
    std::cout << MpzrpcApplication::getApp().getConfig().getRpcServerIp() << std::endl;
    MpzrpcProvider provider;
    provider.publishService(new UserService());
    provider.run();

    return 0;
};