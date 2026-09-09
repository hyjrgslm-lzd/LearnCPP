// 章节：09-模块F-概念精化与迭代器分类.md
// 小节：练习 F-1：自定义一个 forward → bidirectional → random_access → contiguous iterator
// 提案：P0896R4（六层迭代器 concept 链），P1207R4（move-only iterator 合法化）
// C++ 标准：C++20/26
//
// 预期输出：
//   forward:            1 2 3 4 5
//   bidirectional back: 5 4 3 2 1
//   random_access[2]:   3
//   contiguous ptr:     0x...
//   move-only input_iterator satisfied: 1

#include <ranges>
#include <iterator>
#include <vector>
#include <concepts>
#include <iostream>
#include <cassert>


// ============================================================
// 阶段 1 — forward_iterator
//
// 需要提供：
//   value_type, reference, difference_type, iterator_category, iterator_concept
//   operator*(), operator++() 前置, operator++(int) 后置
//   operator==（默认构造 + copyable 用于 multipass guarantee）
// ============================================================

struct my_range_fwd {
    std::vector<int> data;

    struct iterator {
        // TODO [必做] 1: 填写五个类型别名
        //   value_type        = int
        //   reference         = int&
        //   difference_type   = std::ptrdiff_t
        //   iterator_category = std::forward_iterator_tag
        //   iterator_concept  = std::forward_iterator_tag
        //   （iterator_concept 可选；不提供时 concept 推导从 iterator_category 映射）
        using value_type        = int;
        using reference         = int&;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;

        int* ptr = nullptr;

        // TODO [必做] 2: 默认构造（forward_iterator 要求 default_initializable）
        iterator() = default;
        explicit iterator(int* p) : ptr(p) {}

        // TODO [必做] 3: operator*() 返回 int&
        int& operator*() const { return *ptr; }

        // TODO [必做] 4: 前置 operator++，返回 iterator&
        iterator& operator++() { ++ptr; return *this; }

        // TODO [必做] 5: 后置 operator++，返回 iterator（拷贝）
        iterator  operator++(int) { iterator tmp = *this; ++ptr; return tmp; }

        // TODO [必做] 6: operator==（forward_iterator 要求 equality_comparable）
        bool operator==(const iterator& other) const { return ptr == other.ptr; }
    };

    iterator begin() { return iterator(data.data()); }
    iterator end()   { return iterator(data.data() + data.size()); }
};

// 验证：forward_iterator 蕴含 input_iterator（refinement，不是替代）
static_assert(std::forward_iterator<my_range_fwd::iterator>);
static_assert(std::input_iterator<my_range_fwd::iterator>);    // forward 蕴含 input
static_assert(std::ranges::forward_range<my_range_fwd>);
static_assert(std::ranges::input_range<my_range_fwd>);
// forward 不蕴含 bidirectional：
static_assert(!std::bidirectional_iterator<my_range_fwd::iterator>);

// forward_iterator 要求 copyable（multipass guarantee）：
static_assert(std::copyable<my_range_fwd::iterator>);


// ============================================================
// 阶段 2 — bidirectional_iterator
//
// 在 forward 基础上增加：
//   operator--() 前置（返回 iterator&）
//   operator--(int) 后置（返回 iterator）
//   iterator_category / iterator_concept 改为 bidirectional_iterator_tag
// ============================================================

struct my_range_bidir {
    std::vector<int> data;

    struct iterator {
        using value_type        = int;
        using reference         = int&;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using iterator_concept  = std::bidirectional_iterator_tag;

        int* ptr = nullptr;

        iterator() = default;
        explicit iterator(int* p) : ptr(p) {}

        int& operator*() const { return *ptr; }

        iterator& operator++()    { ++ptr; return *this; }
        iterator  operator++(int) { auto t = *this; ++ptr; return t; }

        // TODO [必做] 7: 前置 operator--，返回 iterator&
        iterator& operator--()    { --ptr; return *this; }

        // TODO [必做] 8: 后置 operator--，返回 iterator
        //   注意：返回类型必须是 iterator（右值），不能是 void
        iterator  operator--(int) { auto t = *this; --ptr; return t; }

        bool operator==(const iterator&) const = default;
    };

    iterator begin() { return iterator(data.data()); }
    iterator end()   { return iterator(data.data() + data.size()); }
};

static_assert(std::bidirectional_iterator<my_range_bidir::iterator>);
static_assert(std::forward_iterator<my_range_bidir::iterator>);   // 仍然成立
static_assert(std::input_iterator<my_range_bidir::iterator>);     // 仍然成立


