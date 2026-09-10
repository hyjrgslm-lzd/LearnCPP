#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

int main()
{
    auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
    spdlog::logger logger{"diag", sink};
    logger.info("{:d}", "not an integer");
}
