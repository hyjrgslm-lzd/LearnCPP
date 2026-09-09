#include <boost/asio/steady_timer.hpp>
#include <boost/cobalt.hpp>
#include <boost/cobalt/channel.hpp>
#include <boost/cobalt/gather.hpp>
#include <boost/cobalt/main.hpp>
#include <boost/cobalt/race.hpp>
#include <coroutine_study/exercise_check.hpp>

#include <chrono>
#include <iostream>
#include <vector>

namespace asio = boost::asio;
namespace cobalt = boost::cobalt;

cobalt::promise<void> delay(std::chrono::milliseconds ms)
{
    asio::steady_timer timer{co_await cobalt::this_coro::executor};
    timer.expires_after(ms);
    co_await timer.async_wait(cobalt::use_op);
}

cobalt::promise<void> producer(cobalt::channel<int>& ch)
{
    co_await ch.write(1);
    co_await ch.write(2);
    co_await ch.write(3);
}

cobalt::promise<std::vector<int>> consumer(cobalt::channel<int>& ch)
{
    std::vector<int> values;
    for (int i = 0; i != 3; ++i) {
        values.push_back(co_await ch.read());
    }
    co_return values;
}

cobalt::main co_main(int, char**)
{
    cobalt::channel<int> ch{0u};
    auto c = consumer(ch);
    co_await cobalt::gather(producer(ch), delay(std::chrono::milliseconds{1}));
    auto values = co_await c;

    coroutine_study::check(values == std::vector<int>{1, 2, 3}, "channel values mismatch");

    auto winner = co_await cobalt::race(
        delay(std::chrono::milliseconds{30}),
        delay(std::chrono::milliseconds{1}));
    coroutine_study::check(winner == 1, "race winner mismatch");

    std::cout << "I4 reference passed: channel symmetric handoff, gather, race\n";
    co_return 0;
}
