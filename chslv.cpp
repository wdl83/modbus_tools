#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <nlohmann/json.hpp>

#include "Ensure.h"

namespace {

void help(const char *argv0, const char *message = nullptr)
{
    if(message) std::cout << "WARNING: " << message << '\n';

    std::cout
        << argv0
        << " -s slave"
        << std::endl;
}

using json = nlohmann::json;

const char *const SLAVE = "slave";

void swap(json &input, int slave)
{
    if(input.is_object() && input.count(SLAVE))
    {
        ENSURE(input[SLAVE].is_number(), RuntimeError);
        input[SLAVE] = slave;
    }
    else if(input.is_array())
    {
        for(auto &i : input)
        {
            swap(i, slave);
        }
    }
    else ENSURE(false && "unsupported json type", RuntimeError);
}

} /* namespace */

int main(int argc, char *argv[])
{
    int slave = -1;

    for(int c; -1 != (c = ::getopt(argc, argv, "hs:"));)
    {
        switch(c)
        {
            case 'h':
                help(argv[0]);
                return EXIT_SUCCESS;
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

    if(0 > slave)
    {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        json data;

        std::cin >> data;
        swap(data, slave);
        std::cout << data.dump();
    }
    catch(const RuntimeError &except)
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
