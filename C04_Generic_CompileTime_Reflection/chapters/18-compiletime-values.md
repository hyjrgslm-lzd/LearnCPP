# 18. 值级编译期计算：把一张小表做成类型稳定的查询结构

先修：[常量求值](10-constant-evaluation.md)说明了 `constexpr` 与 `consteval` 什么时候执行；[包与 NTTP](07-packs-nttp.md)说明了值怎样进入模板实参。本章把两者连起来：用一组字符串字面量 key 和 `int` value，在编译期排序、去重，并生成一个大小已经固定的查询表。

运行时写法很直接：

```cpp
struct row {
    std::string_view key;
    int value;
};

std::vector<row> rows = {{"mp", 30}, {"hp", 100}, {"atk", 7}, {"mp", 99}, {"def", 12}};
std::stable_sort(rows.begin(), rows.end(), {}, &row::key);
rows.erase(std::unique(rows.begin(), rows.end(), same_key), rows.end());
```

这段代码能表达算法，但输出大小只在运行时知道。编译期版本要多回答一个问题：结果类型里的 `N` 从哪里来？如果输出是 `std::array<row, N>` 或 `table<N>`，`N` 必须在模板实例化时就是常量表达式。

## 字面量 key 先变成结构化值

字符串字面量不能直接作为 `std::string_view` 存进 NTTP，因为指针值不是我们想比较的 key。练习把 key 固定成 16 字节数组：

```cpp
struct row {
    static constexpr std::size_t key_width = 16;
    std::array<char, key_width> key{};
    int value{};
};

template<std::size_t N>
consteval row make_row(char const (&text)[N], int value) {
    static_assert(N <= row::key_width);
    row out{};
    for (std::size_t i = 0; i + 1 < N; ++i) {
        const auto ch = static_cast<unsigned char>(text[i]);
        if (ch == 0 || ch > 0x7F) {
            throw "literal key must contain only non-NUL ASCII bytes";
        }
        out.key[i] = text[i];
    }
    out.value = value;
    return out;
}
```

`make_row("hp", 100)` 的结果是普通结构化值：key 数组里是 `'h'`, `'p'`, `'\0'`，value 是 `100`。它可以放进 `inline constexpr auto raw = std::array{...};`，再作为 `make_table<raw>()` 的模板实参。这里 key 宽度是教学边界：`key_width == 16` 表示固定存储包含终止 NUL，所以 payload 最长 15 个 ASCII 字符。空 key `""` 允许；payload 内部的 `'\0'` 和非 ASCII byte 拒绝，避免 `"a\0b"` 被比较函数当成 `"a"`。

## 稳定排序先只改变顺序

排序算法不需要运行时容器。固定大小数组已经足够：

```cpp
template<std::size_t N>
consteval auto sort_by_key(std::array<row, N> input) {
    for (std::size_t i = 1; i < N; ++i) {
        row current = input[i];
        std::size_t j = i;
        while (j != 0 && compare_key(current, input[j - 1]) < 0) {
            input[j] = input[j - 1];
            --j;
        }
        input[j] = current;
    }
    return input;
}
```

这里用插入排序，是因为教学目标是常量求值语义，不是追求大输入性能。关键点是比较条件只在 `current < previous` 时移动元素；相等 key 不移动，所以排序稳定。原始表：

```cpp
inline constexpr auto raw_prices = std::array{
    make_row("mp", 30),
    make_row("hp", 100),
    make_row("atk", 7),
    make_row("mp", 99),
    make_row("def", 12),
};
```

排序后顺序是 `atk, def, hp, mp, mp`。两个 `mp` 的相对顺序仍是 `30` 在 `99` 前面。稳定域只覆盖相等 key；不同 key 的相对顺序由字典序决定。

## 去重先数数量，再构造结果形状

去重的业务契约是“重复 key 保留第一项”。如果先排序再 `unique`，稳定排序也能保留第一项；练习用更直接的写法：按原始顺序扫描，见过的 key 跳过，未见过的写入结果。问题在于结果数组大小不是输入大小，而是唯一 key 数量。

连续推导如下：

```cpp
template<auto Raw>
consteval std::size_t unique_count() {
    std::size_t count = 0;
    for (std::size_t i = 0; i < Raw.size(); ++i) {
        bool seen = false;
        for (std::size_t j = 0; j < i; ++j) {
            seen = seen || same_key(Raw[i], Raw[j]);
        }
        if (!seen) ++count;
    }
    return count;
}

template<std::size_t N>
struct table {
    static constexpr std::size_t size = N;
    std::array<row, N> rows{};
};

template<auto Raw>
consteval auto make_table() {
    table<unique_count<Raw>()> out{};
    std::size_t write = 0;
    for (std::size_t i = 0; i < Raw.size(); ++i) {
        bool seen = false;
        for (std::size_t j = 0; j < i; ++j) {
            seen = seen || same_key(Raw[i], Raw[j]);
        }
        if (!seen) out.rows[write++] = Raw[i];
    }
    out.rows = sort_by_key(out.rows);
    return out;
}
```

