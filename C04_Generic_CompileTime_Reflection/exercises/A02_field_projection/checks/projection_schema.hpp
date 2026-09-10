#pragma once
#include <string>
#include <tuple>

namespace c04_projection {
template<std::size_t N>
struct fixed_string {
    char value[N]{};
    constexpr fixed_string(const char (&text)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = text[i];
    }
    [[nodiscard]] constexpr std::size_t size() const { return N - 1; }
};

template<fixed_string Name, auto Member>
struct field {
    static constexpr auto name = Name;
    static constexpr auto member = Member;
};

struct Person {
    int id{};
    bool active{};
    std::string name;
};

struct Order {
    std::string symbol;
    int quantity{};
    bool filled{};
};

template<class T>
struct schema;

template<>
struct schema<Person> {
    static constexpr auto fields = std::tuple{
        field<"id", &Person::id>{},
        field<"active", &Person::active>{},
        field<"name", &Person::name>{}
    };
};

template<>
struct schema<Order> {
    static constexpr auto fields = std::tuple{
        field<"symbol", &Order::symbol>{},
        field<"quantity", &Order::quantity>{},
        field<"filled", &Order::filled>{}
    };
};
}