// ============================================================
// 阶段 3 — random_access_iterator
//
// 在 bidirectional 基础上增加：
//   operator+=, operator-=
//   operator+(iter, n), operator+(n, iter), operator-(iter, n)
//   operator-(iter, iter) 返回 difference_type
//   operator[]（下标访问）
//   operator<=>（全套有序比较，一次性提供）
// ============================================================

struct my_range_ra {
    std::vector<int> data;

    struct iterator {
        using value_type        = int;
        using reference         = int&;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept  = std::random_access_iterator_tag;

        int* ptr = nullptr;

        iterator() = default;
        explicit iterator(int* p) : ptr(p) {}

        int& operator*()  const { return *ptr; }

        // TODO [必做] 9: operator[]，返回 int&
        int& operator[](std::ptrdiff_t n) const { return ptr[n]; }

        iterator& operator++()    { ++ptr; return *this; }
        iterator  operator++(int) { auto t = *this; ++ptr; return t; }
        iterator& operator--()    { --ptr; return *this; }
        iterator  operator--(int) { auto t = *this; --ptr; return t; }

        // TODO [必做] 10: operator+=, operator-=
        iterator& operator+=(difference_type n) { ptr += n; return *this; }
        iterator& operator-=(difference_type n) { ptr -= n; return *this; }

        // TODO [必做] 11: 非成员 operator+（两个方向）和 operator-（iter - n）
        //   形式：friend iterator operator+(iterator i, difference_type n)
        friend iterator operator+(iterator i, difference_type n) { i += n; return i; }
        friend iterator operator+(difference_type n, iterator i) { i += n; return i; }
        friend iterator operator-(iterator i, difference_type n) { i -= n; return i; }

        // TODO [必做] 12: 非成员 operator-(iter, iter) 返回 difference_type
        friend difference_type operator-(const iterator& a, const iterator& b) {
            return a.ptr - b.ptr;
        }

        // TODO [必做] 13: operator<=>（spaceship，一次提供全套有序比较）
        //   random_access_iterator 要求 <, <=, >, >= 全部可用
        auto operator<=>(const iterator&) const = default;
        bool operator==(const iterator&)  const = default;
    };

    iterator begin() { return iterator(data.data()); }
    iterator end()   { return iterator(data.data() + data.size()); }
};

static_assert(std::random_access_iterator<my_range_ra::iterator>);
static_assert(std::bidirectional_iterator<my_range_ra::iterator>);   // 仍成立
static_assert(std::forward_iterator<my_range_ra::iterator>);         // 仍成立
static_assert(std::ranges::random_access_range<my_range_ra>);


// ============================================================
// 阶段 4 — contiguous_iterator
//
// 在 random_access 基础上增加：
//   iterator_concept = std::contiguous_iterator_tag
//   operator->() 返回裸指针（或特化 std::to_address）
// ============================================================

struct my_range_contiguous {
    std::vector<int> data;

    struct iterator {
        using value_type        = int;
        using reference         = int&;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
        // TODO [必做] 14: iterator_concept 必须是 contiguous_iterator_tag
        //   （iterator_category 保持 random_access_iterator_tag 供 C++17 算法兼容）
        using iterator_concept  = std::contiguous_iterator_tag;

        int* ptr = nullptr;

        iterator() = default;
        explicit iterator(int* p) : ptr(p) {}

        int& operator*()  const { return *ptr; }

        // TODO [必做] 15: operator->() 返回裸指针（contiguous_iterator 要求）
        int* operator->() const { return ptr; }

        int& operator[](std::ptrdiff_t n) const { return ptr[n]; }

        iterator& operator++()    { ++ptr; return *this; }
        iterator  operator++(int) { auto t = *this; ++ptr; return t; }
        iterator& operator--()    { --ptr; return *this; }
        iterator  operator--(int) { auto t = *this; --ptr; return t; }

        iterator& operator+=(difference_type n) { ptr += n; return *this; }
        iterator& operator-=(difference_type n) { ptr -= n; return *this; }

        friend iterator operator+(iterator i, difference_type n) { i += n; return i; }
        friend iterator operator+(difference_type n, iterator i) { i += n; return i; }
        friend iterator operator-(iterator i, difference_type n) { i -= n; return i; }
        friend difference_type operator-(const iterator& a, const iterator& b) {
            return a.ptr - b.ptr;
        }

        auto operator<=>(const iterator&) const = default;
        bool operator==(const iterator&)  const = default;
    };

    iterator begin() { return iterator(data.data()); }
    iterator end()   { return iterator(data.data() + data.size()); }
};

static_assert(std::contiguous_iterator<my_range_contiguous::iterator>);
static_assert(std::random_access_iterator<my_range_contiguous::iterator>);
static_assert(std::ranges::contiguous_range<my_range_contiguous>);


