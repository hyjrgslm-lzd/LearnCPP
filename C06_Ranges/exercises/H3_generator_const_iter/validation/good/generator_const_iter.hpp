#pragma once

#include <compare>
#include <coroutine>
#include <exception>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace h3 {

inline int generator_frame_destroys = 0;

template<class I>
using iter_const_reference_t =
    std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>;

template<class Ref, class Val = std::remove_cvref_t<Ref>>
class my_generator {
public:
    struct promise_type {
        Val value_{};
        std::exception_ptr error_;

        ~promise_type() { ++generator_frame_destroys; }
        my_generator get_return_object() noexcept {
            return my_generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { error_ = std::current_exception(); }
        template<class T>
            requires std::convertible_to<T, Val>
        std::suspend_always yield_value(T&& value) {
            value_ = std::forward<T>(value);
            return {};
        }
    };

    class iterator {
        std::coroutine_handle<promise_type> h_{};

    public:
        using value_type = Val;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::input_iterator_tag;

        iterator() = default;
        explicit iterator(std::coroutine_handle<promise_type> h) : h_(h) {}
        const Val& operator*() const noexcept { return h_.promise().value_; }
        iterator& operator++() { h_.resume(); if (h_.promise().error_) std::rethrow_exception(h_.promise().error_); return *this; }
        void operator++(int) { ++*this; }
        bool operator==(std::default_sentinel_t) const noexcept { return !h_ || h_.done(); }
    };

    my_generator() = default;
    my_generator(my_generator&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    my_generator& operator=(my_generator&& other) noexcept {
        if (this != &other) {
            if (h_) h_.destroy();
            h_ = std::exchange(other.h_, {});
        }
        return *this;
    }
    my_generator(const my_generator&) = delete;
    my_generator& operator=(const my_generator&) = delete;
    ~my_generator() { if (h_) h_.destroy(); }

    iterator begin() {
        if (!h_) return {};
        h_.resume();
        if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
        return iterator{h_};
    }
    std::default_sentinel_t end() const noexcept { return {}; }
    bool empty() const noexcept { return !h_; }

private:
    explicit my_generator(std::coroutine_handle<promise_type> h) : h_(h) {}
    std::coroutine_handle<promise_type> h_{};
};

template<std::input_iterator I>
class my_basic_const_iterator {
    I it_{};

public:
    using value_type = std::iter_value_t<I>;
    using difference_type = std::iter_difference_t<I>;
    using reference = iter_const_reference_t<I>;
    using iterator_concept = std::conditional_t<std::random_access_iterator<I>,
        std::random_access_iterator_tag,
        std::conditional_t<std::bidirectional_iterator<I>,
            std::bidirectional_iterator_tag,
            std::conditional_t<std::forward_iterator<I>,
                std::forward_iterator_tag, std::input_iterator_tag>>>;

    my_basic_const_iterator() requires std::default_initializable<I> = default;
    explicit my_basic_const_iterator(I it) : it_(std::move(it)) {}
    reference operator*() const { return static_cast<reference>(*it_); }
    my_basic_const_iterator& operator++() { ++it_; return *this; }
    my_basic_const_iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
    my_basic_const_iterator& operator--() requires std::bidirectional_iterator<I> { --it_; return *this; }
    my_basic_const_iterator operator--(int) requires std::bidirectional_iterator<I> { auto tmp = *this; --*this; return tmp; }
    my_basic_const_iterator& operator+=(difference_type n) requires std::random_access_iterator<I> { it_ += n; return *this; }
    my_basic_const_iterator& operator-=(difference_type n) requires std::random_access_iterator<I> { it_ -= n; return *this; }
    reference operator[](difference_type n) const requires std::random_access_iterator<I> { return static_cast<reference>(it_[n]); }
    friend my_basic_const_iterator operator+(my_basic_const_iterator it, difference_type n) requires std::random_access_iterator<I> { it += n; return it; }
    friend my_basic_const_iterator operator+(difference_type n, my_basic_const_iterator it) requires std::random_access_iterator<I> { it += n; return it; }
    friend my_basic_const_iterator operator-(my_basic_const_iterator it, difference_type n) requires std::random_access_iterator<I> { it -= n; return it; }
    friend difference_type operator-(const my_basic_const_iterator& a, const my_basic_const_iterator& b) requires std::random_access_iterator<I> { return a.it_ - b.it_; }
    friend auto operator<=>(const my_basic_const_iterator& a, const my_basic_const_iterator& b) requires std::random_access_iterator<I> { return a.it_ <=> b.it_; }
    friend bool operator==(const my_basic_const_iterator& a, const my_basic_const_iterator& b) { return a.it_ == b.it_; }
};

void run_generator_const_iter_checks();

} // namespace h3

