#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>

struct fail_once_resource {
    bool fail = false;

    void* allocate(std::size_t n) {
        if (fail) {
            fail = false;
            throw std::bad_alloc();
        }
        if (void* p = std::malloc(n ? n : 1)) return p;
        throw std::bad_alloc();
    }

    void deallocate(void* p) noexcept {
        std::free(p);
    }
};

int main() {
    fail_once_resource r;
    r.fail = true;
    bool caught = false;
    try {
        void* p = r.allocate(4);
        r.deallocate(p);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    assert(caught);
    void* p = r.allocate(4);
    r.deallocate(p);
    std::cout << "ok local allocation injection\n";
}
