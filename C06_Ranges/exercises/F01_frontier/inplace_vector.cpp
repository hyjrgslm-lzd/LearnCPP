#include <frontier_status.hpp>
#if __has_include(<inplace_vector>)
#include <inplace_vector>
#include <new>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#endif

int main() {
#if __has_include(<inplace_vector>) && defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202603L
    std::inplace_vector<std::string, 2> values;
    check(values.empty() && values.capacity() == 2, "fixed capacity does not mean constructed elements");
    values.push_back("first");
    auto* first = &values.front();
    auto accepted = values.try_push_back("second");
    static_assert(std::is_same_v<decltype(accepted), std::optional<std::string&>>);
    check(accepted.has_value() && &*accepted == &values.back(), "try insertion returns optional reference");
    check(&values.front() == first, "append without relocation preserves first element address");
    std::string input = "not consumed";
    auto rejected = values.try_push_back(std::move(input));
    check(!rejected && input == "not consumed", "full try insertion does not consume the input");
    bool exhausted = false;
    try { values.push_back("overflow"); } catch (const std::bad_alloc&) { exhausted = true; }
    check(exhausted && values.size() == 2, "ordinary insertion reports capacity exhaustion");
    values.pop_back();
    check(values.size() == 1 && values.front() == "first", "pop destroys only the last active element");
    std::inplace_vector<int, 0> zero;
    check(!zero.try_push_back(1), "zero capacity rejects insertion");
    return verified("inplace_vector 202603 optional-reference API");
#else
    return unavailable("inplace_vector", "requires <inplace_vector> and macro >= 202603L; old pointer API is a different revision");
#endif
}
