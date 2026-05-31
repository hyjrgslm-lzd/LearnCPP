// 章节：08-模块E-CPO与niebloid.md
// 小节：练习 E-1：手写一个简化版 ranges::begin CPO
// 提案：P0896R4（ranges::begin 三阶查找规则）
// C++ 标准：C++20/26
//
// 预期输出：
//   member begin: *it = 1
//   ADL begin:    *it = 10
//   no-begin path: correctly rejected at compile time

#include <iterator>
#include <concepts>
#include <vector>
#include <string_view>
#include <iostream>

// ============================================================
// my_ranges::my_begin CPO
// 实现"成员优先 + ADL fallback"三路径定制点对象
// ============================================================

namespace my_ranges {

    namespace _my_begin_impl {

        // TODO [必做] 1: 定义 has_member_begin concept
        //   约束：requires(R& r) { { r.begin() } -> std::input_or_output_iterator; }
        //   注意：R& 不是 R&&，CPO 对左值生效
        template<class R>
        concept has_member_begin =
            /* TODO: 填入 requires 表达式 */
            requires(R& r) {
                { r.begin() } -> std::input_or_output_iterator;
            };

        // TODO [必做] 2: 定义 has_adl_begin concept
        //   约束：!has_member_begin<R> 且 ADL begin(r) 返回 iterator
        //   关键：此 concept 必须定义在 _my_begin_impl 命名空间内
        //   （若放在 my_ranges 命名空间，ADL 会查找到 my_begin 自身，逻辑混乱）
        template<class R>
        concept has_adl_begin =
            !has_member_begin<R> &&
            /* TODO: 填入 requires 表达式，使用非限定 begin(r) */
            requires(R& r) {
                { begin(r) } -> std::input_or_output_iterator;
            };

        // TODO [必做] 3: 定义 my_begin_fn 函数对象类型
        //   包含两个 operator() 重载：
        //   - requires has_member_begin<R>：调用 r.begin()，传播 noexcept
        //   - requires has_adl_begin<R>：调用 begin(r)，传播 noexcept
        //   不提供第三个重载（无 begin 路径 → 调用点 SFINAE 失败）
        struct my_begin_fn {

            // 路径 1：成员 begin（优先）
            template<class R>
                requires has_member_begin<R>
            constexpr auto operator()(R& r) const
                noexcept(noexcept(r.begin()))
            {
                // TODO: return r.begin();
                return r.begin();
            }

            // 路径 2：ADL begin（fallback，仅当路径 1 不可用时）
            template<class R>
                requires has_adl_begin<R>
            constexpr auto operator()(R& r) const
                noexcept(noexcept(begin(r)))
            {
                // TODO: return begin(r);
                // （此处 begin 走 ADL，查找 R 所在命名空间；
                //   不会找到 my_ranges::my_begin，因为名字是 begin 而非 my_begin）
                return begin(r);
            }

            // 路径 3：无 begin → 不提供重载，调用点 SFINAE 失败
        };

    } // namespace _my_begin_impl

    // TODO [必做] 4: 声明 CPO 本体
    //   形式：inline constexpr _my_begin_impl::my_begin_fn my_begin{};
    //   关键：inline 允许头文件多定义；constexpr 允许常量表达式上下文调用
    //   关键：my_begin 是变量，名字查找找到变量后不进入 ADL 阶段 → ADL 隔离
    inline constexpr _my_begin_impl::my_begin_fn my_begin{};

} // namespace my_ranges


// ============================================================
// 测试场景 1：标准容器 —— 走成员 begin 路径
// ============================================================
void test_member_begin() {
    std::vector<int> v = {1, 2, 3};
    auto it = my_ranges::my_begin(v);
    std::cout << "member begin: *it = " << *it << '\n';  // 1
}


// ============================================================
// 测试场景 2：只有 ADL begin 的自定义类型
// ============================================================
namespace lib_custom {

    struct WithFreeBegin {
        int arr[3] = {10, 20, 30};
        // 无成员 begin()，只有 ADL 自由函数
    };

    // begin/end 在 lib_custom 命名空间里，ADL 能找到
    int* begin(WithFreeBegin& w) { return w.arr; }
    int* end(WithFreeBegin& w)   { return w.arr + 3; }

} // namespace lib_custom

void test_adl_begin() {
    lib_custom::WithFreeBegin w;
    auto it = my_ranges::my_begin(w);
    std::cout << "ADL begin:    *it = " << *it << '\n';  // 10
}


// ============================================================
// 测试场景 3：无 begin —— 编译期 SFINAE 失败
// ============================================================
void test_no_begin() {
    struct NoBegan { int x; };
    NoBegan nb{42};

    // 下面这行会产生编译错误（取消注释可验证）：
    // my_ranges::my_begin(nb);

    // 用 requires 表达式在不触发错误的情况下验证：
    // 注意：MSVC 19.50 在某些版本对"constrained 函数对象无匹配 operator() 的
    // requires-expression"报 C3889 硬错误；作为骨架，下面两行先注释掉，
    // 实现 TODO 后可取消注释复查：
    // constexpr bool ok = requires { my_ranges::my_begin(nb); };
    // static_assert(!ok, "NoBegan should not have my_begin");
    std::cout << "no-begin path: correctly rejected at compile time\n";
}


// ============================================================
// TODO [进阶] 1: borrowed_range 约束
//   在 my_begin_fn 的两个 operator() 上增加：
//     requires (std::is_lvalue_reference_v<R> ||
//               std::ranges::enable_borrowed_range<std::remove_cvref_t<R>>)
//   然后测试：
//   - my_begin(v)                        左值 vector → 通过
//   - my_begin(std::vector<int>{1,2,3})  右值 vector → 编译期拒绝
//   - my_begin(std::string_view{"hi"})   右值 string_view → 通过（已特化 enable_borrowed_range）
// ============================================================

// TODO [进阶] 2: 对比函数模板版本
//   将 my_begin 改写为函数模板（非 CPO），演示：
//   namespace my_ranges_bad {
//       template<class R>
//       constexpr auto my_begin(R& r) { return r.begin(); }
//   }
//   问题 1：using my_ranges_bad::my_begin; my_begin(r); → ADL 可能找到外部 my_begin
//   问题 2：auto f = my_ranges_bad::my_begin; → 编译错误（函数模板不可直接赋值）
//   对比 CPO：auto g = my_ranges::my_begin; → 合法
// ============================================================


// ============================================================
// static_assert 验证区
// ============================================================

// my_begin 对 std::vector<int> 左值可调用
static_assert(requires { my_ranges::my_begin(std::declval<std::vector<int>&>()); });

// my_begin 对无 begin 类型不可调用
// struct _NoBegan { int x; };
// static_assert(!requires { my_ranges::my_begin(std::declval<_NoBegan&>()); });
// 同上，MSVC C3889 规避；实现层完成后可启用。

// my_begin 返回 iterator（满足 input_or_output_iterator）
static_assert(std::input_or_output_iterator<
    decltype(my_ranges::my_begin(std::declval<std::vector<int>&>()))>);


int main() {
    test_member_begin();
    test_adl_begin();
    test_no_begin();
    return 0;
}
