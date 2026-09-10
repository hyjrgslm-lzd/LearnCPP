#pragma once
#include <record_schema.hpp>
#include <expected>
#include <span>

namespace c04_record {
// Implement these operations; the supplied schema contains metadata only.
struct implementation {
    template<class T, class F>
    static void visit_fields(T&&, F&&) {}
    template<class T>
    static encoded_fields encode_fields(const T&) { return {}; }
    template<class T>
    static std::expected<T, field_error> decode_fields(std::span<const FieldValue>) {
        return std::unexpected(field_error::missing_field);
    }
    template<class T>
    static std::string format_record(const T&) { return {}; }
};
}
