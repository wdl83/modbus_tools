#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <thread>

#include "Ensure.h"
#include "Master.h"
#include "json.h"

namespace {

void help(const char *argv0, const char *message = nullptr)
{
    if(message) std::cout << "WARNING: " << message << '\n';

    std::cout
        << argv0
        << " -d device"
        << " -t (ASCII only)"
        << std::endl;
}

void dump(std::ostream &os, const uint8_t *begin, const uint8_t *const end)
{
    const auto *hexCurr = begin;
    const auto *asciiCurr = begin;

    while(hexCurr != end && asciiCurr != end)
    {
        int hexCntr = 0;
        const auto flags = os.flags();

        while(hexCurr != end && 16 > hexCntr)
        {
            os << std::hex << std::setw(2) << std::setfill('0') << int(*hexCurr);
            ++hexCurr;
            ++hexCntr;

            if(16 > hexCntr) os << ' ';
        }

        os.flags(flags);

        std::fill_n(std::ostream_iterator<char>(os), std::max(0, (16 - hexCntr) * 3 - 1) + 4, ' ');
        os << " | ";

        int asciiCntr = 0;

        while(asciiCurr != end && 16 > asciiCntr)
        {
            if(31 < *asciiCurr && 128 > *asciiCurr) os << static_cast<char>(*asciiCurr);
            else os << '?';
            ++asciiCurr;
            ++asciiCntr;
        }

        if(0 == (hexCntr % 16) || 0 == (asciiCntr  % 16)) os << '\n';
    }
    os << '\n';
}

void exec(const std::string &device, bool hex)
{
    using namespace Modbus;
    using namespace std::chrono;
    bool stop = false;

    while(!stop)
    {
        SerialPort serialPort
        {
            device,
            SerialPort::BaudRate::BR_19200,
            SerialPort::Parity::Even,
            SerialPort::DataBits::Eight,
            SerialPort::StopBits::One
        };

        try
        {
            for(;;)
            {
                std::vector<uint8_t> data(256, uint8_t{0});

                const auto curr =
                    serialPort.read(
                        data.data(), data.data() + data.size(),
                        milliseconds{1000});

                if(curr != data.data())
                {
                    if(hex) dump(std::cout, data.data(), curr);
                    else
                    {
                        for(auto begin = data.data(); begin != curr; ++begin)
                        {
                            std::cout << *begin;
                        }
                    }
                }

            }
        }
        catch(const RuntimeError &except)
        {
            TRACE(TraceLevel::Error, except.what());
        }
        catch(...)
        {
            TRACE(TraceLevel::Error, "unsupported exception");
            stop = true;
        }
    }
}

} /* namespace */

int main(int argc, char *argv[])
{
    std::string device;
    bool hex = true;

    for(int c; -1 != (c = ::getopt(argc, argv, "htd:"));)
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
            case 't':
                hex = false;
                break;
            case ':':
            case '?':
            default:
                help(argv[0], "geopt() failure");
                return EXIT_FAILURE;
                break;
        }
    }

    if(device.empty())
    {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        exec(device, hex);
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
