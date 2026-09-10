// 章节：08-模块E-CPO与niebloid.md
// 小节：练习 E-3：ranges CPO 与 stdexec tag_invoke 对照
// 提案：P1895R0（tag_invoke 单一 ADL 入口设计），P0896R4（ranges CPO）
// C++ 标准：C++20/26
// 注意：tag_invoke 不是 C++ 标准，来自 stdexec/P2300 提案；
//       本练习给出 ranges 侧完整实现 + tag_invoke 侧最小桩（注释说明）
//
// 预期输出：
//   ranges::begin   -> 10
//   ranges::size    -> 4
//   range-for loop  -> 10 20 30 40
//   tag_invoke connect -> 10
//   both customizations work

#include <check.hpp>

#include <ranges>
#include <iterator>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <vector>


// ============================================================
// 查找流程对照（注释形式）
//
// ranges CPO（以 ranges::begin 为例）：
//   调用: ranges::begin(r)
//     → CPO operator() 内部
//     → 阶段 1: r.begin() 合法且返回 iterator? 是 → 调用
//     → 阶段 2: ADL begin(r) 合法且返回 iterator? 是 → 调用
//     → 阶段 3: SFINAE 失败，编译错误
//
// tag_invoke（以 stdexec::connect 为例）：
//   调用: connect(sender, receiver)
//     → CPO operator() 内部
//     → tag_invoke(connect_t{}, sender, receiver)
//     → 单一 ADL 入口（只有 tag_invoke 这一个名字）
//     → 查找 sender/receiver 所在命名空间的 tag_invoke 重载
//     → friend tag_invoke(connect_t, MySender, Receiver)
// ============================================================


// ============================================================
// 模拟 tag_invoke 基础设施（最小桩，stdlib only）
//
// 真实的 stdexec tag_invoke 基础设施位于 <stdexec/execution.hpp>。
// 本练习不依赖 stdexec，仅在注释和最小桩里演示接口形态，
// 帮助理解与 ranges CPO 的设计对比。
// ============================================================

namespace my_exec {

    // tag 类型：每个定制点对应一个 tag struct
    // （类比 ranges 里每个 CPO 是一个独立的函数对象类型）
    struct connect_t {
        // TODO [必做] 1: 在 operator() 里调用 tag_invoke(connect_t{}, ...)
        //   实际 stdexec 的 connect CPO 形如：
        //   template<class Sender, class Receiver>
        //   auto operator()(Sender&& s, Receiver&& r) const {
        //       // 通过 tag_invoke 分发给用户的 friend 实现
        //       return tag_invoke(*this, std::forward<Sender>(s),
        //                        std::forward<Receiver>(r));
        //   }
        //
        // 最小桩实现（演示接口，不做完整类型约束）：
        template<class Sender>
        auto operator()(Sender& s) const {
            // ADL 查找 tag_invoke(connect_t, Sender&)
            return tag_invoke(*this, s);
        }
    };

    inline constexpr connect_t connect{};
    // ^ 同样是 inline constexpr 变量——tag_invoke 侧的 CPO 本体

} // namespace my_exec


// ============================================================
// TODO [必做] 2: MyContainer —— 同时接入 ranges CPO 和 tag_invoke
//
// ranges 侧：提供 member begin() / end() / size()
// tag_invoke 侧：提供 friend tag_invoke(my_exec::connect_t, MyContainer&)
// ============================================================

class MyContainer {
    int data_[4] = {10, 20, 30, 40};

public:
    // ---- ranges CPO 接入（三阶查找的第一阶：成员函数）----
    // TODO [必做] 2a: 提供 begin() 返回 int*（满足 input_or_output_iterator）
    int* begin() { return data_; }

    // TODO [必做] 2b: 提供 end() 返回 int*
    int* end()   { return data_ + 4; }

    // TODO [必做] 2c: 提供 size() 返回 std::size_t（满足 ranges::sized_range）
    std::size_t size() const { return 4; }

    // ---- tag_invoke 接入（单一 ADL 名字 + 标签分发）----
    // TODO [必做] 3: 提供 friend tag_invoke(my_exec::connect_t, MyContainer&)
    //   语义（演示用）：返回容器第一个元素
    //   friend int tag_invoke(my_exec::connect_t, MyContainer& c) { return c.data_[0]; }
    friend int tag_invoke(my_exec::connect_t, MyContainer& c) {
        // TODO: return c.data_[0];
        return c.data_[0];
    }
};



