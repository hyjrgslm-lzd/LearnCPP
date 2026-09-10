#include <frontier_status.hpp>
#include <map>
#include <unordered_map>
#include <flat_map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#if defined(__cpp_lib_map_lookup) && __cpp_lib_map_lookup >= 202606L
template<class Map>
void verify_lookup() {
    Map values{{1, "first"}};
    auto hit = values.lookup(1);
    static_assert(std::is_same_v<decltype(hit), std::optional<std::string&>>);
    check(hit.has_value(), "lookup finds the mapped value");
    *hit = "changed";
    check(values.at(1) == "changed", "lookup reference aliases the mapped object");
    check(!values.lookup(9) && values.size() == 1, "missing lookup does not insert a default value");
    const auto& constant = values;
    static_assert(std::is_same_v<decltype(constant.lookup(1)), std::optional<const std::string&>>);
    check(constant.lookup(9).value_or(std::string{"fallback"}) == "fallback", "missing const lookup supplies an independent default");
}
#endif

int main() {
#if defined(__cpp_lib_map_lookup) && __cpp_lib_map_lookup >= 202606L
    verify_lookup<std::map<int, std::string>>();
    verify_lookup<std::unordered_map<int, std::string>>();
    verify_lookup<std::flat_map<int, std::string>>();
    return verified("C++29 map/unordered_map/flat_map lookup");
#else
    return unavailable("C++29 associative lookup", "requires __cpp_lib_map_lookup >= 202606L; old proposal name get is not used");
#endif
}
