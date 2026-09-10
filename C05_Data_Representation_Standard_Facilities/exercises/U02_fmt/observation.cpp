#include <check.hpp>

#include <fmt/args.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

struct Metric {
    std::string name;
    int milli{};
};

template<>
struct fmt::formatter<Metric> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const Metric& metric, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}={}.{:03}", metric.name, metric.milli / 1000,
            metric.milli % 1000);
    }
};

template<typename... Args>
static std::string checked(fmt::format_string<Args...> pattern, Args&&... args)
{
    return fmt::format(pattern, std::forward<Args>(args)...);
}

static bool runtime_rejects(std::string_view pattern)
{
    try {
        (void)fmt::format(fmt::runtime(pattern), "text");
    } catch (const fmt::format_error&) {
        return true;
    }
    return false;
}

int main()
{
    const auto fixed_text = checked("{}={}", std::string_view{"id"}, 7);
    const bool runtime_error = runtime_rejects("{:d}");
    check(fixed_text == "id=7", "fmt::format_string checks fixed call shape");
    check(runtime_error, "fmt::runtime moves bad pattern to runtime format_error");

    const Metric cpu{"cpu", 85375};
    const auto metric_text = fmt::format("{}", cpu);
    check(metric_text == "cpu=85.375", "custom formatter formats unfamiliar type");

    const auto compiled_text = fmt::format(FMT_COMPILE("{}:{}"), "port", 443);
    check(compiled_text == "port:443", "FMT_COMPILE formats through compiled representation");

#if FMT_USE_NONTYPE_TEMPLATE_ARGS
    using namespace fmt::literals;
    const auto cf_text = fmt::format("{}={}"_cf, "answer", 42);
    check(cf_text == "answer=42", "_cf literal is available");
#else
    const std::string cf_text = "<unavailable>";
    check(true, "_cf literal unavailable on this fmt/compiler combination");
#endif

    int count = 3;
    std::string label = "items";
    auto store = fmt::make_format_args(label, count);
    count = 4;
    const auto erased_text = fmt::vformat("{}={}", store);
    check(erased_text == "items=3",
        "format_arg_store may keep small builtin values while borrowing object-backed args");

    std::string copied = "owned";
    fmt::dynamic_format_arg_store<fmt::format_context> owned;
    owned.push_back(copied);
    owned.push_back(9);
    copied[0] = 'X';
    const auto owned_text = fmt::vformat("{}={}", owned);
    check(owned_text == "owned=9", "dynamic_format_arg_store can own copied dynamic arguments");

    std::string borrowed = "borrowed";
    fmt::dynamic_format_arg_store<fmt::format_context> borrowed_args;
    borrowed_args.push_back(std::cref(borrowed));
    borrowed[0] = 'B';
    const auto borrowed_text = fmt::vformat("{}", borrowed_args);
    check(borrowed_text == "Borrowed", "dynamic_format_arg_store borrows explicit cref arguments");

    static_assert(!std::is_same_v<decltype(store), fmt::format_args>,
        "make_format_args returns a typed store before erasure");
    fmt::format_args erased = store;
    check(fmt::vformat("{}={}", erased) == "items=3", "format_args is the erased view");

    std::cout << "fixed_text=" << fixed_text << '\n';
    std::cout << "runtime_error=" << runtime_error << '\n';
    std::cout << "metric_text=" << metric_text << '\n';
    std::cout << "compiled_text=" << compiled_text << '\n';
    std::cout << "cf_text=" << cf_text << '\n';
    std::cout << "erased_text=" << erased_text << '\n';
    std::cout << "owned_text=" << owned_text << '\n';
    std::cout << "borrowed_text=" << borrowed_text << '\n';

#ifdef C05_FMT_SOURCE_COMMIT
    check(std::string_view{C05_FMT_SOURCE_COMMIT}
            == "407c905e45ad75fc29bf0f9bb7c5c2fd3475976f",
        "fmt source commit is pinned");
#endif
}