手推 `raw_prices`：

1. `mp` 没见过，保留 `mp=30`，唯一数量变成 1。
2. `hp` 没见过，保留 `hp=100`，数量 2。
3. `atk` 没见过，保留 `atk=7`，数量 3。
4. 第二个 `mp` 已见过，跳过，`mp=99` 不进入表。
5. `def` 没见过，保留 `def=12`，数量 4。
6. `unique_count<raw_prices>()` 是 4，所以返回类型是 `table<4>`。
7. 输出行再排序，最终顺序是 `atk=7, def=12, hp=100, mp=30`。

空输入也走同一条路径：`unique_count` 是 0，返回 `table<0>`，查找任何 key 都失败。空 key 与空输入不是一回事：`make_row("", 1)` 是一条合法记录，能通过 `find(table, "")` 查到；`table<0>` 里没有任何记录。这个边界很重要，因为它证明算法没有依赖“至少一个元素”的隐藏前提。

## 普通 consteval 参数能算值，不能直接改变返回类型形状

很多人第一次写 `consteval` 会以为函数参数在函数体里都能作为数组长度。反例：

```cpp
consteval auto bad_extent(std::size_t n) {
    return std::array<int, n>{}; // n 是普通参数，不能作为模板实参
}
```

`consteval` 保证调用必须在常量求值中完成，但普通参数 `n` 仍不是模板实参位置可用的常量表达式。下面这种写法合法，因为返回类型不需要根据计算结果改变大小：

```cpp
template<std::size_t N>
consteval auto normalize_same_size(std::array<row, N> input) {
    return sort_by_key(input); // 返回 std::array<row, N>
}
```

如果结果形状要从值推导出来，就要让输入值进入模板参数：

```cpp
template<auto Raw>
consteval auto normalized = make_table<Raw>(); // 类型是 table<unique_count<Raw>()>
```

也可以在编译期先用临时 `std::vector` 做 scratch：读入、排序、去重都在 `consteval` 中完成；但指向 vector 内部元素的指针、迭代器和 `string_view` 不能逃逸，因为 scratch 的存储在常量求值结束时消失。安全做法是把最终内容复制到拥有存储的 `std::array` 或 `table<N>`。练习的负编译用例专门拒绝“返回 scratch 指针”的写法。

## 查找阶段可以是普通 constexpr

生成后的表已经排序，运行时和编译期都可以二分查找：

```cpp
template<std::size_t N>
constexpr std::optional<int> find(table<N> const& input, std::string_view key) {
    std::size_t first = 0;
    std::size_t last = N;
    while (first != last) {
        const std::size_t mid = first + (last - first) / 2;
        const int cmp = compare_key(input.rows[mid], key);
        if (cmp < 0) first = mid + 1;
        else if (cmp > 0) last = mid;
        else return input.rows[mid].value;
    }
    return std::nullopt;
}
```

`find(table, "mp")` 返回 `30`，不是 `99`；`find(table, "agi")` 返回空；`find(table, "")` 可以命中空 key；对 `table<0>` 查找也返回空。运行时查询用 `std::string_view`，比较时仍把内部 NUL 当作真实字符处理，`std::string_view{"", 1}` 不等于空 key。这里没有运行时 `unordered_map`，也没有动态初始化顺序问题。表的规范化成本付在编译期，查询结构的形状写在类型里。

[A01练习](../exercises/A01_compiletime_values/README.md)要求 Student 初态能构建但运行失败。Reference 与 good 都实现完整契约；bad 会保留重复 key 的最后值，检查器必须拒绝。运行观察应同时看正例、坏例和负编译：正例证明表形状、排序、去重和查询；坏例证明检查器能抓到业务错误；负编译证明临时 scratch 不能把指针带出常量求值。

自测：

1. `normalize_same_size(raw_prices)` 为什么不能返回 `table<4>`？因为 `raw_prices` 是普通函数参数，计算出的唯一数量不能直接用于返回类型形状。
2. 为什么 `make_table<raw_prices>()` 可以返回 `table<4>`？因为 `raw_prices` 已进入模板实参，`unique_count<raw_prices>()` 在实例化点形成常量。
3. 重复 key 为什么保留第一项？这是表规范化契约；稳定排序只保证相等 key 的相对顺序，不替你决定业务保留哪一个。
4. 为什么 `key_width == 16` 只能接受 15 个字符？数组里还要保存终止 NUL，payload 不能占满全部存储。
5. 为什么不能返回指向编译期 `std::vector` scratch 的指针？scratch 没有可逃逸的运行期存储；最终结果必须复制到拥有存储的对象里。
