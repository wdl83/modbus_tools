#include <cstdlib>
#include <iomanip>
#include <iostream>

#include <unistd.h>

#include <nlohmann/json.hpp>

#include "Ensure.h"

namespace {

const char *const ADDR = "addr";
const char *const SLAVE = "slave";
const char *const VALUE = "value";

template <typename T, typename V>
bool inRange(V value)
{
    return
        std::numeric_limits<T>::min() <= value
        && std::numeric_limits<T>::max() >= value;
}

using json = nlohmann::json;

void tlog_dump(const json &input)
{
    if(!input.count(ADDR)) return;

    ENSURE(input[ADDR].is_number(), RuntimeError);

    const auto addr = input[ADDR].get<int>();

    ENSURE(inRange<uint16_t>(addr), RuntimeError);

    if(!input.count(VALUE)) return;

    ENSURE(input[VALUE].is_array(), RuntimeError);

    const auto data = input[VALUE].get<std::vector<int>>();

    if(input.count(SLAVE))
    {
        ENSURE(input[SLAVE].is_number(), RuntimeError);
    }

    for(auto i : data) std::cout << char(i);
}

void parse(const json &input)
{
    if(input.is_array())
    {
        for(const auto &i : input) parse(i);
    }
    else tlog_dump(input);
}

}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    json input;

    std::cin >> input;

    parse(input);

    std::cout << std::endl;

    return EXIT_SUCCESS;
}
