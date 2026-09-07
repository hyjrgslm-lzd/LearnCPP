#include "concurrency_study/bounded_channel.hpp"
#include "checks.hpp"

int main() { channel_checks::run<cs::bounded_channel>(); }