namespace adl_demo {
class FreeBeginRange {
    int data_[2] = {7, 8};
public:
    friend int* begin(FreeBeginRange& r) { return r.data_; }
    friend int* end(FreeBeginRange& r) { return r.data_ + 2; }
};
} // namespace adl_demo
// ============================================================
// 四维度对照（注释形式）
//
// 维度 1 —— 如何新增定制点：
//   ranges CPO：给类型加成员 begin() 或 ADL begin(r)，各 CPO 独立检测
//   tag_invoke ：给类型加 friend tag_invoke(tag_t, ...)，所有定制点共用一个 ADL 名字
//
// 维度 2 —— 如何屏蔽 ADL 污染：
//   ranges CPO：CPO 是变量，名字查找到变量后不发起函数 ADL；
//               但 CPO 内部的 fallback ADL 仍然存在（受控的 ADL）
//   tag_invoke ：只有一个 ADL 名字（tag_invoke），比 N 个独立名字冲突风险更低
//
// 维度 3 —— 错误消息定位：
//   ranges CPO：错误在 SFINAE 失败处，不同 CPO 格式各异
//   tag_invoke ：错误统一为 "no matching function for tag_invoke(tag_t, ...)"，格式一致
//
// 维度 4 —— 泛型扩展性：
//   ranges CPO：适合接入标准已有 CPO；新增 CPO 需要完整实现三阶查找逻辑
//   tag_invoke ：新增定制点只需新建一个 tag struct；用户接入方式完全统一
//                这是 stdexec 有几十个定制点还能保持接口一致性的原因
// ============================================================


// ============================================================
// TODO [进阶] 1: 给 MyContainer 增加更多 ranges 定制
//   - rbegin() / rend()：满足 ranges::bidirectional_range
//   - data()：满足 ranges::contiguous_range
//   - 每步用 static_assert 验证对应 range concept
//
// TODO [进阶] 2: 扩展 tag_invoke 侧
//   增加第二个 tag：
//     struct get_size_t {};
//     inline constexpr get_size_t get_size{};
//   在 MyContainer 里加 friend tag_invoke(get_size_t, const MyContainer&)
//   演示"新增定制点只需新建 tag"的扩展优势
//
// TODO [进阶] 3: 阅读说明
//   ranges CPO 于 C++20 合入（P0896R4）。
//   tag_invoke 来自 P1895R0（2019），晚于 C++20 feature freeze，
//   故 ranges 未采用 tag_invoke 集中入口——时间线因素，不是技术限制。
//   range-v3 的 include/range/v3/range/access.hpp 有更集中化的 _cpo_t 写法，
//   思路已接近 tag_invoke。
// ============================================================


// ============================================================
// static_assert 验证区
// ============================================================

// MyContainer 满足 ranges::range（有 begin/end）
static_assert(std::ranges::range<MyContainer>);

// MyContainer 满足 ranges::sized_range（有 size()）
static_assert(std::ranges::sized_range<MyContainer>);

// MyContainer 的迭代器满足 std::contiguous_iterator（int* 是 contiguous）
static_assert(std::contiguous_iterator<int*>);

// my_exec::connect CPO 是对象（变量），不是函数
static_assert(std::is_object_v<decltype(my_exec::connect)>);


// ============================================================
// 演示：两套定制路径都能工作
// ============================================================
void demo_dual_customization() {
    MyContainer mc;

    // 通过 ranges CPO 访问（走成员 begin 路径）：
    auto it = std::ranges::begin(mc);
    check(*it == 10, "ranges::begin uses the member begin path");
    std::cout << "ranges::begin   -> " << *it << '\n';  // 10

    std::size_t sz = std::ranges::size(mc);
    check(sz == 4, "ranges::size calls the member size path");
    std::cout << "ranges::size    -> " << sz << '\n';   // 4

    std::vector<int> seen;
    std::cout << "range-for loop  -> ";
    for (int x : mc) {
        seen.push_back(x);
        std::cout << x << ' ';
    }
    check(seen == std::vector<int>({10, 20, 30, 40}), "range-for consumes the member begin/end pair");
    std::cout << '\n';  // 10 20 30 40

    // 通过 tag_invoke 访问（走 friend tag_invoke 路径）：
    int first = my_exec::connect(mc);
    check(first == 10, "tag_invoke connect dispatch returns the first element");
    std::cout << "tag_invoke connect -> " << first << '\n';  // 10

    adl_demo::FreeBeginRange free_range;
    check(*std::ranges::begin(free_range) == 7, "ranges::begin uses the ADL fallback path when no member exists");

    std::cout << "both customizations work\n";
}


int main() {
    demo_dual_customization();
    return 0;
}
