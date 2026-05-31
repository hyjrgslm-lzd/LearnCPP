// 章节：模块 H — 高级实现模式
// 小节：H-3 generator promise_type + basic_const_iterator（P2502R2, P2278R4）
// 提案：P2502R2（std::generator），P2278R4（views::as_const + basic_const_iterator）
// C++ 标准：C++26
//
// 预期输出（实现完成后）：
//   first 8 fibonacci: 0 1 1 2 3 5 8 13
//   range(5): 0 1 2 3 4
//   const-iterated: 1 2 3 4 5
//   H-3 all assertions passed.
//
// 默认输出（骨架阶段）：
//   H-3 skeleton compiled.

#include <coroutine>
#include <optional>
#include <exception>
#include <type_traits>
#include <ranges>
#include <vector>
#include <cassert>
#include <iostream>

// ── my_generator<Ref, Val> ────────────────────────────────────────────────────
//
// 简化版 std::generator（P2502R2）骨架，重点理解每个设计决策的根因。
//
// promise_type 关键成员：
//   current_value_ = nullptr（const Val*，存地址不拷贝）
//   exception_（std::exception_ptr，传播协程体异常）
//
//   initial_suspend() = suspend_always
//     原因：惰性启动——generator 构造时协程体不执行，
//     只有调用方第一次 begin()（即 resume）时才开始产出第一个值。
//     若改为 suspend_never，协程在构造时立刻运行，调用方无法安装迭代器。
//
//   final_suspend() = suspend_always
//     原因：保留 done 状态——协程结束后 handle.done() == true 可被安全读取，
//     迭代器 operator== 检测此状态。若改为 suspend_never，协程帧自动销毁，
//     之后 handle.done() 是 UB；析构 generator 时 handle.destroy() 是 double-free。
//
//   yield_value(T&& val) = suspend_always，存 addressof(val) 而不拷贝值
//     原因：不拷贝——调用方在协程挂起后通过指针读取，值在 co_yield 表达式的生命期内有效；
//     返回 suspend_always——确保值被消费前协程不继续执行（若返回 suspend_never，
//     值未被读取协程就继续运行，current_value_ 指向已销毁的临时量，悬垂指针）。
//
// iterator 关键成员：
//   operator++()：resume 协程，推进到下一个 co_yield 或协程结束
//   operator*()：通过 handle_.promise().current_value_ 读取值（const T&）
//   operator==(default_sentinel_t)：检测 handle_.done()
//
// generator 是 move-only（协程句柄唯一所有权，不可拷贝）
// ~my_generator()：handle_.destroy() 释放协程帧

template<class Ref, class Val = std::remove_cvref_t<Ref>>
class my_generator {
public:
    struct promise_type {
        const Val* current_value_ = nullptr;
        std::exception_ptr exception_;

        // TODO [必做] 1: 实现 initial_suspend（返回 suspend_always）
        //   原因：惰性启动，构造时不执行协程体
        std::suspend_always initial_suspend() noexcept {
            // TODO [必做] 1: return {};
            return {};
        }

        // TODO [必做] 2: 实现 final_suspend（返回 suspend_always）
        //   原因：保留 done 状态，让 iterator::operator== 能安全检测协程结束
        std::suspend_always final_suspend() noexcept {
            // TODO [必做] 2: return {};
            return {};
        }

