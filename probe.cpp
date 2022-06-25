#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

#include "Ensure.h"
#include "Except.h"
#include "Master.h"
#include "json.h"

void help(const char *argv0, const char *message = nullptr)
{
    if(message) std::cout << "WARNING: " << message << '\n';

    std::cout
        << argv0
        << " -d device"
        << " [-s slave]"
        << std::endl;
}

int main(int argc, char *argv[])
{
    std::string device;
    int slave = -1;

    for(int c; -1 != (c = ::getopt(argc, argv, "hd:s:"));)
    {
        switch(c)
        {
            case 'h':
                help(argv[0]);
                return EXIT_SUCCESS;
                break;
            case 'd':
                device = optarg ? optarg : "";
                break;
            case 's':
                slave = optarg ? ::atoi(optarg) : -1;
                break;
            case ':':
            case '?':
            default:
                help(argv[0], "geopt() failure");
                return EXIT_FAILURE;
                break;
        }
    }

    if(device.empty() || -1 != slave && (slave < 0 || slave > 255))
    {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        using namespace Modbus;
        RTU::Master master{device.c_str()};
        const auto begin = -1 == slave ? 1 : slave;
        const auto end = -1 == slave ? 256 : slave + 1;

        for(auto i = begin; i != end; ++i)
        {
            RTU::JSON::json input
            {
                {"slave", i},
                {"fcode", 3}, /* READ HOLDING REGISTERS */
                {"addr", 0},
                {"count", 1},
            };

            RTU::JSON::json output;

            try
            {
                TRACE(TraceLevel::Info, "slave ", int(i));
                Modbus::RTU::JSON::dispatch(master, input, output);
            }
            catch(const RTU::TimeoutError &except)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds{25});
            }
            catch(const RTU::ReplyError &except)
            {
                TRACE(TraceLevel::Info, "reply error ", except.what(), " from ", int(i));
                std::this_thread::sleep_for(std::chrono::milliseconds{25});
            }
            catch(const RuntimeError &except)
            {
                TRACE(TraceLevel::Warning, "unexpected runtime error ", except.what());
                std::this_thread::sleep_for(std::chrono::milliseconds{100});
            }
            catch(...)
            {
                TRACE(TraceLevel::Error, "unsupported exception addr ", i);
                break;
            }
        }

    }
    catch(const std::exception &except)
    {
        TRACE(TraceLevel::Error, except.what());
        return EXIT_FAILURE;
    }
    catch(...)
    {
        TRACE(TraceLevel::Error, "unsupported exception");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
