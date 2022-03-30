# mpzRPC - An RPC library and framework

mpzRPC是基于C++实现的RPC分布式网络通信框架，框架基于muduo高性能网络库、protobuf数据交换、Zookeepr服务注册中间件开发，即mpzRPC。mpzRPC在客户端隐藏了通信和连接过程，在服务端提供了简洁本地服务发布注册接口，整体高效易用。

# 1 概述

## 1.1 设计思路 

protofbuf原生提供了基于rpc service方法调用代码框架，无需实现对服务接口的描述，只需要关注服务管理和rpc通信流程；其次，protobuf基于二进制序列化数据的，同带宽具有更高的通信效率；因此，采用protobuf为基础，整体框架初始构想如下；

![](images/design_init.svg)

灰色区域由protobuf提供，红色区域是框架的待开发部分：

- 服务映射表：记录服务端发布的本地服务名称和服务对象；当客户端发起调用时，输入RPC方法名->查表->取服务对象->调用；
- 服务注册中间件：服务端在将发布的服务进行注册，客户端根据服务、方法名从服务注册中间件获取服务所在机器的URL，从而定位通信对象；
- 网络中间件：RPC请求的发送和服务调用响应的返回，需要网络中间件实现客户端和服务端的通信；
- 通信协议：客户端调用一个远程RPC服务，需要提供服务名称、方法名称、方法输入参数，因此客户端打包这些参数和服务端解包需要提前约定一个通信协议；

## 1.2 框架设计

![](images/design.svg)

- 服务映射表：由provider类->publicService实现本地服务发布为RPC服务；
- 服务注册中间件：利用Zookeeper的服务注册功和发现能，具有两方便优势，其一，客户端和服务端解耦，客户端无需手动修改服务URL配置文件；其二，提供者和服务中心间存在心跳检测和服务协调，实时检测更新提供服务的URL；
- 网络中间件：采用muduo网络库实现通信，muduo库采用one loop per thread设计，多线程并发执行多个事件循环，每个事件循环采用非阻塞+epoll作为IO模型，性能优秀；
- 通信协议：head_size+head_str+request_str设计，其中head_str中包含request_size信息，避免Tcp粘包；

## 1.3 交互流程

1. 发布本地服务，在服务端，通过provider->publishService将本地服务封装成可被远程调用的rpc服务，并添加到服务映射表；
2. 启动服务，provider->run启动TcpServer,并创建一个监听fd->acceptFd；将服务映射表上的RPC服务注册到ZooKeeper;
3. 客户端发起调用，根据目标服务名、方法名，从ZooKeeper上获取RPC方法所在URL；按通信协议打包数据包，序列化后通过clientFd连接TcpServer,发送数据包；
4. 建立连接，当客户端连接acceptFd成功时，acceptFd返回connFd与clientFd组成TCP链路，并创建对应TcpConnection对象；
5. 服务端接收数据，客户端发送数据包写入服务端connFd读缓冲区时，触发TcpConnection对象的onMessageCallback回调；
6. 服务端解包，在onMessageCallback回调中，反序列化后，按通信协议解包，通过包中rpc方法名查服务映射表获取对应service、MethodDescribe对象,通过包中request_str构建Request对象；
7. 服务端绑定回调，将provider的sendResponse设置为callMethod中的closure关闭回调对象；
8. 服务端调用，在onMessageCallback回调中，service对象->callMethod->执行rpc method->执行local method->返回值写入Response对象->触发关闭回调sendResponse；
9. 服务端返回，sendResponse中将response序列化后返回客户端，客户端clientFd通过recv接收并反序列化后从response对象中获取调用结果，至此完成一轮rpc调用，关闭Tcp连接；

# 2 安装

## 2.1 安装依赖库

- [protobuf](https://github.com/protocolbuffers/protobuf)
- [ZooKeeper](https://github.com/apache/zookeeper)
- [muduo](https://github.com/chenshuo/muduo)
- [cmake](https://github.com/Kitware/CMake)
- [json](https://github.com/nlohmann/json)

## 2.2 编译

```bash
# 克隆


# 编译
cd ./mpzRPC
./autobuild.sh
```

# 3 使用

## 3.1 服务端发布RPC服务

## 3.2 客户端调用RPC服务

