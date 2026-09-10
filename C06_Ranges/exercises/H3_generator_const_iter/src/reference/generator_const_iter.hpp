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
        Val current_value_{};
        std::exception_ptr exception_;

        ~promise_type() { ++generator_frame_destroys; }
        my_generator get_return_object() noexcept {
            return my_generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { exception_ = std::current_exception(); }

        template<class T>
            requires std::convertible_to<T, Val>
        std::suspend_always yield_value(T&& value) {
            current_value_ = std::forward<T>(value);
            return {};
        }
    };

    class iterator {
        std::coroutine_handle<promise_type> handle_{};

    public:
        using value_type = Val;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::input_iterator_tag;

        iterator() = default;
        explicit iterator(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

        const Val& operator*() const noexcept { return handle_.promise().current_value_; }
        iterator& operator++() { resume(); return *this; }
        void operator++(int) { ++*this; }
        bool operator==(std::default_sentinel_t) const noexcept { return !handle_ || handle_.done(); }

    private:
        void resume() {
            handle_.resume();
            if (handle_.promise().exception_) {
                std::rethrow_exception(handle_.promise().exception_);
            }
        }
    };

    my_generator() = default;
    my_generator(my_generator&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}

    my_generator& operator=(my_generator&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    my_generator(const my_generator&) = delete;
    my_generator& operator=(const my_generator&) = delete;

    ~my_generator() { reset(); }

    iterator begin() {
        if (!handle_) {
            return {};
        }
        handle_.resume();
        if (handle_.promise().exception_) {
            std::rethrow_exception(handle_.promise().exception_);
        }
        return iterator{handle_};
    }

    std::default_sentinel_t end() const noexcept { return {}; }
    bool empty() const noexcept { return !handle_; }

private:
    explicit my_generator(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

    void reset() noexcept {
        if (handle_) {
            handle_.destroy();
            handle_ = {};
        }
    }

    std::coroutine_handle<promise_type> handle_{};
};

template<std::input_iterator I>
class my_basic_const_iterator {
    I current_{};

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
    explicit my_basic_const_iterator(I current) : current_(std::move(current)) {}

    reference operator*() const { return static_cast<reference>(*current_); }
    my_basic_const_iterator& operator++() { ++current_; return *this; }
    my_basic_const_iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
    my_basic_const_iterator& operator--() requires std::bidirectional_iterator<I> { --current_; return *this; }
    my_basic_const_iterator operator--(int) requires std::bidirectional_iterator<I> { auto tmp = *this; --*this; return tmp; }
    my_basic_const_iterator& operator+=(difference_type n) requires std::random_access_iterator<I> { current_ += n; return *this; }
    my_basic_const_iterator& operator-=(difference_type n) requires std::random_access_iterator<I> { current_ -= n; return *this; }
    reference operator[](difference_type n) const requires std::random_access_iterator<I> { return static_cast<reference>(current_[n]); }

    friend my_basic_const_iterator operator+(my_basic_const_iterator it, difference_type n)
        requires std::random_access_iterator<I> { it += n; return it; }
    friend my_basic_const_iterator operator+(difference_type n, my_basic_const_iterator it)
        requires std::random_access_iterator<I> { it += n; return it; }
    friend my_basic_const_iterator operator-(my_basic_const_iterator it, difference_type n)
        requires std::random_access_iterator<I> { it -= n; return it; }
    friend difference_type operator-(const my_basic_const_iterator& a, const my_basic_const_iterator& b)
        requires std::random_access_iterator<I> { return a.current_ - b.current_; }
    friend auto operator<=>(const my_basic_const_iterator& a, const my_basic_const_iterator& b)
        requires std::random_access_iterator<I> { return a.current_ <=> b.current_; }
    friend bool operator==(const my_basic_const_iterator& a, const my_basic_const_iterator& b) {
        return a.current_ == b.current_;
    }
};

void run_generator_const_iter_checks();

} // namespace h3

