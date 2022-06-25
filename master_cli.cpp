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
        <<
            " -d device"
            " -i input.json|-"
            " [-o output.json]"
            " [-r rate]"
            " [-p parity(O/E/N)]"
        << std::endl;
}

int main(int argc, char *argv[])
{
    std::string device, iname, oname, rate = "19200", parity = "E";

    for(int c; -1 != (c = ::getopt(argc, argv, "hd:i:o:r:p:"));)
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
            case 'o':
                oname = optarg ? optarg : "";
                break;
            case 'r':
                rate = optarg ? optarg : "";
                break;
            case 'p':
                parity = optarg ? optarg : "";
                break;
            case ':':
            case '?':
            default:
                help(argv[0], "geopt() failure");
                return EXIT_FAILURE;
                break;
        }
    }

    if(device.empty() || iname.empty())
    {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        Modbus::RTU::JSON::json input, output;

        if("-" == iname) std::cin >> input;
        else std::ifstream(iname) >> input;

        ENSURE(input.is_array(), RuntimeError);

        Modbus::RTU::Master master
        {
            device.c_str(),
            Modbus::toBaudRate(rate),
            Modbus::toParity(parity),
            Modbus::SerialPort::DataBits::Eight,
            Modbus::SerialPort::StopBits::One
        };

        for(const auto &i : input)
        {
            Modbus::RTU::JSON::dispatch(master, i, output);
            /* (silent interval) at least 3.5t character delay ~ 1750us @ 19200bps */
            std::this_thread::sleep_for(std::chrono::microseconds(1750 * 19200/9600));
        }

        if(oname.empty()) std::cout << output;
        else std::ofstream{oname} << output;
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
