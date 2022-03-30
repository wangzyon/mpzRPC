#include "mpzrpcapplication.h"
#include <unistd.h>
#include <string>

MpzrpcApplication &MpzrpcApplication::getApp()
{
    static MpzrpcApplication app;
    return app;
}

void MpzrpcApplication::init(int argc, char **argv)
{
    if (argc < 2)
    {
        showArgsHelp();
        exit(EXIT_FAILURE);
    }
    else
    {
        int o;
        std::string config_file;
        const char *optstring = "c:";
        while ((o = getopt(argc, argv, optstring)) != -1)
        {
            switch (o)
            {
            case 'c':
                config_file = optarg;
                break;

            default:
                break;
            }
        }
        getConfig().LoadConfigFromFile(config_file);
    }
};

MpzrpcConfig &MpzrpcApplication::getConfig()
{
    static MpzrpcConfig m_config;
    return m_config;
};