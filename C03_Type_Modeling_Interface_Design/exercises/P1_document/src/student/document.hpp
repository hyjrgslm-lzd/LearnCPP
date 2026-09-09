#pragma once
#include <document_types.hpp>
#include <span>
#include <utility>
#include <vector>

namespace c03 {
// Implement this class only. document_types.hpp and the checker are provided.
class Document {
public:
    Document() = default;
    Document(const Document&) = default;
    Document& operator=(const Document&) = default;
    Document(Document&&) noexcept = default;
    Document& operator=(Document&&) noexcept = default;
    std::size_t size() const noexcept { return elements_.size(); }
    std::vector<Element> snapshot() const { return elements_; }
    std::optional<Element> find(ElementId) const { return std::nullopt; }
    std::expected<void, EditError> apply(const Edit& edit) {
        return apply_batch(std::span<const Edit>(&edit, 1));
    }
    std::expected<void, EditError> apply_batch(std::span<const Edit>) {
        // TODO: validate on a private candidate; commit once, after full success.
        return {};
    }
    void swap(Document& other) noexcept { elements_.swap(other.elements_); }
    bool operator==(const Document&) const = default;
private:
    std::vector<Element> elements_;
};
} // namespace c03
