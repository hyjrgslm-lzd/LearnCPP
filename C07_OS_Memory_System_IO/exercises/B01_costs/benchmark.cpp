#include <c07/file_pipeline_io.hpp>
#include <pipeline.hpp>
#include <pmr_lab.hpp>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <stdexcept>
#include <vector>
#ifndef _WIN32
#include <time.h>
#endif

namespace {
using bench_clock = std::chrono::steady_clock;
double elapsed(bench_clock::time_point start) { return std::chrono::duration<double>(bench_clock::now() - start).count(); }
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
std::size_t number(const char* input) {
    std::string_view text{input};
    std::size_t result{};
    auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) throw std::invalid_argument("invalid integer");
    return result;
}

struct scratch_files {
    std::filesystem::path directory;
    std::filesystem::path input;
    std::filesystem::path output;
    scratch_files() {
        directory = std::filesystem::temp_directory_path() /
            ("c07-costs-" + std::to_string(bench_clock::now().time_since_epoch().count()));
        input = directory / "input.bin";
        output = directory / "output.bin";
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("fixture already exists");
    }
    ~scratch_files() noexcept {
        int failure = 0;
        for (const auto* path : {&output, &input, &directory}) {
            std::error_code error;
            std::filesystem::remove(*path, error);
            if (error) failure = error.value();
        }
        if (failure) { std::fprintf(stderr, "benchmark fixture cleanup failed: %d\n", failure); std::_Exit(71); }
    }
};

struct io_result {
    double seconds{}, setup_seconds{}, read_seconds{}, assemble_seconds{}, write_seconds{};
    std::size_t bytes{}, read_calls{}, completions{}, peak_in_flight{}, application_copy_bytes{};
};

