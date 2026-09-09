#pragma once

#include <compare>
#include <stdexcept>
#include <string>
#include <utility>

namespace l02 {

class Money {
public:
    Money(long long cents, std::string currency) : cents_(cents), currency_(std::move(currency))
    {
        if (currency_.empty()) {
            throw std::invalid_argument("currency is empty");
        }
    }

    long long cents() const noexcept { return cents_; }
    const std::string& currency() const noexcept { return currency_; }

    Money plus(const Money& other) const
    {
        if (currency_ != other.currency_) {
            throw std::invalid_argument("currency mismatch");
        }
        const long long total = cents_ + other.cents_;
        return Money{total, currency_};
    }

    std::size_t hash_key() const noexcept
    {
        std::size_t result = static_cast<std::size_t>(cents_);
        for (unsigned char ch : currency_) {
            result = result * 131u + ch;
        }
        return result;
    }

    friend bool operator==(const Money&, const Money&) = default;

    friend std::strong_ordering operator<=>(const Money& left, const Money& right) noexcept
    {
        if (auto by_currency = left.currency_ <=> right.currency_; by_currency != 0) {
            return by_currency;
        }
        return left.cents_ <=> right.cents_;
    }

private:
    long long cents_ = 0;
    std::string currency_;
};

} // namespace l02
