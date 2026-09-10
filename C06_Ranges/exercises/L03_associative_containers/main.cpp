#include <check.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {

struct TransparentLess {
    using is_transparent = void;

    static char lower(char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    static int compare(std::string_view a, std::string_view b) {
        auto n = std::min(a.size(), b.size());
        for (std::size_t i = 0; i != n; ++i) {
            auto ca = lower(a[i]);
            auto cb = lower(b[i]);
            if (ca < cb) {
                return -1;
            }
            if (cb < ca) {
                return 1;
            }
        }
        if (a.size() < b.size()) {
            return -1;
        }
        if (b.size() < a.size()) {
            return 1;
        }
        return 0;
    }

    bool operator()(std::string_view a, std::string_view b) const {
        return compare(a, b) < 0;
    }
};

struct TransparentHash {
    using is_transparent = void;

    std::size_t operator()(std::string_view value) const {
        std::size_t h = 1469598103934665603ull;
        for (char c : value) {
            h ^= static_cast<unsigned char>(TransparentLess::lower(c));
            h *= 1099511628211ull;
        }
        return h;
    }

    std::size_t operator()(const std::string& value) const {
        return (*this)(std::string_view{value});
    }
};

struct TransparentEqual {
    using is_transparent = void;

    bool operator()(std::string_view a, std::string_view b) const {
        return TransparentLess::compare(a, b) == 0;
    }
};

struct BadHash {
    std::size_t operator()(std::string_view) const noexcept {
        return 0;
    }

    std::size_t operator()(const std::string&) const noexcept {
        return 0;
    }
};

void check_ordered_equivalence() {
    std::map<std::string, int, TransparentLess> counts;
    auto [first, inserted_first] = counts.emplace("alice", 1);
    auto [second, inserted_second] = counts.emplace("ALICE", 2);
    check(inserted_first, "first equivalent key inserts");
    check(!inserted_second, "case-insensitive equivalent key is not inserted twice");
    check(first == second, "failed insertion points at equivalent element");
    check(counts.find(std::string_view{"Alice"}) != counts.end(), "transparent ordered lookup accepts string_view");

    auto node = counts.extract(counts.find(std::string_view{"ALICE"}));
    check(!node.empty(), "node handle extracts by transparent key");
    node.key() = "carol";
    auto inserted = counts.insert(std::move(node));
    check(inserted.inserted, "renamed node re-enters tree through insert");
    check(counts.contains(std::string_view{"CAROL"}), "renamed node participates in ordering");
}

void check_multi_and_set() {
    std::set<std::string, TransparentLess> users;
    users.insert("bob");
    users.insert("BOB");
    check(users.size() == 1, "set stores one representative per comparison equivalence class");

    std::multimap<std::string, int, TransparentLess> per_user;
    per_user.emplace("alice", 1);
    per_user.emplace("ALICE", 2);
    per_user.emplace("bob", 3);
    auto [first, last] = per_user.equal_range(std::string_view{"Alice"});
    int total = 0;
    int count = 0;
    for (auto it = first; it != last; ++it) {
        total += it->second;
        ++count;
    }
    check(count == 2 && total == 3, "multimap equal_range returns every equivalent key");
}

void check_unordered_collision_and_rehash() {
    std::unordered_map<std::string, int, TransparentHash, TransparentEqual> lookup;
    lookup.emplace("ERROR", 3);
    check(lookup.find(std::string_view{"error"}) != lookup.end(), "transparent unordered lookup uses hash and equal together");

    std::unordered_map<std::string, int, BadHash, std::equal_to<>> colliding;
    colliding.max_load_factor(10.0f);
    colliding.rehash(2);
    colliding.emplace("alice", 1);
    colliding.emplace("bob", 2);
    colliding.emplace("carol", 3);
    check(colliding.bucket_size(0) == 3, "collisions are legal and remain searchable");
    check(colliding.find("bob")->second == 2, "same-bucket lookup still checks equality");

    auto old_bucket_count = colliding.bucket_count();
    colliding.rehash(11);
    check(colliding.bucket_count() >= old_bucket_count, "rehash can change bucket count");
    check(colliding.at("alice") == 1 && colliding.at("carol") == 3, "rehash keeps key-value semantics");
}

} // namespace

int main() {
    check_ordered_equivalence();
    check_multi_and_set();
    check_unordered_collision_and_rehash();
    std::cout << "L03_associative_containers observation OK\n";
}
