#include <algorithm>
#include <bits/chrono.h>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <future>
#include <iterator>
#include <random>
#include <thread>

#include "utest.h"
#include "PseudoSerial.h"


UTEST(SerialPort, open_and_close)
{
    auto pair =
        createPseudoPair(
            SerialPort::BaudRate::BR_9600, SerialPort::Parity::None,
            SerialPort::DataBits::Eight, SerialPort::StopBits::One);
    EXPECT_TRUE(true);
}

UTEST(SerialPort, write_then_read)
{
    auto pair =
        createPseudoPair(
            SerialPort::BaudRate::BR_9600, SerialPort::Parity::None,
            SerialPort::DataBits::Eight, SerialPort::StopBits::One);

    const uint8_t message[] = "hello on other side!";
    const auto timeout = std::chrono::milliseconds{100};

    {
        const auto i = pair.master.write(std::cbegin(message), std::cend(message), timeout);
        EXPECT_TRUE(i == std::cend(message));
    }

    {
        uint8_t buf[255] = {};
        const auto i = pair.slave.read(std::begin(buf), std::cend(buf), timeout);
        EXPECT_TRUE(size_t(i - std::cbegin(buf)) == std::size(message));
        EXPECT_TRUE(0 == memcmp(message, buf, std::size(message)));
    }
}

UTEST(SerialPort, async_read_and_write)
{
    auto pair =
        createPseudoPair(
            SerialPort::BaudRate::BR_9600, SerialPort::Parity::None,
            SerialPort::DataBits::Eight, SerialPort::StopBits::One);

    const uint8_t message[] = "hello on other side!";
    const auto timeout = std::chrono::milliseconds{100};

    auto receiver =
        std::async(
            std::launch::async,
            [&]()
            {
                uint8_t buf[255] = {};
                const auto i = pair.slave.read(std::begin(buf), std::cend(buf), timeout);
                EXPECT_TRUE(i - std::cbegin(buf) == std::size(message));
                EXPECT_TRUE(0 == memcmp(message, buf, std::size(message)));
            });

    const auto i = pair.master.write(std::cbegin(message), std::cend(message), timeout);
    EXPECT_TRUE(i == std::cend(message));
}

UTEST(SerialPort, async_echo)
{
    using namespace std::chrono;
    const uint8_t stopMessage[] = "STOP";
    auto pair =
        createPseudoPair(
            SerialPort::BaudRate::BR_115200, SerialPort::Parity::None,
            SerialPort::DataBits::Eight, SerialPort::StopBits::One);

    auto receiver =
        std::thread{
            [&]()
            {
                std::random_device rdev;
                std::mt19937 rgen{rdev()};
                std::uniform_int_distribution<> dist{5, 30};

                for(;;)
                {
                    uint8_t buf[32] = {};
                    const auto i = pair.slave.read(std::begin(buf), std::cend(buf), milliseconds{dist(rgen)});

                    if(std::cbegin(buf) == i) continue;

                    EXPECT_TRUE(pair.slave.write(std::cbegin(buf), i, milliseconds{dist(rgen)}) == i);
                    static_assert(std::size(buf) >= std::size(stopMessage));
                    // exit receiver thread if "STOP" message received
                    if(0 == memcmp(stopMessage, buf, std::size(stopMessage))) break;
                }
            }};

    const auto timeout = milliseconds{10};
    const uint8_t message[] = "*** hello on other side! ?\t? \n12 _ $ test \n message *** STOP";
    const auto end = std::cend(message);

    // send 1 word at a time
    for(auto i = std::cbegin(message); i != end;)
    {
        // [i, j) - contains single word
        auto j = std::find(i, end, ' ');
        const auto len = j - i;
        ASSERT_TRUE(i != j);
        EXPECT_TRUE(pair.master.write(i, j, timeout) == j);

        // receive echo and check if it matches
        for(;;)
        {
            uint8_t buf[32] = {};
            ASSERT_TRUE(std::size(buf) >= size_t(len));
            const auto l = pair.master.read(std::begin(buf), std::cend(buf), timeout);
            if(std::cbegin(buf) == l) continue;
            EXPECT_TRUE(l - std::cbegin(buf) == len);
            EXPECT_TRUE(0 == memcmp(i, buf, len));
            break;
        }

        i = j;

        while(i != end && std::isspace(*i)) ++i;
    }

    receiver.join();
}

UTEST(SerialPort, read_timeout)
{
    using namespace std::chrono;
    auto pair =
        createPseudoPair(
            SerialPort::BaudRate::BR_9600, SerialPort::Parity::None,
            SerialPort::DataBits::Eight, SerialPort::StopBits::One);

    const auto timeout = milliseconds{20};
    const auto start = steady_clock::now();
    uint8_t buf[32] = {};
    EXPECT_TRUE(std::cbegin(buf) == pair.master.read(std::begin(buf), std::cend(buf), timeout));
    const auto elapsed = steady_clock::now() - start;
    EXPECT_TRUE(duration_cast<milliseconds>(elapsed) >= timeout);
}

UTEST_MAIN();
