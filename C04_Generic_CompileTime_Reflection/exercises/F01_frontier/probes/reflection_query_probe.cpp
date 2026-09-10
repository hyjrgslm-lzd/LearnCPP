#include <iostream>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define C04_HAS_META_HEADER 1
#endif
#endif

#if defined(C04_FORCE_REFLECTION_PROBE) || (defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202603L)
#define C04_HAS_REFLECTION_SYNTAX 1
#endif
#ifndef C04_HAS_META_HEADER
#define C04_HAS_META_HEADER 0
#endif
#ifndef C04_HAS_REFLECTION_SYNTAX
#define C04_HAS_REFLECTION_SYNTAX 0
#endif

enum class ProbeColor { red, blue };

struct ProbeRecord {
    int id;
    bool enabled;
};

#if C04_HAS_META_HEADER && C04_HAS_REFLECTION_SYNTAX
consteval bool metadata_queries_work() {
    using namespace std::meta;
    auto fields = nonstatic_data_members_of(^^ProbeRecord, access_context::unprivileged());
    auto colors = enumerators_of(^^ProbeColor);
    return fields.size() == 2
        && has_identifier(fields[0])
        && identifier_of(fields[0]) == "id"
        && type_of(fields[0]) == ^^int
        && colors.size() == 2
        && identifier_of(colors[0]) == "red";
}
#endif

int main() {
    std::cout << "probe=reflection_query header=" << C04_HAS_META_HEADER
              << " macro=" << C04_HAS_REFLECTION_SYNTAX
              << " body=" << (C04_HAS_META_HEADER && C04_HAS_REFLECTION_SYNTAX) << "\n";
#if !C04_HAS_META_HEADER || !C04_HAS_REFLECTION_SYNTAX
    std::cout << "SKIP reflection query: missing <meta> or explicit reflection syntax macro\n";
    return 77;
#else
    static_assert(metadata_queries_work());
    if (!metadata_queries_work()) {
        std::cout << "FAIL reflection metadata query\n";
        return 1;
    }
    std::cout << "PASS reflection metadata queries\n";
    return 0;
#endif
}
