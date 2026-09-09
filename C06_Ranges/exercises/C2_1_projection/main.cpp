// 模块 C2 · 练习 C2-1：projection 模式
// 提案：P0896R4（C++20 ranges 算法投影参数）
// 标准：C++26
//
// 预期输出（空白 main，仅 static_assert）：
//   （无输出，静默通过）
//
// 完成必做 TODO 后预期输出：
//   sorted by age: Alice(25) Dave(25) Bob(30) Carol(35)
//   found: Alice, age=25
//   oldest: Carol(35)
//   youngest: Alice(25)  oldest: Carol(35)
//   age >= 30 count: 2

#include <ranges>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <functional>

struct Person {
    std::string name;
    int age;
};

int main()
{
    std::vector<Person> people = {
        {"Bob",   30},
        {"Alice", 25},
        {"Carol", 35},
        {"Dave",  25},
    };

    // TODO [必做] 1: 成员指针 projection — sort
    //   ranges::sort(people, std::less{}, &Person::age)
    //   打印排序结果，确认 Alice(25) Dave(25) Bob(30) Carol(35)。
    //   在注释里写出等价的 C++17 lambda 版本，体会意图表达力差异。

    // TODO [必做] 2: 成员指针 projection — find
    //   ranges::find(people, std::string{"Alice"}, &Person::name)
    //   找到后打印 "found: Alice, age=25"。

    auto ret = std::ranges::find(people, std::string{ "Alice" }, &Person::name);

    // TODO [必做] 3: 成员指针 projection — max
    //   ranges::max(people, std::less{}, &Person::age)
    //   打印最年长者姓名与年龄。

    // TODO [必做] 4: minmax_result 结构化绑定
    //   auto [youngest, oldest] = ranges::minmax(people, std::less{}, &Person::age)
    //   分别打印最年轻与最年长者信息。

    // TODO [必做] 5: count_if + projection
    //   ranges::count_if(people, [](int a){ return a >= 30; }, &Person::age)
    //   打印计数，确认为 2（Bob 30, Carol 35）。

    // TODO [必做] 6: lambda 作为 projection
    //   用 lambda [](const Person& p){ return p.name[0]; } 作为 projection，
    //   用 ranges::find_if 找到首字母为 'C' 的人。

    // TODO [进阶] 1: projection 可组合性
    //   自定义 Department 结构体，用 lambda 计算人均预算作为 projection，
    //   用 ranges::sort 按人均预算降序排列。

    // TODO [进阶] 2: 用 static_assert 验证 minmax_result 有 min / max 成员：
    //   static_assert(requires{ ranges::minmax_result<Person>{}.min; });

    return 0;
}

// ── static_assert 验证区 ──────────────────────────────────────────────────
// 完成必做 4 后，把下列 assert 从注释中解开并确认编译通过。

// static_assert(requires {
//     std::ranges::min_max_result<int>{}.min;
//     std::ranges::min_max_result<int>{}.max;
// });