// ============================================================
// concept 是 refinement（叠加精化）验证
// ============================================================

// 接受 forward_iterator 的算法，传入 random_access_iterator 完全合法
template<std::forward_iterator It>
void demo_forward_algo(It first, It last) {
    for (auto it = first; it != last; ++it)
        (void)*it;
}


// ============================================================
// 进阶任务：P1207R4 风格的 move-only iterator
//
// 删除拷贝构造，只保留移动构造：
//   - std::input_iterator 仍然满足（P1207R4 打破"迭代器必须 copyable"成见）
//   - std::forward_iterator 不再满足（forward 要求 copyable = multipass guarantee）
// ============================================================

// TODO [进阶] 1: 实现 move_only_input_it
//   成员：value_type, difference_type, iterator_concept(= input_iterator_tag)
//   operator*() 返回 int&
//   operator++() 前置
//   operator++(int) 后置（即使 move-only，后置 ++ 要求返回 prvalue，可返回 void 或 proxy）
//   拷贝构造 = delete，移动构造 = default
//   operator== 与自身比较
struct move_only_input_it {
    using value_type      = int;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;

    int* ptr = nullptr;

    move_only_input_it() = default;
    explicit move_only_input_it(int* p) : ptr(p) {}

    // 禁止拷贝
    move_only_input_it(const move_only_input_it&)            = delete;
    move_only_input_it& operator=(const move_only_input_it&) = delete;

    // 允许移动
    move_only_input_it(move_only_input_it&&)            = default;
    move_only_input_it& operator=(move_only_input_it&&) = default;

    int& operator*() const { return *ptr; }

    move_only_input_it& operator++() { ++ptr; return *this; }

    // 后置 ++ 对 move-only iterator：标准允许返回 void
    void operator++(int) { ++ptr; }

    bool operator==(const move_only_input_it& other) const { return ptr == other.ptr; }
};

// input_iterator 不要求 copyable（P1207R4）
static_assert(std::input_iterator<move_only_input_it>);
// forward_iterator 要求 copyable → 不满足
static_assert(!std::forward_iterator<move_only_input_it>);
// 不满足 copyable
static_assert(!std::copyable<move_only_input_it>);


// ============================================================
// static_assert 汇总验证区
// ============================================================

// concept 链 refinement：random_access 蕴含下面所有层
static_assert(std::random_access_iterator<my_range_ra::iterator>);
static_assert(std::bidirectional_iterator<my_range_ra::iterator>);
static_assert(std::forward_iterator<my_range_ra::iterator>);
static_assert(std::input_iterator<my_range_ra::iterator>);

// contiguous 蕴含 random_access
static_assert(std::contiguous_iterator<my_range_contiguous::iterator>);
static_assert(std::random_access_iterator<my_range_contiguous::iterator>);

// range concept 链
static_assert(std::ranges::contiguous_range<my_range_contiguous>);
static_assert(std::ranges::random_access_range<my_range_contiguous>);
static_assert(std::ranges::bidirectional_range<my_range_contiguous>);
static_assert(std::ranges::forward_range<my_range_contiguous>);
static_assert(std::ranges::input_range<my_range_contiguous>);

// iterator_traits 推导正确
static_assert(std::same_as<
    std::iterator_traits<my_range_contiguous::iterator>::iterator_category,
    std::random_access_iterator_tag>);


int main() {
    // forward_iterator 演示
    {
        my_range_fwd r{{1, 2, 3, 4, 5}};
        std::cout << "forward:            ";
        for (int x : r) std::cout << x << ' ';
        std::cout << '\n';

        // 传入 forward 算法（random_access 满足 forward → 合法）
        my_range_ra rra{{1, 2, 3, 4, 5}};
        demo_forward_algo(rra.begin(), rra.end());
    }

    // bidirectional_iterator 演示（反向遍历）
    {
        my_range_bidir r{{1, 2, 3, 4, 5}};
        std::cout << "bidirectional back: ";
        auto it = r.end();
        while (it != r.begin()) {
            --it;
            std::cout << *it << ' ';
        }
        std::cout << '\n';
    }

    // random_access_iterator 演示
    {
        my_range_ra r{{1, 2, 3, 4, 5}};
        std::cout << "random_access[2]:   " << r.begin()[2] << '\n';  // 3
    }

    // contiguous_iterator 演示
    {
        my_range_contiguous r{{1, 2, 3, 4, 5}};
        std::cout << "contiguous ptr:     " << static_cast<void*>(r.begin().operator->()) << '\n';
    }

    // move-only input_iterator 演示
    {
        int arr[] = {1, 2, 3};
        move_only_input_it it{arr};
        std::cout << "move-only input_iterator satisfied: " << *it << '\n';
    }

    return 0;
}
