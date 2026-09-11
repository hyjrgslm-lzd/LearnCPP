#include <boost/cobalt.hpp>
#include <boost/cobalt/channel.hpp>
#include <boost/cobalt/gather.hpp>
#include <boost/cobalt/main.hpp>
#include <boost/cobalt/race.hpp>

#include <iostream>

namespace cobalt = boost::cobalt;

struct channel_trace {
    int writes = 0;
    int reads = 0;
    int sum = 0;
};

cobalt::promise<void> producer(cobalt::channel<int>& ch, channel_trace& trace)
{
    // TODO: co_await ch.write(value); suspend instead of blocking when backpressured.
    // Write the varied values below and increment trace only after each write completes.
    (void)ch;
    (void)trace;
    co_return;
}

cobalt::promise<void> consumer(cobalt::channel<int>& ch, channel_trace& trace)
{
    // TODO: co_await ch.read(); keep this fixed-count starter before adding close/error policy.
    (void)ch;
    (void)trace;
    co_return;
}

cobalt::main co_main(int, char**)
{
    cobalt::channel<int> ch{0u};
    channel_trace trace;
    co_await cobalt::gather(producer(ch, trace), consumer(ch, trace));
    // TODO: add cobalt::race(...) for timeout after the basic pipeline works.
    if (trace.writes != 3 || trace.reads != 3 || trace.sum != 63) {
        std::cout << "student check failed: channel trace="
                  << trace.writes << "/" << trace.reads << "/" << trace.sum
                  << ", expected 3/3/63\n";
        co_return 1;
    }
    std::cout << "I4 student check passed.\n";
    co_return 0;
}
