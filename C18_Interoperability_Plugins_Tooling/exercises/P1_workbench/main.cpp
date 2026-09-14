#include "c18/bytes.hpp"
#include "c18/serial_plugin.hpp"
#ifdef C18_WITH_PYTHON
#include "python_backend.hpp"
#endif
#ifdef C18_WITH_LUA
#include "lua_backend.hpp"
#endif
#include <charconv>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

int main(int argc, char** argv) {
    try {
        std::string backend, hexadecimal, library;
        bool have_input = false;
        for (int i = 1; i < argc; i += 2) {
            if (i + 1 == argc) throw std::runtime_error("an option value is required");
            const std::string_view option(argv[i]);
            if (option == "--backend") backend = argv[i + 1];
            else if (option == "--hex") { hexadecimal = argv[i + 1]; have_input = true; }
            else if (option == "--plugin") library = argv[i + 1];
            else throw std::runtime_error("unknown option");
        }
        if (!have_input || hexadecimal.size() % 2 || hexadecimal.size() > 2 * 1024 * 1024)
            throw std::runtime_error("--hex requires even-length hexadecimal, at most 1 MiB of decoded bytes");
        std::vector<uint8_t> input(hexadecimal.size() / 2), output(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            unsigned value = 0;
            const char* first = hexadecimal.data() + i * 2;
            const auto parsed = std::from_chars(first, first + 2, value, 16);
            if (parsed.ec != std::errc{} || parsed.ptr != first + 2 || value > 255)
                throw std::runtime_error("invalid hexadecimal byte");
            input[i] = static_cast<uint8_t>(value);
        }
        size_t written = 0;
        if (backend == "native") {
            if (c18::transform_bytes(input.data(), input.size(), output.data(), output.size(), &written) != C18_STATUS_OK)
                throw std::runtime_error("native transformation failed");
        } else if (backend == "plugin") {
            c18::SerialPlugin plugin;
            if (plugin.open(std::filesystem::path(library)) != C18_STATUS_OK)
                throw std::runtime_error("plugin load or ABI negotiation failed");
            const auto status = plugin.process(input, output, written);
            const auto closed = plugin.close();
            if (status != C18_STATUS_OK || closed != C18_STATUS_OK)
                throw std::runtime_error("plugin processing or cleanup failed");
        } else if (backend == "python") {
#ifdef C18_WITH_PYTHON
            output = c18::python_transform(input);
            written = output.size();
#else
            throw std::runtime_error("python backend was not enabled at build time");
#endif
        } else if (backend == "lua") {
#ifdef C18_WITH_LUA
            output = c18::lua_transform(input);
            written = output.size();
#else
            throw std::runtime_error("lua backend was not enabled at build time");
#endif
        } else throw std::runtime_error("--backend must be native, plugin, python, or lua");
        if (written != input.size() || output.size() != input.size())
            throw std::runtime_error("backend broke the byte-count contract");
        std::cout << std::hex << std::setfill('0');
        for (uint8_t byte : output) std::cout << std::setw(2) << static_cast<unsigned>(byte);
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "workbench: " << error.what() << '\n';
        return 2;
    }
}
