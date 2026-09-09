#include <cstdio>
#include <cstdlib>
#include <exception>
#include <new>
#include <string>
#include <utility>

static bool inject = false;
void* operator new(std::size_t bytes) {
    if (inject) {
        inject = false;
        std::fprintf(stderr, "injected allocation: bytes=%zu\n", bytes);
        throw std::bad_alloc();
    }
    if (auto* p = std::malloc(bytes ? bytes : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

int main() {
    std::set_terminate([] {
        std::fputs("terminate while moving std::string\n", stderr);
        std::_Exit(86); // bounded noninteractive evidence, no CRT abort dialog
    });
    std::string source(256, 'x');
    inject = true;
    std::string destination(std::move(source));
    inject = false;
    std::printf("string move succeeded: size=%zu\n", destination.size());
    return destination.size() == 256 ? 0 : 1;
}
