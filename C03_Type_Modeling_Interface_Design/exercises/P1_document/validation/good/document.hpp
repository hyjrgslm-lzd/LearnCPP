#pragma once
#include <document_types.hpp>
#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace c03 {
// A separate completion: edit an independent vector using explicit alternatives.
class Document {
public:
    Document() = default;
    Document(const Document&) = default;
    Document& operator=(const Document& rhs) {
        auto candidate = rhs.entries_;
        entries_.swap(candidate);
        return *this;
    }
    Document(Document&& rhs) noexcept { entries_.swap(rhs.entries_); }
    Document& operator=(Document&& rhs) noexcept {
        if (this != &rhs) {
            entries_.clear();
            entries_.swap(rhs.entries_);
        }
        return *this;
    }
    std::size_t size() const noexcept { return entries_.size(); }
    std::vector<Element> snapshot() const { return entries_; }
    std::optional<Element> find(ElementId id) const {
        for (const auto& entry : entries_) if (entry.id == id) return entry;
        return {};
    }
    std::expected<void, EditError> apply(const Edit& edit) {
        return apply_batch({&edit, 1});
    }
    std::expected<void, EditError> apply_batch(std::span<const Edit> edits) {
        if (edits.empty()) return {};
        auto candidate = entries_;
        for (const auto& edit : edits) {
            if (const auto* add = std::get_if<Add>(&edit)) {
                const auto exists = std::ranges::find(candidate, add->element.id, &Element::id);
                if (exists != candidate.end())
                    return std::unexpected(EditError{EditErrorCode::duplicate_id, add->element.id});
                candidate.push_back(add->element);
            } else {
                const auto* replacement = std::get_if<Replace>(&edit);
                const auto id = replacement ? replacement->element.id : std::get<Erase>(edit).id;
                const auto index = std::find_if(candidate.begin(), candidate.end(),
                    [id](const auto& e) { return e.id == id; });
                if (index == candidate.end())
                    return std::unexpected(EditError{EditErrorCode::missing_id, id});
                if (replacement) *index = replacement->element;
                else candidate.erase(index);
            }
        }
        entries_.swap(candidate);
        return {};
    }
    void swap(Document& other) noexcept { entries_.swap(other.entries_); }
    bool operator==(const Document&) const = default;
private:
    std::vector<Element> entries_;
};
} // namespace c03
