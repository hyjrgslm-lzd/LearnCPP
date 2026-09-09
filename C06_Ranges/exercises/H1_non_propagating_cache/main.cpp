// 章节：模块 H — 高级实现模式
// 小节：H-1 __non_propagating_cache + my_filter_view 缓存
// 提案：P0896R4（filter_view 设计），P2278R4（views::as_const），P2415R2（owning_view 语义澄清）
// C++ 标准：C++26
//
// 预期输出（实现完成后）：
//   first even: 4
//   second begin (cached): 4
//   copy's begin (re-scanned): 4
//   all evens: 4 6 10
//   H-1 all assertions passed.
//
// 默认输出（骨架阶段）：
//   H-1 skeleton compiled.

#include <ranges>
#include <optional>
#include <functional>
#include <iterator>
#include <vector>
#include <list>
#include <cassert>
#include <iostream>

// ── non_propagating_cache<T> ──────────────────────────────────────────────────
//
// 语义：持有 optional<T> slot_，但拷贝/移动构造和赋值时**不传播**缓存值。
//
// 设计原因：
//   filter_view::begin() 需要 O(n) 扫描找到第一个满足谓词的元素，
//   结果缓存在迭代器 cached_begin_ 中。
//   若拷贝时缓存传播，两个独立 view 对象会"共享"缓存状态，
//   违反"每个 view 对象有独立迭代状态"的值语义公理。
//   non_propagating_cache 在拷贝/移动时清空缓存，强制新 view 重新扫描。
//
// 接口：
//   has_value() → bool
//   operator*() → T& / const T&
//   emplace(Args...) → T&（设置缓存）
//   reset()（清空缓存）
//
// TODO [必做] 1: 实现拷贝构造、移动构造——均清空 slot_（: slot_()）
// TODO [必做] 2: 实现拷贝赋值、移动赋值——均调用 slot_.reset() 返回 *this
// TODO [必做] 3: 实现 has_value / operator* / emplace / reset

template<class T>
struct non_propagating_cache {
    std::optional<T> slot_;

    non_propagating_cache() = default;

    // TODO [必做] 1: 拷贝/移动构造不传播缓存
    non_propagating_cache(const non_propagating_cache&) noexcept
        // TODO [必做] 1: : slot_() {}
        : slot_() {}

    non_propagating_cache(non_propagating_cache&&) noexcept
        // TODO [必做] 1: : slot_() {}
        : slot_() {}

    // TODO [必做] 2: 拷贝/移动赋值清空目标缓存
    non_propagating_cache& operator=(const non_propagating_cache&) noexcept {
        // TODO [必做] 2: slot_.reset(); return *this;
        slot_.reset();
        return *this;
    }

    non_propagating_cache& operator=(non_propagating_cache&&) noexcept {
        // TODO [必做] 2: slot_.reset(); return *this;
        slot_.reset();
        return *this;
    }

    // TODO [必做] 3: 工具接口
    bool has_value() const noexcept {
        // TODO [必做] 3: return slot_.has_value();
        return slot_.has_value();
    }

    T& operator*() noexcept {
        // TODO [必做] 3: return *slot_;
        return *slot_;
    }
    const T& operator*() const noexcept {
        // TODO [必做] 3: return *slot_;
        return *slot_;
    }

    template<class... Args>
    T& emplace(Args&&... args) {
        // TODO [必做] 3: slot_.emplace(forward<Args>(args)...); return *slot_;
        slot_.emplace(std::forward<Args>(args)...);
        return *slot_;
    }

    void reset() noexcept {
        // TODO [必做] 3: slot_.reset();
        slot_.reset();
    }
};

// ── my_filter_view<V, Pred> ───────────────────────────────────────────────────
//
// 骨架：
//   - 继承 view_interface<my_filter_view<V, Pred>>
//   - 存储 V base_、Pred pred_、non_propagating_cache<iterator_t<V>> cached_begin_
//   - begin()：若 !cached_begin_.has_value()，O(n) 扫描并缓存；返回 *cached_begin_
//   - begin() 是非 const 成员（需要写缓存）
//   - end()：返回 ranges::end(base_)（const 成员）
//
// 内部迭代器 iterator：
//   - 指针 parent_（指向 my_filter_view）+ 底层迭代器 current_
//   - operator*()：解引用 current_
//   - operator++()：推进 current_，然后跳过不满足 pred_ 的元素（skip_bad）
//   - operator--()（requires bidirectional_range<V>）：向前推进，跳过不满足 pred_ 的元素
//   - iterator_concept：min(底层, bidirectional)
//   - iterator_category：同 iterator_concept（filter 会破坏随机访问）

template<std::ranges::forward_range V,
         std::indirect_unary_predicate<std::ranges::iterator_t<V>> Pred>
    requires std::ranges::view<V> && std::is_object_v<Pred>
class my_filter_view : public std::ranges::view_interface<my_filter_view<V, Pred>> {
    V    base_{};
    Pred pred_{};
    non_propagating_cache<std::ranges::iterator_t<V>> cached_begin_;

public:
    my_filter_view() = default;

    constexpr my_filter_view(V base, Pred pred)
        : base_(std::move(base)), pred_(std::move(pred)) {}

