#pragma once
#include <check.hpp>
#include <field_projection.hpp>

#include <concepts>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace c04_projection::checks {
template<class T>
concept can_project_temporary = requires { c04_projection::project<"id">(T{}); };

template<class T>
concept can_project_expr = requires(T&& value) {
    c04_projection::project<"id">(std::forward<T>(value));
};

inline void run() {
    Person person{7, true, "Ada"};
    auto& id = c04_projection::get<"id">(person);
    static_assert(std::same_as<decltype(id), int&>);
    id = 8;
    check(person.id == 8, "get returns a writable member reference");

    const Person const_person{9, false, "Lin"};
    decltype(auto) const_name = c04_projection::get<"name">(const_person);
    static_assert(std::same_as<decltype(const_name), const std::string&>);
    check(const_name == "Lin", "get preserves const lvalue reference");

    std::string moved = c04_projection::get<"name">(Person{1, false, "Tmp"});
    check(moved == "Tmp", "get can consume an rvalue member expression");
    check((std::same_as<decltype(c04_projection::get<"name">(std::declval<Person&&>())), std::string&&> &&
           std::same_as<decltype(c04_projection::get<"name">(std::declval<const Person&&>())), const std::string&&>),
        "get preserves mutable and const rvalue reference categories");

    auto selected = c04_projection::project<"name,id">(person);
    std::get<0>(selected) = "Grace";
    std::get<1>(selected) = 42;
    check(person.name == "Grace" && person.id == 42, "project returns writable references in requested order");

    auto const_selected = c04_projection::project<"id,name">(const_person);
    check((std::same_as<decltype(const_selected), std::tuple<const int&, const std::string&>>),
        "const projection retains references rather than copies");
    check(std::get<0>(const_selected) == 9 && std::get<1>(const_selected) == "Lin",
        "project preserves const field references");

    auto empty = c04_projection::project<"">(person);
    static_assert(std::same_as<decltype(empty), std::tuple<>>);
    check(std::tuple_size_v<decltype(empty)> == 0, "empty projection is an empty tuple");

    Order order{"MSFT", 3, false};
    auto order_projection = c04_projection::project<"filled,symbol,quantity">(order);
    std::get<0>(order_projection) = true;
    std::get<1>(order_projection) = "NVDA";
    std::get<2>(order_projection) = 5;
    check(order.filled && order.symbol == "NVDA" && order.quantity == 5,
        "schema/key registration works for another supported record");

    check(!can_project_temporary<Person> && !can_project_expr<Person> && !can_project_expr<const Person>,
        "project rejects temporary records");
}
}
