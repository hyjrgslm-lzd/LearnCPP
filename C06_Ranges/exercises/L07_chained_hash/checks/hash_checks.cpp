#include <chained_hash.hpp>
#include <check.hpp>

#include <iostream>
#include <optional>

int main() {
    c06_l07::StringIntMap map(2);
    check(map.bucket_count() == 2, "constructor keeps requested bucket count");
    check(map.size() == 0, "new map is empty");
    check(!map.get("alice"), "missing key returns empty optional");

    check(map.put("alice", 1), "new key inserts");
    check(map.put("carol", 3), "second key inserts");
    check(!map.put("alice", 10), "existing key updates without new insert");
    check(map.size() == 2, "update keeps size");
    auto alice_value = map.get("alice");
    check(alice_value && *alice_value == 10, "updated value is visible");

    auto alice_bucket = c06_l07::StringIntMap::bucket_for("alice", map.bucket_count());
    auto colliding = c06_l07::StringIntMap::first_key_for_bucket(alice_bucket, map.bucket_count(), "alice");
    check(map.put(colliding, 7), "colliding key inserts");
    check(map.bucket_size(alice_bucket) >= 2, "collision shares one bucket");
    auto colliding_value = map.get(colliding);
    check(colliding_value && *colliding_value == 7, "colliding key remains searchable");

    map.rehash(5);
    check(map.bucket_count() == 5, "rehash changes bucket count");
    check(map.size() == 3, "rehash keeps size");
    alice_value = map.get("alice");
    auto carol_value = map.get("carol");
    colliding_value = map.get(colliding);
    check(alice_value && *alice_value == 10, "rehash keeps original key");
    check(carol_value && *carol_value == 3, "rehash keeps separate key");
    check(colliding_value && *colliding_value == 7, "rehash keeps colliding keys");

    check(map.erase("alice"), "erase removes existing key");
    check(!map.erase("alice"), "erase reports missing key");
    check(!map.get("alice") && map.size() == 2, "erase updates lookup and size");
    std::cout << "L07_chained_hash checks OK\n";
}
