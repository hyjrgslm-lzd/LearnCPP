#include <check.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <latch>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {

int shared_global = 0;

struct sample {
    std::uint64_t process_id = 0;
    std::uint64_t thread_id = 0;
    std::uintptr_t global_address = 0;
    std::uintptr_t tls_address = 0;
};

std::uint64_t current_process_id() {
#ifdef _WIN32
    return static_cast<std::uint64_t>(::GetCurrentProcessId());
#else
    return static_cast<std::uint64_t>(::getpid());
#endif
}

std::uint64_t current_thread_id() {
#ifdef _WIN32
    return static_cast<std::uint64_t>(::GetCurrentThreadId());
#else
    return static_cast<std::uint64_t>(::syscall(SYS_gettid));
#endif
}

void capture(sample& out, std::latch& ready, std::latch& release) {
    thread_local int per_thread = 0;
    out.process_id = current_process_id();
    out.thread_id = current_thread_id();
    out.global_address = reinterpret_cast<std::uintptr_t>(&shared_global);
    out.tls_address = reinterpret_cast<std::uintptr_t>(&per_thread);
    ready.count_down();
    release.wait();
}

} // namespace

int main() {
    std::array<sample, 2> samples{};
    std::latch ready(2);
    std::latch release(1);
    std::array<std::jthread, 2> threads;

    try {
        threads[0] = std::jthread(capture, std::ref(samples[0]), std::ref(ready), std::ref(release));
        threads[1] = std::jthread(capture, std::ref(samples[1]), std::ref(ready), std::ref(release));
    } catch (...) {
        release.count_down();
        throw;
    }

    ready.wait();
    check(samples[0].process_id == samples[1].process_id, "two jthreads share one process ID");
    check(samples[0].global_address == samples[1].global_address, "two jthreads share one global address");
    check(samples[0].thread_id != samples[1].thread_id, "two jthreads have different native thread IDs");
    check(samples[0].tls_address != samples[1].tls_address, "two live jthreads have different TLS addresses");

    std::cout << "process_id=" << samples[0].process_id << "\n";
    std::cout << "thread_ids=" << samples[0].thread_id << "," << samples[1].thread_id << "\n";
    std::cout << "shared_global_address=0x" << std::hex << samples[0].global_address << "\n";
    std::cout << "tls_addresses=0x" << samples[0].tls_address << ",0x" << samples[1].tls_address << std::dec << "\n";

    release.count_down();
    return 0;
}
