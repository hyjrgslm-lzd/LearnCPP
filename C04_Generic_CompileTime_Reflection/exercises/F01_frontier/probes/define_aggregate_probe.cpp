#include <iostream>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define C04_HAS_META_HEADER 1
#endif
#endif

#if defined(C04_FORCE_DEFINE_AGGREGATE_PROBE) || (defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202603L)
#define C04_HAS_DEFINE_AGGREGATE 1
#endif
#ifndef C04_HAS_META_HEADER
#define C04_HAS_META_HEADER 0
#endif
#ifndef C04_HAS_DEFINE_AGGREGATE
#define C04_HAS_DEFINE_AGGREGATE 0
#endif

template<class T>
struct GeneratedRecord;

int main() {
    std::cout << "probe=define_aggregate header=" << C04_HAS_META_HEADER
              << " macro=" << C04_HAS_DEFINE_AGGREGATE
              << " body=" << (C04_HAS_META_HEADER && C04_HAS_DEFINE_AGGREGATE) << "\n";
#if !C04_HAS_META_HEADER || !C04_HAS_DEFINE_AGGREGATE
    std::cout << "SKIP define_aggregate: missing <meta> or explicit aggregate generation macro\n";
    return 77;
#else
    consteval {
        std::meta::define_aggregate(^^GeneratedRecord<int>, {
            std::meta::data_member_spec(^^int, {.name = "id"}),
            std::meta::data_member_spec(^^bool, {.name = "enabled"})
        });
    }
    GeneratedRecord<int> value{.id = 3, .enabled = true};
    if (value.id != 3 || !value.enabled) {
        std::cout << "FAIL define_aggregate generated layout\n";
        return 1;
    }
    std::cout << "PASS data_member_spec and define_aggregate\n";
    return 0;
#endif
}