io_result io_case(std::string_view variant, std::size_t size, std::size_t rounds, std::size_t depth) {
    require(size > 0 && size <= 8 * 1024 * 1024 && rounds > 0 && rounds <= 16, "bounded IO benchmark inputs required");
    c07::file_backend backend;
    if (variant == "buffered") backend = c07::file_backend::buffered;
    else if (variant == "mapped") backend = c07::file_backend::mapped;
    else if (variant == "completion") backend = c07::file_backend::completion;
    else throw std::invalid_argument("unknown IO variant");
    scratch_files files;
    std::vector<std::byte> original(size);
    std::uint32_t state = 1729;
    for (auto& byte : original) { state = state * 1664525u + 1013904223u; byte = std::byte(state >> 24); }
    auto created = c07::write_new_file(files.input, original);
    if (!created) throw std::system_error(created.error(), "create benchmark source");
    io_result result;
    for (std::size_t round = 0; round < rounds; ++round) {
        if (round) require(std::filesystem::remove(files.output), "remove previous owned output");
        const auto started = bench_clock::now();
        auto batch = c07::read_file_chunks(files.input, backend, 65536, depth);
        if (!batch) throw std::system_error(batch.error(), "read_file_chunks");
        const auto assembling = bench_clock::now();
        auto output = c07_p1::assemble(batch->chunks, batch->total_bytes);
        if (!output) throw std::system_error(output.error(), "assemble");
        const auto assemble_time = elapsed(assembling);
        const auto writing = bench_clock::now();
        auto written = c07::write_new_file(files.output, *output);
        if (!written) throw std::system_error(written.error(), "write_new_file");
        const auto write_time = elapsed(writing);
        result.seconds += elapsed(started);
        result.setup_seconds += batch->metrics.setup_seconds;
        result.read_seconds += batch->metrics.read_seconds;
        result.assemble_seconds += assemble_time;
        result.write_seconds += write_time;
        require(output->size() == size && output->front() == original.front()
            && output->back() == original.back(), "lightweight timed-round integrity");
        result.bytes += output->size();
        result.read_calls += batch->metrics.read_calls;
        result.completions += batch->metrics.completions;
        result.peak_in_flight = std::max(result.peak_in_flight, batch->metrics.peak_in_flight);
        result.application_copy_bytes += batch->metrics.application_copy_bytes;
    }
    // Full validation is outside timing. The same reader/assembler/sink code is
    // independently exercised by P1; every process also verifies its final file.
    std::ifstream stream(files.output, std::ios::binary);
    std::vector<char> actual{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    require(actual.size() == original.size(), "full output size");
    require(std::equal(actual.begin(), actual.end(), original.begin(), [](char a, std::byte b) {
        return static_cast<unsigned char>(a) == std::to_integer<unsigned char>(b);
    }), "full output bytes");
    return result;
}

struct allocation_result {
    double seconds{}, setup_seconds{}, loop_seconds{}, cleanup_seconds{};
    std::size_t allocations{}, deallocations{}, upstream_bytes{}, peak_bytes{}, observed_address_reuse{};
    std::uint64_t checksum{};
};

allocation_result allocation_case(std::string_view variant, std::size_t items) {
    require(items > 0 && items <= 65536, "bounded allocation benchmark inputs required");
    c07_l05::counting_resource upstream;
    allocation_result result;
    const auto started = bench_clock::now();
    auto run = [&](std::pmr::memory_resource& resource) {
        result.setup_seconds = elapsed(started);
        const auto loop = bench_clock::now();
        std::uintptr_t previous = 0;
        for (std::size_t i = 0; i < items; ++i) {
            void* block = resource.allocate(64, 16);
            auto* object = std::construct_at(static_cast<std::uint64_t*>(block), static_cast<std::uint64_t>(i));
            volatile std::uint64_t* observed = object; // Observation only, not inter-thread synchronization.
            *observed = i;
            result.checksum += *observed;
            const auto address = reinterpret_cast<std::uintptr_t>(block);
            if (previous == address) ++result.observed_address_reuse;
            previous = address;
            result.peak_bytes = std::max(result.peak_bytes, upstream.counters().outstanding);
            std::destroy_at(object);
            resource.deallocate(block, 64, 16);
        }
        result.loop_seconds = elapsed(loop);
    };
    if (variant == "heap") {
        run(upstream);
    } else if (variant == "arena") {
        std::pmr::vector<std::byte> backing{&upstream};
        backing.resize(items * 64 + 64);
        c07_l05::bounded_arena_resource arena{backing};
        run(arena);
        arena.release();
    } else if (variant == "pool") {
        std::pmr::vector<std::byte> backing{&upstream};
        backing.resize(2 * 64 + 64);
        c07_l05::fixed_pool_resource pool{backing, 64, 16};
        run(pool);
    } else if (variant == "std-monotonic") {
        std::pmr::monotonic_buffer_resource arena{&upstream};
        run(arena);
        arena.release();
    } else if (variant == "std-pool") {
        std::pmr::unsynchronized_pool_resource pool{&upstream};
        run(pool);
        pool.release();
    } else {
        throw std::invalid_argument("unknown allocation variant");
    }
    result.seconds = elapsed(started); // Includes setup, work and final resource/backing release.
    result.cleanup_seconds = std::max(0.0, result.seconds - result.setup_seconds - result.loop_seconds);
    const auto counters = upstream.counters();
    result.allocations = counters.allocations;
    result.deallocations = counters.deallocations;
    result.upstream_bytes = counters.bytes;
    require(counters.outstanding == 0 && counters.allocations == counters.deallocations, "upstream balance");
    require(result.checksum == static_cast<std::uint64_t>(items) * (items - 1) / 2, "allocation payload checksum");
    return result;
}

double clock_resolution() {
#ifdef _WIN32
    LARGE_INTEGER frequency{};
    return ::QueryPerformanceFrequency(&frequency) ? 1.0 / static_cast<double>(frequency.QuadPart) : 0.0;
#else
    timespec value{};
    return ::clock_getres(CLOCK_MONOTONIC, &value) == 0 ? value.tv_sec + value.tv_nsec * 1e-9 : 0.0;
#endif
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--check") {
            for (auto variant : {"heap", "arena", "pool", "std-monotonic", "std-pool"}) (void)allocation_case(variant, 32);
            (void)io_case("buffered", 4096, 1, 2);
            (void)io_case("mapped", 4096, 1, 2);
            std::cout << "B01 core controls: actual bytes, resource payloads and upstream balance passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--check-completion") {
            (void)io_case("completion", 131073, 1, 2);
            std::cout << "B01 completion control: actual native read, assembly and checked output passed\n";
            return 0;
        }
        std::cout << std::setprecision(17);
        if (argc == 4 && std::string_view(argv[1]) == "alloc") {
            const auto items = number(argv[3]);
            const auto r = allocation_case(argv[2], items);
            std::cout << "{\"kind\":\"alloc\",\"variant\":\"" << argv[2] << "\",\"items\":" << items
                << ",\"seconds\":" << r.seconds << ",\"setup_seconds\":" << r.setup_seconds
                << ",\"loop_seconds\":" << r.loop_seconds << ",\"cleanup_seconds\":" << r.cleanup_seconds
                << ",\"upstream_allocations\":" << r.allocations << ",\"upstream_deallocations\":" << r.deallocations
                << ",\"upstream_bytes\":" << r.upstream_bytes << ",\"peak_upstream_bytes\":" << r.peak_bytes
                << ",\"observed_address_reuse\":" << r.observed_address_reuse << ",\"checksum\":" << r.checksum;
        } else if (argc == 6 && std::string_view(argv[1]) == "io") {
            const auto size = number(argv[3]), rounds = number(argv[4]), depth = number(argv[5]);
            const auto r = io_case(argv[2], size, rounds, depth);
            std::cout << "{\"kind\":\"io\",\"variant\":\"" << argv[2] << "\",\"size\":" << size
                << ",\"rounds\":" << rounds << ",\"depth\":" << depth << ",\"seconds\":" << r.seconds
                << ",\"setup_seconds\":" << r.setup_seconds << ",\"read_seconds\":" << r.read_seconds
                << ",\"assemble_seconds\":" << r.assemble_seconds << ",\"write_seconds\":" << r.write_seconds
                << ",\"bytes\":" << r.bytes << ",\"read_calls\":" << r.read_calls
                << ",\"completions\":" << r.completions << ",\"peak_in_flight\":" << r.peak_in_flight
                << ",\"application_copy_bytes\":" << r.application_copy_bytes;
        } else {
            std::cerr << "usage: B01_costs_benchmark --check | --check-completion | alloc VARIANT ITEMS | io VARIANT BYTES ROUNDS DEPTH\n";
            return 2;
        }
        std::cout << ",\"clock_resolution_seconds\":" << clock_resolution()
            << ",\"clock_period_num\":" << bench_clock::period::num
            << ",\"clock_period_den\":" << bench_clock::period::den << ",\"valid\":true}\n";
        return 0;
    } catch (const std::system_error& error) {
        if (error.code() == std::make_error_code(std::errc::function_not_supported)) {
            std::cout << "SKIP: " << error.what() << '\n'; return 77;
        }
        std::cerr << error.what() << '\n'; return 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
