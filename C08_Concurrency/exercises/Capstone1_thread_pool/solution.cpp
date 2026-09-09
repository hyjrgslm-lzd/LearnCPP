#include "concurrency_study/thread_pool.hpp"
#include "checks.hpp"

int main() { pool_checks::run<cs::thread_pool>(); }
