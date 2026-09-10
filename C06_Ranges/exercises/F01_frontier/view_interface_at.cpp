#include <frontier_status.hpp>
#include <ranges>
#include <stdexcept>

template<class View>
concept has_at = requires(View& view) { view.at(0); };

int main() {
#if defined(__cpp_lib_view_interface) && __cpp_lib_view_interface >= 202606L
    int owner[]{1, 2, 3};
    auto view = std::ranges::subrange(owner);
    check(view.at(1) == 2, "view_interface at accesses a valid relative index");
    bool negative = false, upper = false;
    try { (void)view.at(-1); } catch (const std::out_of_range&) { negative = true; }
    try { (void)view.at(3); } catch (const std::out_of_range&) { upper = true; }
    check(negative && upper, "at rejects both negative and upper bound indices");
    const auto& const_view = view;
    const_view.at(0) = 7;
    check(owner[0] == 7, "const view does not imply const elements");
    auto filtered = view | std::views::filter([](int x) { return x > 0; });
    static_assert(!has_at<decltype(filtered)>);
    return verified("C++29 view_interface::at");
#else
    return unavailable("C++29 view_interface::at", "requires __cpp_lib_view_interface >= 202606L");
#endif
}
