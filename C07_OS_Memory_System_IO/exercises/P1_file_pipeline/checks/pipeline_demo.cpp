#include <pipeline.hpp>
#include <c07/file_pipeline_types.hpp>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <c07/file_pipeline_io.hpp>

namespace {

struct options {
    std::filesystem::path input;
    std::filesystem::path output;
    c07::file_backend backend = c07::file_backend::buffered;
    std::size_t chunk = 65536;
    std::size_t in_flight = 4;
    bool self_test = false;
};

std::string narrow_arg(
#ifdef _WIN32
    const wchar_t* value
#else
    const char* value
#endif
) {
#ifdef _WIN32
    const std::wstring wide{value};
    std::string out;
    out.reserve(wide.size());
    for (const wchar_t ch : wide) out.push_back(static_cast<char>(ch));
    return out;
#else
    return value;
#endif
}

std::optional<c07::file_backend> parse_backend(std::string_view value) {
    if (value == "buffered") return c07::file_backend::buffered;
    if (value == "mapped") return c07::file_backend::mapped;
    if (value == "completion") return c07::file_backend::completion;
    return std::nullopt;
}

bool parse_size(std::string_view value, std::size_t min, std::size_t max, std::size_t& out) {
    std::size_t parsed = 0;
    const auto* first = value.data();
    const auto* last = value.data() + value.size();
    const auto result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last || parsed < min || parsed > max) return false;
    out = parsed;
    return true;
}

template <class Char>
int run(int argc, Char** argv) {
    options opt;
    for (int i = 1; i != argc; ++i) {
        const std::string arg = narrow_arg(argv[i]);
        if (arg == "--self-test") {
            opt.self_test = true;
        } else if ((arg == "--input" || arg == "--output" || arg == "--backend" || arg == "--chunk"
                       || arg == "--inflight")
            && i + 1 < argc) {
            const std::string value = narrow_arg(argv[++i]);
            if (arg == "--input") opt.input = argv[i];
            if (arg == "--output") opt.output = argv[i];
            if (arg == "--backend") {
                const auto parsed = parse_backend(value);
                if (!parsed) return 2;
                opt.backend = *parsed;
            }
            if (arg == "--chunk" && !parse_size(value, 1, 1024 * 1024, opt.chunk)) return 2;
            if (arg == "--inflight" && !parse_size(value, 1, 16, opt.in_flight)) return 2;
        } else {
            return 2;
        }
    }
    if (opt.self_test) {
        auto assembled = c07_p1::assemble({}, 0);
        return assembled && assembled->empty() ? 0 : 1;
    }
    if (opt.input.empty() || opt.output.empty() || std::filesystem::exists(opt.output)) return 2;

    const auto batch = c07::read_file_chunks(opt.input, opt.backend, opt.chunk, opt.in_flight);
    if (!batch) {
        std::cerr << "read failed: " << batch.error().message() << '\n';
        return 1;
    }
    const auto assembled = c07_p1::assemble(batch->chunks, batch->total_bytes);
    if (!assembled) {
        std::cerr << "assembly failed: " << assembled.error().message() << '\n';
        return 1;
    }
    const auto written = c07::write_new_file(opt.output, *assembled);
    if (!written) {
        std::cerr << "write failed: " << written.error().message() << '\n';
        return 1;
    }
    return 0;
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    return run(argc, argv);
}
#else
int main(int argc, char** argv) {
    return run(argc, argv);
}
#endif
