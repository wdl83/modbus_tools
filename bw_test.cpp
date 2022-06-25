#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

#include "Ensure.h"
#include "Master.h"
#include "json.h"

void help(const char *argv0, const char *message = nullptr)
{
    if(message) std::cout << "WARNING: " << message << '\n';

    std::cout
        << argv0
        << " -d device - input.json -t time_window_in_seconds"
        << std::endl;
}

void exec(const std::string &device, const Modbus::RTU::JSON::json &input, int t)
{
    using namespace Modbus;
    using namespace std::chrono;

    auto err = false;

    for(uint64_t i = 0;; ++i)
    {
        std::cout << "loop " << i << std::endl;
        try
        {
            Modbus::RTU::Master master{device.c_str()};

            auto timestamp = steady_clock::now();

            for(;;)
            {
                Modbus::RTU::JSON::json output;

                for(const auto &data : input)
                {
                    Modbus::RTU::JSON::dispatch(master, data, output);

                    if(err) err = false;
                    /* (silent interval) at least 3.5t character delay ~ 1750us @ 19200bps */
                    std::this_thread::sleep_for(std::chrono::microseconds(1750));
                }
                const auto now = steady_clock::now();
                const auto diff = now - timestamp;
                const auto diffInS = duration_cast<seconds>(diff);

                if(duration_cast<milliseconds>(diff) > milliseconds{1000 * t})
                {
                    const auto flags = std::cout.flags();
                    std::cout << "rx " << std::fixed << std::setprecision(0) << double(master.device().rxCntr() * 11) / diffInS.count() << "bps";
                    std::cout << " tx " << std::fixed << std::setprecision(0) << double(master.device().txCntr() * 11) / diffInS.count() << "bps";
                    std::cout << " rx_total " << std::fixed << std::setprecision(4) << double(master.device().rxTotalCntr() * 11) / (1024 * 1024) << "Mbit";
                    std::cout << " tx_total " << std::fixed << std::setprecision(4) << double(master.device().txTotalCntr() * 11) / (1024 * 1024) << "Mbit\n";
                    std::cout.flags(flags);

                    timestamp = now;
                    master.device().clearCntrs();
                }

                //std::cout << duration_cast<milliseconds>(diff).count() << "ms\n";
            }
        }
        catch(const RTU::TimeoutError &except)
        {
            if(err) break;
            err = true;
            std::this_thread::sleep_for(std::chrono::milliseconds{500});
        }
        catch(const RuntimeError &except)
        {
            if(err) break;
            err = true;
            std::this_thread::sleep_for(std::chrono::milliseconds{500});
        }
        catch(...)
        {
            TRACE(TraceLevel::Error, "unsupported exception");
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    std::string device, iname;
    int t = 1;

    for(int c; -1 != (c = ::getopt(argc, argv, "hd:i:t:"));)
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
            case 'i':
                iname = optarg ? optarg : "";
                break;
            case 't':
                t = optarg ? ::atoi(optarg) : 1;
                break;
            case ':':
            case '?':
            default:
                help(argv[0], "geopt() failure");
                return EXIT_FAILURE;
                break;
        }
    }

    if(device.empty() || iname.empty() || 0 >= t)
    {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        Modbus::RTU::JSON::json input;

        if("-" == iname) std::cin >> input;
        else std::ifstream(iname) >> input;

        ENSURE(input.is_array(), RuntimeError);

        exec(device, input, t);
    }
    catch(const std::exception &except)
    {
        std::cerr << except.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        std::cerr << "unsupported exception" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
