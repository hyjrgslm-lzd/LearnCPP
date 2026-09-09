#pragma once
#include <document_types.hpp>
#include <algorithm>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace c03 {
class Document {
public:
    Document() = default;
    Document(const Document&) = default;
    Document& operator=(const Document& other) {
        if (this != &other) {
            Document next(other);
            swap(next);
        }
        return *this;
    }
    Document(Document&& other) noexcept : elements_(std::move(other.elements_)) {
        other.elements_.clear();
    }
    Document& operator=(Document&& other) noexcept {
        if (this != &other) {
            elements_ = std::move(other.elements_);
            other.elements_.clear();
        }
        return *this;
    }
    std::size_t size() const noexcept { return elements_.size(); }
    std::vector<Element> snapshot() const { return elements_; }
    std::optional<Element> find(ElementId id) const {
        const auto at = std::ranges::find(elements_, id, &Element::id);
        if (at == elements_.end()) return std::nullopt;
        return *at;
    }
    std::expected<void, EditError> apply(const Edit& edit) {
        return apply_batch(std::span<const Edit>(&edit, 1));
    }
    std::expected<void, EditError> apply_batch(std::span<const Edit> edits) {
        if (edits.empty()) return {};
        Document next(*this);
        for (const Edit& edit : edits) {
            if (auto result = next.apply_in_place(edit); !result) return result;
        }
        swap(next);
        return {};
    }
    void swap(Document& other) noexcept { elements_.swap(other.elements_); }
    bool operator==(const Document&) const = default;

private:
    std::expected<void, EditError> apply_in_place(const Edit& edit) {
        return std::visit([this](const auto& operation) -> std::expected<void, EditError> {
            using Operation = std::remove_cvref_t<decltype(operation)>;
            const ElementId id = [&] {
                if constexpr (std::is_same_v<Operation, Erase>) return operation.id;
                else return operation.element.id;
            }();
            const auto at = std::ranges::find(elements_, id, &Element::id);
            if constexpr (std::is_same_v<Operation, Add>) {
                if (at != elements_.end())
                    return std::unexpected(EditError{EditErrorCode::duplicate_id, id});
                elements_.push_back(operation.element);
            } else {
                if (at == elements_.end())
                    return std::unexpected(EditError{EditErrorCode::missing_id, id});
                if constexpr (std::is_same_v<Operation, Replace>) *at = operation.element;
                else elements_.erase(at);
            }
            return {};
        }, edit);
    }
    std::vector<Element> elements_;
};
} // namespace c03
