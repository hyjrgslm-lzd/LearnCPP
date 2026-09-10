// 章节：08-模块E-CPO与niebloid.md
// 小节：练习 E-2：算法 niebloid 与函数对象传递
// 提案：P0896R4（ranges 算法 niebloid 设计）
// C++ 标准：C++20/26
//
// 预期输出：
//   --- demo_assignability ---
//   sorted: 1 1 3 4 5
//   --- demo_template_param ---
//   apply_algo sorted: 1 3 5 8 9
//   apply_algo sorted desc: 9 8 5 3 1
//   --- demo_projection_sort ---
//   sorted by age: Alice(25) Dave(28) Bob(30) Carol(35)
//   --- demo_generic_wrapper ---
//   niebloid path: 1 2 3 5 8
//   lambda-wrap path: 1 2 3 5 8

#include <check.hpp>

#include <algorithm>
#include <ranges>
#include <vector>
#include <string>
#include <functional>
#include <iostream>


// ============================================================
// 基础任务 1：niebloid 直接赋值 vs 函数模板取地址
// ============================================================
void demo_assignability() {
    std::cout << "--- demo_assignability ---\n";

    // TODO [必做] 1: 把 std::ranges::sort 赋值给 auto 变量（niebloid = 对象，合法）
    //   niebloid 类型是实现定义的，用 auto 接收即可
    auto sort_fn = std::ranges::sort;  // TODO: 填写 = 右侧

    std::vector<int> v = {3, 1, 4, 1, 5};
    sort_fn(v);
    check(v == std::vector<int>({1, 1, 3, 4, 5}), "ranges::sort object sorts ascending");

    std::cout << "sorted: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';  // 1 1 3 4 5

    // 对比：函数模板无法直接赋值（取消注释会编译错误）：
    // auto sort_bad = std::sort;  // error: cannot deduce template args
    //
    // 必须显式实例化才能取函数指针（丢失泛型性）：
    // using Iter = std::vector<int>::iterator;
    // auto sort_ptr = static_cast<void(*)(Iter, Iter)>(std::sort);
}


// ============================================================
// 基础任务 2：template<auto Algo> 非类型模板参数传递 niebloid
// ============================================================

// TODO [必做] 2: 实现 apply_algo 包装器
//   签名：template<auto Algo, class R, class... Args>
//   将 r 和 args... 转发给 Algo
//   展示 niebloid 作为非类型模板参数的用法
template<auto Algo, class R, class... Args>
void apply_algo(R&& r, Args&&... args) {
    // TODO: 调用 Algo，完美转发 r 和 args...
    Algo(std::forward<R>(r), std::forward<Args>(args)...);
}

void demo_template_param() {
    std::cout << "--- demo_template_param ---\n";

    std::vector<int> v = {5, 3, 8, 1, 9};

    // TODO [必做] 3: 用 apply_algo<std::ranges::sort> 对 v 升序排序
    apply_algo<std::ranges::sort>(v);
    check(v == std::vector<int>({1, 3, 5, 8, 9}), "template<auto> wrapper calls ranges::sort ascending");
    std::cout << "apply_algo sorted: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';  // 1 3 5 8 9

    // TODO [必做] 4: 用 apply_algo<std::ranges::sort> + std::greater<>{} 降序排序
    apply_algo<std::ranges::sort>(v, std::greater<>{});
    check(v == std::vector<int>({9, 8, 5, 3, 1}), "template<auto> wrapper forwards comparator");
    std::cout << "apply_algo sorted desc: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';  // 9 8 5 3 1
}


// ============================================================
// 基础任务 3：projection 对 vector<Person> 按年龄排序
// ============================================================
struct Person {
    std::string name;
    int age;
};

void demo_projection_sort() {
    std::cout << "--- demo_projection_sort ---\n";

    std::vector<Person> people = {
        {"Bob",   30},
        {"Alice", 25},
        {"Carol", 35},
        {"Dave",  28},
    };

    // TODO [必做] 5: 用 ranges::sort + projection 按 age 升序排序
    //   不使用任何手写 lambda；projection 参数传 &Person::age
    //   形式：auto sort_by_age = std::ranges::sort;
    //         sort_by_age(people, std::less{}, &Person::age);
    auto sort_by_age = std::ranges::sort;
    sort_by_age(people, std::less{}, &Person::age);  // TODO: 填入 projection
    check(std::ranges::is_sorted(people, {}, &Person::age), "projection sort orders people by age");
    check(people.front().name == "Alice" && people.back().name == "Carol", "projection sort keeps expected endpoints");

    std::cout << "sorted by age: ";
    for (const auto& p : people)
        std::cout << p.name << '(' << p.age << ") ";
    std::cout << '\n';  // Alice(25) Dave(28) Bob(30) Carol(35)
}


