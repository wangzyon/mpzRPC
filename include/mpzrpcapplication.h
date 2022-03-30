#pragma once

#include <iostream>
#include "mpzrpcconfig.h"

class MpzrpcApplication
{
public:
    static void init(int argc, char **argv);

    static void showArgsHelp()
    {
        std::cout << "format: command -c <json config file>\n";
    }

    static MpzrpcApplication &getApp();
    static MpzrpcConfig &getConfig();

private:
    MpzrpcApplication(){};
    MpzrpcApplication(const MpzrpcApplication &) = delete;
    MpzrpcApplication(MpzrpcApplication &&) = delete;
};