#pragma once
#include "../reference/codec.hpp"
#include <meta>
#include <functional>

namespace c04_record {
// The schema registry selects the same teaching domain; actual field discovery,
// names and access below come from reflection, never the manual descriptors.
struct reflected_visitor {
    template<class T>
    static consteval auto members() {
        return std::define_static_array(std::meta::nonstatic_data_members_of(^^T,
            std::meta::access_context::unprivileged()));
    }
    static consteval std::string_view name_of(std::meta::info member) {
        const auto names = std::meta::annotations_of_with_type(member, ^^external_name);
        if (names.size() > 1) throw "multiple external names";
        if (!names.empty()) return std::meta::extract<external_name>(names.front()).text;
        if (!std::meta::has_identifier(member)) throw "unnamed field";
        return std::meta::identifier_of(member);
    }
    template<class T>
    static consteval bool supported() {
        if constexpr (!requires { schema<T>::fields; }) return false;
        else {
            if (!std::is_aggregate_v<T> || !std::meta::bases_of(^^T, std::meta::access_context::unprivileged()).empty()) return false;
            constexpr auto fields = members<T>();
            std::array<std::string_view, fields.size()> names{};
            std::size_t count = 0;
            template for (constexpr auto member : fields) {
                using value_type = [:std::meta::type_of(member):];
                if constexpr (!detail::atom<value_type> || !std::is_assignable_v<value_type&, value_type>) return false;
                const auto name = name_of(member);
                if (name.empty()) return false;
                for (std::size_t previous = 0; previous < count; ++previous)
                    if (names[previous] == name) return false;
                names[count++] = name;
            }
            return true;
        }
    }
    template<class T, class F>
    static consteval bool callable() {
        if constexpr (!requires { schema<std::remove_cvref_t<T>>::fields; }) return false;
        else {
            template for (constexpr auto member : members<std::remove_cvref_t<T>>()) {
                if constexpr (!std::invocable<F&, std::string_view, decltype(std::declval<T&&>().[:member:])>) return false;
            }
            return true;
        }
    }
    template<class T, class F>
    static consteval bool nothrow() {
        bool result = true;
        template for (constexpr auto member : members<std::remove_cvref_t<T>>()) {
            result = result && std::is_nothrow_invocable_v<F&, std::string_view, decltype(std::declval<T&&>().[:member:])>;
        }
        return result;
    }
    template<class T, class F> requires (callable<T, F>())
    static constexpr void visit_fields(T&& object, F&& function) noexcept(nothrow<T, F>()) {
        template for (constexpr auto member : members<std::remove_cvref_t<T>>()) {
            std::invoke(function, name_of(member), std::forward<T>(object).[:member:]);
        }
    }
};
using implementation = detail::codec<reflected_visitor>;
}