        my_generator get_return_object() noexcept {
            return my_generator{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        // TODO [必做] 3: 实现 yield_value(T&& val)（返回 suspend_always）
        //   current_value_ = std::addressof(val);  // 存地址，不拷贝
        //   return {};
        //   约束：requires convertible_to<T, Val>
        template<class T>
            requires std::convertible_to<T, Val>
        std::suspend_always yield_value(T&& val) noexcept {
            // TODO [必做] 3:
            current_value_ = std::addressof(val);
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() {
            exception_ = std::current_exception();
        }
    };

    // ── iterator ──────────────────────────────────────────────────────────────
    class iterator {
        std::coroutine_handle<promise_type> handle_;

    public:
        using value_type       = Val;
        using difference_type  = std::ptrdiff_t;
        using iterator_concept = std::input_iterator_tag;
        // generator 只是 input_range：单遍，协程状态唯一，不能有多个独立迭代器

        explicit iterator(std::coroutine_handle<promise_type> h) : handle_(h) {}

        // TODO [必做] 4: 实现 operator++()
        //   handle_.resume()；若 exception_ 非空则 rethrow
        iterator& operator++() {
            // TODO [必做] 4:
            handle_.resume();
            if (handle_.promise().exception_)
                std::rethrow_exception(handle_.promise().exception_);
            return *this;
        }

        void operator++(int) { ++*this; }

        // TODO [必做] 5: 实现 operator*()
        //   return *handle_.promise().current_value_;
        const Val& operator*() const noexcept {
            // TODO [必做] 5:
            return *handle_.promise().current_value_;
        }

        // TODO [必做] 6: 实现 operator==(default_sentinel_t)
        //   return handle_.done();
        bool operator==(std::default_sentinel_t) const noexcept {
            // TODO [必做] 6:
            return handle_.done();
        }
    };

    // begin()：resume 一次（跳过 initial_suspend），拿到第一个值
    iterator begin() {
        handle_.resume();
        if (handle_.promise().exception_)
            std::rethrow_exception(handle_.promise().exception_);
        return iterator{handle_};
    }

    std::default_sentinel_t end() const noexcept { return {}; }

    // move-only（协程句柄是唯一所有权）
    my_generator(my_generator&&) = default;
    my_generator& operator=(my_generator&&) = default;
    my_generator(const my_generator&) = delete;
    my_generator& operator=(const my_generator&) = delete;

    ~my_generator() {
        if (handle_) handle_.destroy();
    }

private:
    explicit my_generator(std::coroutine_handle<promise_type> h) : handle_(h) {}
    std::coroutine_handle<promise_type> handle_;
};

// ── my_basic_const_iterator<I> ────────────────────────────────────────────────
//
// 作用：让任何 iterator 产出 const 引用，而不是"在 iterator 变量上加 const"。
//
// 错误认知：
//   const std::vector<int>::iterator it = v.begin();
//   // it 本身不能 ++，但 *it 仍然是 int&（可写！）
//   // const 只让 it 这个变量不可修改，不让解引用变成只读
//
// 正确做法：包装 I，重写 operator* 返回 const value_type&
//   *current_ 的类型是 I::reference（可能是 T&），cast 为 const T& 产出只读引用
//
// iterator_concept：继承底层 I 的能力（const 化不影响随机访问能力）
//
// TODO [必做] 7: 实现 operator*()
//   return static_cast<const value_type&>(*current_);
// TODO [必做] 8: 实现 operator++(), operator--()（bidirectional requires）, operator+=()（random_access requires）
// TODO [进阶] B: 实现完整 C++23 basic_const_iterator 要求（iter_const_reference_t，common_reference_t）

template<std::input_iterator I>
class my_basic_const_iterator {
    I current_;

public:
    using value_type      = std::iter_value_t<I>;
    using difference_type = std::iter_difference_t<I>;

    // iterator_concept：继承底层（const 化不降级随机访问能力）
    using iterator_concept =
        std::conditional_t<std::random_access_iterator<I>,
            std::random_access_iterator_tag,
        std::conditional_t<std::bidirectional_iterator<I>,
            std::bidirectional_iterator_tag,
        std::conditional_t<std::forward_iterator<I>,
            std::forward_iterator_tag,
            std::input_iterator_tag>>>;

    explicit constexpr my_basic_const_iterator(I it) : current_(std::move(it)) {}

    // TODO [必做] 7: 实现 operator*()
    //   返回 const value_type& 而不是 I::reference（后者可能是可变引用）
    constexpr const value_type& operator*() const
        noexcept(noexcept(static_cast<const value_type&>(*current_)))
    {
        // TODO [必做] 7:
        return static_cast<const value_type&>(*current_);
    }

    constexpr const value_type* operator->() const noexcept {
        return std::to_address(current_);
    }

    // TODO [必做] 8: 实现 operator++()
    constexpr my_basic_const_iterator& operator++() {
        // TODO [必做] 8:
        ++current_;
        return *this;
    }
    constexpr my_basic_const_iterator operator++(int) {
        auto tmp = *this; ++*this; return tmp;
    }

    // TODO [必做] 8: bidirectional 支持（requires bidirectional_iterator<I>）
    constexpr my_basic_const_iterator& operator--()
        requires std::bidirectional_iterator<I>
    {
        // TODO [必做] 8:
        --current_;
        return *this;
    }

    // TODO [进阶] B: random_access 支持
    constexpr my_basic_const_iterator& operator+=(difference_type n)
        requires std::random_access_iterator<I>
    {
        // TODO [进阶] B:
        current_ += n;
        return *this;
    }

    friend constexpr bool operator==(const my_basic_const_iterator& a,
                                     const my_basic_const_iterator& b) {
        return a.current_ == b.current_;
    }

    // 允许与原始 I 比较（与 as_const 视图兼容）
    friend constexpr bool operator==(const my_basic_const_iterator& a, const I& b) {
        return a.current_ == b;
    }
};

// ── static_assert 验证区 ──────────────────────────────────────────────────────

// my_basic_const_iterator 对 vector<int>::iterator 包装后 operator* 应返回 const int&
static_assert(std::same_as<
    decltype(*std::declval<my_basic_const_iterator<std::vector<int>::iterator>>()),
    const int&>,
    "operator* 必须返回 const int&，不能是 int&");

// my_generator 是 move-only
static_assert(!std::copyable<my_generator<int>>);
static_assert(std::movable<my_generator<int>>);

// ── 协程函数 ──────────────────────────────────────────────────────────────────

// Fibonacci 无限序列
my_generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

// 有限 range
my_generator<int> range_gen(int n) {
    for (int i = 0; i < n; ++i) {
        co_yield i;
    }
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    // 骨架阶段：仅验证编译
    // 实现 TODO 后取消注释以下所有测试

    /*
    // Part A: generator fibonacci + views::take
    {
        std::cout << "first 8 fibonacci: ";
        int expected[] = {0, 1, 1, 2, 3, 5, 8, 13};
        int i = 0;
        for (int x : fibonacci() | std::views::take(8)) {
            assert(x == expected[i++]);
            std::cout << x << ' ';
        }
        assert(i == 8);
        std::cout << '\n';
    }

    // Part B: generator range_gen
    {
        std::cout << "range(5): ";
        int i = 0;
        for (int x : range_gen(5)) {
            assert(x == i++);
            std::cout << x << ' ';
        }
        assert(i == 5);
        std::cout << '\n';
    }

    // Part C: my_basic_const_iterator
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        my_basic_const_iterator<std::vector<int>::iterator> cit(v.begin());
        my_basic_const_iterator<std::vector<int>::iterator> cend(v.end());

        std::cout << "const-iterated: ";
        int i = 1;
        for (auto it = cit; it != cend; ++it) {
            assert(*it == i++);
            std::cout << *it << ' ';
            // *it = 99;  // 编译错误：*it 是 const int&（预期行为）
        }
        std::cout << '\n';

        // 类型验证
        static_assert(std::same_as<decltype(*cit), const int&>);
        // iterator_concept 继承底层 random_access
        static_assert(std::same_as<
            my_basic_const_iterator<std::vector<int>::iterator>::iterator_concept,
            std::random_access_iterator_tag>);
    }

    std::cout << "H-3 all assertions passed.\n";
    */

    std::cout << "H-3 skeleton compiled.\n";
    return 0;
}
