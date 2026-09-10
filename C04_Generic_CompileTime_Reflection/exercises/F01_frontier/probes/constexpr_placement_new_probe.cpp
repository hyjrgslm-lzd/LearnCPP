#include <iostream>
#include <memory>
#include <new>

#if defined(C04_HAS_CONSTEXPR_PLACEMENT_NEW) && C04_HAS_CONSTEXPR_PLACEMENT_NEW
#define C04_TRY_CONSTEXPR_PLACEMENT_NEW 1
#elif defined(C04_FORCE_CONSTEXPR_PLACEMENT_NEW_PROBE)
#define C04_TRY_CONSTEXPR_PLACEMENT_NEW 1
#elif defined(__cpp_lib_constexpr_new) && __cpp_lib_constexpr_new >= 202406L
#define C04_TRY_CONSTEXPR_PLACEMENT_NEW 1
#endif
#ifndef C04_TRY_CONSTEXPR_PLACEMENT_NEW
#define C04_TRY_CONSTEXPR_PLACEMENT_NEW 0
#endif

#if C04_TRY_CONSTEXPR_PLACEMENT_NEW
struct Point {
    int x;
    int y;
};

constexpr int construct_scalar() {
    std::allocator<int> allocator;
    int* p = allocator.allocate(1);
    ::new (p) int(42);
    int value = *p;
    std::destroy_at(p);
    allocator.deallocate(p, 1);
    return value;
}

constexpr int construct_aggregate() {
    std::allocator<Point> allocator;
    Point* p = allocator.allocate(1);
    ::new (p) Point{.x = 4, .y = 5};
    int value = p->x + p->y;
    std::destroy_at(p);
    allocator.deallocate(p, 1);
    return value;
}

#if defined(C04_P2747_NEGATIVE_WRONG_STORAGE)
constexpr int wrong_storage() {
    std::allocator<unsigned char> allocator;
    unsigned char* p = allocator.allocate(sizeof(int));
    ::new (p) int(7);
    int value = *reinterpret_cast<int*>(p);
    allocator.deallocate(p, sizeof(int));
    return value;
}
static_assert(wrong_storage() == 7);
#endif
#endif

int main() {
    std::cout << "probe=constexpr_placement_new header=na macro="
#if defined(__cpp_lib_constexpr_new)
              << __cpp_lib_constexpr_new
#else
              << 0
#endif
              << " body=" << C04_TRY_CONSTEXPR_PLACEMENT_NEW << "\n";
#if !C04_TRY_CONSTEXPR_PLACEMENT_NEW
    std::cout << "SKIP constexpr placement new: compile probe did not prove P2747R2 behavior\n";
    return 77;
#else
    static_assert(construct_scalar() == 42);
    static_assert(construct_aggregate() == 9);
    std::cout << "PASS constexpr placement new scalar and aggregate\n";
    return 0;
#endif
}