// ============================================================
// 进阶任务：泛型包装器对比代码量（niebloid vs lambda 包装 std::sort）
// ============================================================

// TODO [进阶] 1: 版本 A —— 直接用 niebloid（template<auto Algo>）
//   sorted_print<std::ranges::sort>(v)：一行调用，projection 可扩展
template<auto Algo, class R, class Comp = std::ranges::less, class Proj = std::identity>
void sorted_print(R r, Comp comp = {}, Proj proj = {}) {
    // TODO: 用 Algo 对 r 排序（传入 comp 和 proj），然后打印
    Algo(r, comp, proj);
    check(std::ranges::is_sorted(r, comp, proj), "niebloid generic wrapper sorts its copy");
    for (const auto& x : r)
        std::cout << x << ' ';
    std::cout << '\n';
}

// TODO [进阶] 2: 版本 B —— lambda 包装 std::sort（必须手工 wrap）
template<class SortFn, class R, class Comp = std::less<>>
void sorted_print_legacy(SortFn sort_fn, R r, Comp comp = {}) {
    // TODO: 调用 sort_fn(r.begin(), r.end(), comp)，然后打印
    sort_fn(r.begin(), r.end(), comp);
    check(std::is_sorted(r.begin(), r.end(), comp), "legacy wrapper calls concrete std::sort adapter");
    for (const auto& x : r)
        std::cout << x << ' ';
    std::cout << '\n';
}

void demo_generic_wrapper() {
    std::cout << "--- demo_generic_wrapper ---\n";

    std::vector<int> v = {5, 2, 8, 1, 9, 3};

    // 版本 A：niebloid 直接传，简洁
    std::cout << "niebloid path: ";
    sorted_print<std::ranges::sort>(v);  // 1 2 3 5 8 9（注意：会修改 v 的副本）

    // 版本 B：必须手工包装 std::sort
    auto wrapped_sort = [](auto first, auto last, auto comp) {
        std::sort(first, last, comp);
    };
    std::cout << "lambda-wrap path: ";
    sorted_print_legacy(wrapped_sort, v);
}


// ============================================================
// TODO [进阶] 3: niebloid 内部实现示意（只读，不需要填写）
//
// namespace std::ranges {
//   namespace _sort_impl {
//     struct _sort_fn {
//       // 迭代器重载
//       template<std::random_access_iterator I, std::sentinel_for<I> S,
//                class Comp = ranges::less, class Proj = std::identity>
//       requires std::sortable<I, Comp, Proj>
//       constexpr I operator()(I first, S last, Comp comp={}, Proj proj={}) const;
//
//       // range 重载
//       template<ranges::random_access_range R,
//                class Comp = ranges::less, class Proj = std::identity>
//       requires std::sortable<ranges::iterator_t<R>, Comp, Proj>
//       constexpr ranges::borrowed_iterator_t<R>
//       operator()(R&& r, Comp comp={}, Proj proj={}) const;
//     };
//   }
//   inline constexpr _sort_impl::_sort_fn sort{};
//   //                                    ^^^^ 变量，不是函数
//   //   using std::ranges::sort; sort(r); → 名字查找到变量，不发起函数 ADL
// }
// ============================================================


// ============================================================
// static_assert 验证区
// ============================================================

// std::ranges::sort 可赋值给 auto（niebloid 是对象）
static_assert(std::is_object_v<decltype(std::ranges::sort)>);
// std::ranges::sort 满足 std::is_const_v（inline constexpr）
static_assert(std::is_const_v<std::remove_reference_t<decltype(std::ranges::sort)>>);

// 类型 Person 配合 projection 可以被 ranges::sort 排序
static_assert(std::sortable<
    std::vector<Person>::iterator,
    std::less<>,
    decltype(&Person::age)>);


int main() {
    demo_assignability();
    demo_template_param();
    demo_projection_sort();
    demo_generic_wrapper();
    return 0;
}
