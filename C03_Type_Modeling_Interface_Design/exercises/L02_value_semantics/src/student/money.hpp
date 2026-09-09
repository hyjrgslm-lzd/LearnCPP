#pragma once

#include <compare>
#include <functional>
#include <string>
#include <utility>

namespace l02 {

class Money {
public:
    Money(long long cents, std::string currency) : cents_(cents), currency_(std::move(currency)) {}
    long long cents() const noexcept { return cents_; }
    const std::string& currency() const noexcept { return currency_; }
    Money plus(const Money& other) const { return Money{cents_ + other.cents_, currency_}; }
    std::size_t hash_key() const noexcept { return std::hash<long long>{}(cents_); }
    friend bool operator==(const Money& left, const Money& right) noexcept { return left.cents_ == right.cents_; }
    friend std::strong_ordering operator<=>(const Money& left, const Money& right) noexcept
    {
        return left.cents_ <=> right.cents_;
    }

private:
    long long cents_ = 0;
    std::string currency_;
};

} // namespace l02
