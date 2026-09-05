#include <boost/cobalt.hpp>
#include <boost/cobalt/channel.hpp>
#include <boost/cobalt/gather.hpp>
#include <boost/cobalt/main.hpp>
#include <boost/cobalt/race.hpp>

#include <iostream>

namespace cobalt = boost::cobalt;

cobalt::promise<void> producer(cobalt::channel<int>& ch)
{
    (void)ch;
    // TODO: co_await ch.write(value); suspend instead of blocking when backpressured.
    co_return;
}

cobalt::promise<void> consumer(cobalt::channel<int>& ch)
{
    (void)ch;
    // TODO: co_await ch.read(); keep a fixed-count starter before adding close/error policy.
    co_return;
}

cobalt::main co_main(int, char**)
{
    cobalt::channel<int> ch{1u};
    (void)ch;
    // TODO: co_await cobalt::gather(producer(ch), consumer(ch)).
    // TODO: add cobalt::race(...) for timeout after the basic pipeline works.
    std::cout << "I4 Cobalt starter skeleton compiled. It posts no channel operations yet.\n";
    co_return 0;
}
