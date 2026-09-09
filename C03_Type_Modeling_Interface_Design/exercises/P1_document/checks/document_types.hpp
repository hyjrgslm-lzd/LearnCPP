#pragma once
#include <compare>
#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <variant>

namespace c03 {
enum class ValueError { zero_id, nonpositive_length };

class ElementId {
public:
    static std::expected<ElementId, ValueError> make(std::uint64_t value) noexcept {
        if (value == 0) return std::unexpected(ValueError::zero_id);
        return ElementId(value);
    }
    std::uint64_t value() const noexcept { return value_; }
    auto operator<=>(const ElementId&) const = default;
private:
    explicit ElementId(std::uint64_t value) noexcept : value_(value) {}
    std::uint64_t value_;
};

class PositiveLength {
public:
    static std::expected<PositiveLength, ValueError> make(int value) noexcept {
        if (value <= 0) return std::unexpected(ValueError::nonpositive_length);
        return PositiveLength(value);
    }
    int value() const noexcept { return value_; }
    auto operator<=>(const PositiveLength&) const = default;
private:
    explicit PositiveLength(int value) noexcept : value_(value) {}
    int value_;
};

struct Rectangle {
    PositiveLength width, height;
    auto operator<=>(const Rectangle&) const = default;
};
struct Circle {
    PositiveLength radius;
    auto operator<=>(const Circle&) const = default;
};
using Shape = std::variant<Rectangle, Circle>;
struct Element {
    ElementId id;
    Shape shape;
    std::optional<std::string> label;
    auto operator<=>(const Element&) const = default;
};
struct Add { Element element; };
struct Replace { Element element; };
struct Erase { ElementId id; };
using Edit = std::variant<Add, Replace, Erase>;
enum class EditErrorCode { duplicate_id, missing_id };
struct EditError {
    EditErrorCode code;
    ElementId id;
    bool operator==(const EditError&) const = default;
};
} // namespace c03

template<> struct std::hash<c03::ElementId> {
    std::size_t operator()(c03::ElementId id) const noexcept {
        return std::hash<std::uint64_t>{}(id.value());
    }
};