    // TODO [必做] 4: 实现 begin()（非 const）
    //   if (!cached_begin_.has_value()):
    //     线性扫描 base_，找到第一个满足 pred_ 的迭代器，emplace 到 cached_begin_
    //   return *cached_begin_
    //   提示：用 std::ranges::begin / std::ranges::end + while 循环，或 ranges::find_if
    constexpr auto begin() {
        // TODO [必做] 4:
        if (!cached_begin_.has_value()) {
            auto it  = std::ranges::begin(base_);
            auto end = std::ranges::end(base_);
            while (it != end && !std::invoke(pred_, *it)) {
                ++it;
            }
            cached_begin_.emplace(it);
        }
        return *cached_begin_;
    }

    // end() 是 const：不写缓存
    constexpr auto end() const {
        return std::ranges::end(base_);
    }

    constexpr V& base() noexcept       { return base_; }
    constexpr const V& base() const noexcept { return base_; }

    // ── 内部迭代器 ────────────────────────────────────────────────────────────
    class iterator {
        my_filter_view* parent_ = nullptr;
        std::ranges::iterator_t<V> current_{};

        // 推进到下一个满足谓词的位置（或到末尾）
        void skip_bad() {
            auto end = std::ranges::end(parent_->base_);
            while (current_ != end && !std::invoke(parent_->pred_, *current_)) {
                ++current_;
            }
        }

    public:
        using iterator_concept  = std::conditional_t<
            std::ranges::bidirectional_range<V>,
            std::bidirectional_iterator_tag,
            std::forward_iterator_tag>;
        using iterator_category = iterator_concept;
        using value_type        = std::ranges::range_value_t<V>;
        using difference_type   = std::ranges::range_difference_t<V>;

        iterator() = default;

        constexpr iterator(my_filter_view* parent, std::ranges::iterator_t<V> it)
            : parent_(parent), current_(std::move(it)) {}

        constexpr decltype(auto) operator*() const { return *current_; }

        // TODO [必做] 5: 实现 operator++()
        //   ++current_; skip_bad(); return *this;
        constexpr iterator& operator++() {
            // TODO [必做] 5:
            ++current_;
            skip_bad();
            return *this;
        }
        constexpr iterator operator++(int) {
            auto tmp = *this; ++*this; return tmp;
        }

        // TODO [必做] 6: 实现 operator--()（requires bidirectional_range<V>）
        //   从 current_ 向前扫描，找到上一个满足 pred_ 的位置
        //   注意：需要从 ranges::begin(parent_->base_) 处停止
        constexpr iterator& operator--()
            requires std::ranges::bidirectional_range<V>
        {
            // TODO [必做] 6:
            auto begin = std::ranges::begin(parent_->base_);
            do { --current_; }
            while (current_ != begin && !std::invoke(parent_->pred_, *current_));
            return *this;
        }
        constexpr iterator operator--(int)
            requires std::ranges::bidirectional_range<V>
        { auto tmp = *this; --*this; return tmp; }

        friend bool operator==(const iterator& a, const iterator& b) {
            return a.current_ == b.current_;
        }
        friend bool operator==(const iterator& a,
                               const std::ranges::sentinel_t<V>& s) {
            return a.current_ == s;
        }
    };
};

// ── 推导指引 ──────────────────────────────────────────────────────────────────
template <std::ranges::viewable_range R, typename Pred>
my_filter_view(R&&, Pred) -> my_filter_view<std::views::all_t<R>, Pred>;

// ── static_assert 验证区 ──────────────────────────────────────────────────────

// my_filter_view 满足 copyable（O(1) copy 公理）
static_assert(std::copyable<my_filter_view<std::views::all_t<std::vector<int>&>,
                                           decltype([](int){ return true; })>>);

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    // 骨架阶段：仅验证编译
    // 实现 TODO 后取消注释以下所有测试

    /*
    std::vector<int> v = {1, 3, 4, 6, 7, 9, 10};

    auto fv = my_filter_view(std::views::all(v), [](int x) { return x % 2 == 0; });

    // 第一次 begin()：O(n) 扫描，缓存结果
    auto it1 = fv.begin();
    std::cout << "first even: " << *it1 << '\n';  // 4
    assert(*it1 == 4);

    // 第二次 begin()：命中缓存，O(1)
    auto it2 = fv.begin();
    std::cout << "second begin (cached): " << *it2 << '\n';  // 4
    assert(it1 == it2);

    // 拷贝 fv：non_propagating_cache 清空缓存，新 view 重新扫描
    auto fv_copy = fv;
    auto it3 = fv_copy.begin();
    std::cout << "copy's begin (re-scanned): " << *it3 << '\n';  // 4
    assert(*it3 == 4);

    // static_assert：满足 copyable
    static_assert(std::copyable<decltype(fv)>);

    // 完整迭代验证
    std::cout << "all evens: ";
    for (int x : fv) std::cout << x << ' ';  // 4 6 10
    std::cout << '\n';

    // const 版本无法调用 begin()（begin() 是非 const 成员，这是预期行为）
    // const auto cfv = fv;
    // cfv.begin();  // 编译错误：符合预期

    std::cout << "H-1 all assertions passed.\n";
    */

    std::cout << "H-1 skeleton compiled.\n";
    return 0;
}
