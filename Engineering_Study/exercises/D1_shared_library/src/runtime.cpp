#include "d1_runtime.h"

#include <cstdio>
#include <mutex>
#include <string>

namespace {
std::mutex exit_path_mutex;
std::string exit_path;
int init_count = 0;

struct runtime_marker {
    runtime_marker() { ++init_count; }

    ~runtime_marker()
    {
        std::lock_guard lock(exit_path_mutex);
        if (exit_path.empty()) return;
        std::FILE* file = nullptr;
#if defined(_MSC_VER)
        (void)fopen_s(&file, exit_path.c_str(), "wb");
#else
        file = std::fopen(exit_path.c_str(), "wb");
#endif
        if (file != nullptr) {
            std::fputs("runtime shutdown observed\n", file);
            std::fclose(file);
        }
    }
};

runtime_marker marker;
}

extern "C" int lesson_runtime_value(void)
{
    return 7;
}

extern "C" int lesson_runtime_init_count(void)
{
    return init_count;
}

extern "C" void lesson_runtime_record_exit_to(const char* path)
{
    std::lock_guard lock(exit_path_mutex);
    exit_path = path == nullptr ? std::string{} : path;
}
