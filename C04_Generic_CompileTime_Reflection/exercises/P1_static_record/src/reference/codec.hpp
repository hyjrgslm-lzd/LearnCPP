#pragma once
#include <record_schema.hpp>
#include <array>
#include <charconv>
#include <concepts>
#include <expected>
#include <iomanip>
#include <limits>
#include <optional>
#include <span>
#include <sstream>
#include <type_traits>
#include <utility>

namespace c04_record::detail {
template<class T>
inline constexpr bool atom = std::same_as<T, int> || std::same_as<T, bool> || std::same_as<T, std::string>;

inline std::string encode_atom(const std::string& value) { return value; }
inline std::string encode_atom(bool value) { return value ? "true" : "false"; }
inline std::string encode_atom(int value) {
    std::array<char, std::numeric_limits<int>::digits10 + 3> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    // The buffer covers the full signed decimal range, including sign.
    return {buffer.data(), result.ptr};
}
inline bool decode_atom(std::string_view text, std::string& value) {
    value.assign(text);
    return true;
}
inline bool decode_atom(std::string_view text, bool& value) {
    if (text != "true" && text != "false") return false;
    value = text == "true";
    return true;
}
inline bool decode_atom(std::string_view text, int& value) {
    if (text.empty()) return false;
    int candidate{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), candidate, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) return false;
    value = candidate;
    return true;
}

// Both the manual and actual reflection paths use this same codec contract.
// Student never includes this solution header; its own operations are checked.
template<class Visitor>
struct codec {
    template<class T, class F>
        requires requires(T&& value, F&& function) { Visitor::visit_fields(std::forward<T>(value), std::forward<F>(function)); }
    static constexpr void visit_fields(T&& value, F&& function)
        noexcept(noexcept(Visitor::visit_fields(std::forward<T>(value), std::forward<F>(function)))) {
        Visitor::visit_fields(std::forward<T>(value), std::forward<F>(function));
    }

    template<class T> requires (Visitor::template supported<T>())
    static encoded_fields encode_fields(const T& value) {
        encoded_fields result;
        visit_fields(value, [&](std::string_view name, const auto& member) {
            result.push_back({std::string(name), encode_atom(member)});
        });
        return result;
    }

    template<class T> requires (Visitor::template supported<T>() && std::default_initializable<T>)
    static std::expected<T, field_error> decode_fields(std::span<const FieldValue> input) {
        T result{};
        std::optional<field_error> error;
        std::size_t recognized = 0;
        // ponytail: O(fields * input), indexed lookup only for measured large schemas.
        visit_fields(result, [&](std::string_view name, auto& member) {
            if (error) return;
            const FieldValue* found = nullptr;
            for (const auto& entry : input) {
                if (entry.name != name) continue;
                if (found) { error = field_error::duplicate_field; return; }
                found = &entry;
            }
            if (!found) { error = field_error::missing_field; return; }
            ++recognized;
            if (!decode_atom(found->value, member)) error = field_error::invalid_value;
        });
        if (error) return std::unexpected(*error);
        if (recognized != input.size()) return std::unexpected(field_error::unknown_field);
        return result;
    }

    template<class T> requires (Visitor::template supported<T>())
    static std::string format_record(const T& value) {
        std::ostringstream out;
        out << "record(";
        bool first = true;
        visit_fields(value, [&](std::string_view name, const auto& member) {
            if (!std::exchange(first, false)) out << ", ";
            out << name << '=';
            if constexpr (std::same_as<std::remove_cvref_t<decltype(member)>, std::string>) out << std::quoted(member);
            else out << std::boolalpha << member;
        });
        out << ')';
        return out.str();
    }
};
}
