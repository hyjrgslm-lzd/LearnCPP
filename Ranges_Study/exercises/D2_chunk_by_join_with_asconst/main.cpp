// 模块 D — 练习 D-2：chunk_by / join_with / as_const / as_rvalue
// 章节：06-模块D-C++23高阶视图与协程桥.md §练习 D-2
// 提案：P2442R1 (chunk_by), P2441R2 (join_with), P2278R4 (as_const), P2446R2 (as_rvalue)
// C++ 标准：C++23
//
// 预期输出（填完 TODO 后）：
//   chunk_by(a<=b): [123] [1245]
//   chunk_by(a==b) runs: [11] [222] [3] [11]
//   join_with('-'): hello-world-cpp23
//   join:           helloworldcpp23
//   join_with(", "): hello, world, cpp23
//   as_const: 1 2 3 4 5
//   editable after += readonly: 11 22 33
//   strs after move via ranges::to: [] [] []  （或有效但未指定状态）
//
// 关键概念：
//   - chunk_by(pred)：pred(a,b)=false 处切断，b 开始新块
//   - join_with(delim)：相邻内层之间插入分隔符，第一个之前不插入
//   - as_const (P2278R4)：basic_const_iterator 包装，与 filter_view begin() 缓存正交
//   - as_rvalue (P2446R2)：产出右值引用，move 发生在消费侧（赋值/构造才 move）

#include <ranges>
#include <vector>
#include <string>
#include <iostream>
#include <type_traits>

int main() {
    // ============================================================
    // TODO [必做] 1: 观察 views::chunk_by 的相邻条件分块
    //   v = {1,2,3,1,2,4,5}
    //   chunk_by([](int a, int b){ return a <= b; })
    //   pred 为 false 时（3>1）切断，产出 [1,2,3] 和 [1,2,4,5]
    //   手动画出期望块边界再运行对照
    // ============================================================

    // TODO [必做] 2: chunk_by 用于 RLE（游程分组）
    //   w = {1,1,2,2,2,3,1,1}
    //   chunk_by([](a,b){ return a==b; })
    //   产出 [11] [222] [3] [11]
    // ============================================================

    // TODO [必做] 3: 对比 chunk(3) 与 chunk_by
    //   同一个 v，chunk(3) 按固定大小切：[123] [124] [5]
    //   chunk_by 按内容条件切：块大小不固定
    // ============================================================

    // TODO [必做] 4: 观察 views::join_with('-')
    //   words = {"hello", "world", "cpp23"}
    //   join_with('-') 产出 hello-world-cpp23
    //   对比 views::join 产出 helloworldcpp23（无分隔符）
    // ============================================================

    // TODO [必做] 5: join_with(string 分隔符)
    //   join_with(std::string{", "}) 产出 hello, world, cpp23
    //   验证：第一个内层之前不插入分隔符（语义与 Python str.join() 一致）
    // ============================================================

    // TODO [必做] 6: 验证 views::as_const 的 const 保护
    //   v = {1,2,3,4,5}
    //   auto cv = v | views::as_const
    //   用 static_assert 验证 range_reference_t<decltype(cv)> == const int&
    //   尝试（注释中）通过 cv 修改元素，观察编译错误
    // ============================================================

    // TODO [必做] 7: as_const + zip 防止 zip 下游误写只读端
    //   editable = {1,2,3}, readonly_src = {10,20,30}
    //   zip(editable, readonly_src | as_const)
    //   结构化绑定后 e 是 int&（可写），r 是 const int&（只读）
    //   执行 e += r，验证 editable 变为 {11,22,33}
    // ============================================================

    // TODO [必做] 8: 观察 views::as_rvalue 与 ranges::to 的 move 语义
    //   strs = {"hello", "world", "cpp"}
    //   moved = strs | views::as_rvalue | ranges::to<vector<string>>()
    //   打印 moved（有值）和 strs（元素已被 move，通常为空字符串）
    //   用 static_assert 验证 as_rvalue 产出 string&&
    // ============================================================

    // TODO [进阶] 1: 用 chunk_by + transform 实现 RLE 编码
    //   {a,a,b,b,b,c} → chunk_by(a==b) → transform 每组为 pair(元素, 个数)
    //   输出 (a,2) (b,3) (c,1)
    // ============================================================

    // TODO [进阶] 2: 演示 as_const 与 filter_view const 迭代问题正交
    //   auto fv = v | views::filter([](int x){ return x>2; });
    //   auto cfv = fv | views::as_const;
    //   注释说明：as_const 不修复 filter_view 的 begin() 非 const 限制
    // ============================================================

    // TODO [进阶] 3: 观察 as_rvalue 的消费时机
    //   for (auto s : strs | views::as_rvalue) { ... }  // auto 触发 move
    //   for (const auto& s : strs | views::as_rvalue) { ... }  // const ref 不 move
    //   对比两种写法后 strs 元素的状态
    // ============================================================

    std::cout << "D2 skeleton — fill TODOs to observe chunk_by/join_with/as_const/as_rvalue\n";
    return 0;
}

// ============================================================
// 静态验证区（填完 TODO 后解注释）
// ============================================================
// static_assert(std::same_as<
//     std::ranges::range_reference_t<
//         decltype(std::declval<std::vector<int>&>() | std::views::as_const)>,
//     const int&>);
//
// static_assert(std::same_as<
//     std::ranges::range_reference_t<
//         decltype(std::vector<std::string>{} | std::views::as_rvalue)>,
//     std::string&&>);
