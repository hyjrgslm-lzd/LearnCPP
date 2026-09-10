#include <pipeline.hpp>
#include <check.hpp>
#include <c07/file_pipeline_types.hpp>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <c07/file_pipeline_io.hpp>

namespace {

std::vector<std::byte> bytes(std::string_view text) {
    std::vector<std::byte> out(text.size());
    std::ranges::transform(text, out.begin(), [](char ch) {
        return static_cast<std::byte>(static_cast<unsigned char>(ch));
    });
    return out;
}

std::string text(std::span<const std::byte> data) {
    std::string out(data.size(), '\0');
    std::ranges::transform(data, out.begin(), [](std::byte value) {
        return static_cast<char>(value);
    });
    return out;
}

c07::completed_chunk chunk(std::uint64_t id, std::size_t offset, std::string_view payload) {
    return c07::completed_chunk{id, offset, bytes(payload)};
}

std::uint64_t runtime_seed() {
    const auto ticks = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return ticks ^ (std::random_device{}() + 0x9e3779b97f4a7c15ULL);
}

std::string payload(std::size_t size, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::string out;
    out.reserve(size);
    for (std::size_t i = 0; i != size; ++i) {
        out.push_back(static_cast<char>('!' + (rng() % 94)));
    }
    return out;
}

std::filesystem::path temp_path(std::wstring_view name) {
    const auto stamp = std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / (L"c07_p1_" + stamp + L"_" + std::wstring{name});
}

void write_text(const std::filesystem::path& path, std::string_view data) {
    std::ofstream out(path, std::ios::binary);
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

void expect_assembled(std::vector<c07::completed_chunk> chunks, std::size_t total, std::string_view expected,
    std::string_view message) {
    const auto assembled = c07_p1::assemble(chunks, total);
    check(assembled.has_value(), message);
    check(text(*assembled) == expected, message);
}

void expect_rejected(std::vector<c07::completed_chunk> chunks, std::size_t total, std::string_view message) {
    check(!c07_p1::assemble(chunks, total), message);
}

void synthetic_checks() {
    const auto seed = runtime_seed();
    const auto data = payload(73, seed);
    std::cout << "P1 synthetic seed=" << seed << '\n';
    expect_assembled({chunk(3, 29, std::string_view{data}.substr(29)),
                         chunk(1, 0, std::string_view{data}.substr(0, 17)),
                         chunk(2, 17, std::string_view{data}.substr(17, 12))},
        data.size(), data, "out-of-order chunks assemble by offset");
    expect_assembled({}, 0, "", "empty file has zero chunks");

    expect_rejected({chunk(0, 0, std::string_view{data}.substr(0, 1))}, 1, "zero request id is rejected");
    expect_rejected({chunk(7, 0, std::string_view{data}.substr(0, 1)),
                        chunk(7, 1, std::string_view{data}.substr(1, 1))},
        2, "duplicate request id is rejected");
    expect_rejected({chunk(1, 0, std::string_view{data}.substr(0, 3)),
                        chunk(2, 2, std::string_view{data}.substr(2, 3))},
        5, "overlap is rejected");
    expect_rejected({chunk(1, 0, std::string_view{data}.substr(0, 2)),
                        chunk(2, 3, std::string_view{data}.substr(3, 2))},
        5, "gap is rejected");
    expect_rejected({chunk(1, std::numeric_limits<std::size_t>::max(), std::string_view{data}.substr(0, 1))},
        1, "offset overflow is rejected");
    expect_rejected({c07::completed_chunk{1, 0, {}}}, 1, "empty non-eof chunk is rejected");
    expect_rejected({chunk(1, 0, std::string_view{data}.substr(0, 3))}, 2, "range past total is rejected");
    expect_rejected({}, c07::max_pipeline_bytes + 1, "oversize total is rejected");
    expect_rejected({chunk(1, 0, std::string_view{data}.substr(0, 1))}, 0, "empty total rejects chunks");
}

c07::file_backend parse_backend(std::string_view value) {
    if (value == "buffered") return c07::file_backend::buffered;
    if (value == "mapped") return c07::file_backend::mapped;
    if (value == "completion") return c07::file_backend::completion;
    std::cerr << "unknown backend: " << value << '\n';
    std::exit(2);
}

int backend_check(std::string_view name) {
    const auto seed = runtime_seed();
    const auto payload = ::payload(4109, seed);
    std::cout << "P1 backend seed=" << seed << '\n';

    const auto input = temp_path(L"input unicode \u7ec4\u88c5.txt");
    const auto output = temp_path(L"output unicode \u7ec4\u88c5.txt");
    write_text(input, payload);

    const auto backend = parse_backend(name);
    const auto batch = c07::read_file_chunks(input, backend, 257, 3);
    if (!batch && backend == c07::file_backend::completion
        && batch.error() == std::make_error_code(std::errc::function_not_supported)) {
        std::cout << "SKIP: completion backend not supported\n";
        return 77;
    }
    if (!batch) std::cerr << "read_file_chunks failed: " << batch.error().message() << '\n';
    check(batch.has_value(), "native backend read succeeds");

    const auto assembled = c07_p1::assemble(batch->chunks, batch->total_bytes);
    check(assembled.has_value(), "native chunks assemble");
    check(text(*assembled) == payload, "native chunks match source bytes");

    const auto written = c07::write_new_file(output, *assembled);
    check(written.has_value(), "native assembled bytes write to new file");
    check(read_text(output) == payload, "written file reads back");

    std::error_code ignored;
    std::filesystem::remove(input, ignored);
    std::filesystem::remove(output, ignored);
    std::cout << "P1 file pipeline " << name << " backend check passed\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string_view{argv[1]} == "--backend") {
        return backend_check(argv[2]);
    }
    synthetic_checks();
    std::cout << "P1 file pipeline assembly checks passed\n";
}
