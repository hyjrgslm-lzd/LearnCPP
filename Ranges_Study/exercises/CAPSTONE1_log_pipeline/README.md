# 结课项目 1：CSV 日志多层管道

> 对应章节：../../07-结课项目1-数据管道与源码阅读.md §结课项目 1
> C++ 基线：C++20 / C++23（chunk_by / zip / generator 为 C++23 可选）

## 项目目标

把模块 A-D 的主要能力串起来，做一个完整但不复杂的 ranges 小系统。重点不是业务真实度，而是同时管理：

- view 依赖图（多层嵌套 view 的对象关系）
- iterator_concept 分布（每层 view 的迭代器概念降级链）
- borrowed_range 判定（哪些层是借用，哪些层持有生命周期）
- projection 投影（用成员指针替换 lambda 的场合）
- 结构适配（join / chunk_by 改变 range 形状）
- `ranges::to` 收束（把惰性管道实体化为具体容器）
- 协程源（可选：用 `std::generator` 写惰性行产生器）

## 必做任务

1. **先画 view 依赖图，再写代码**。至少标出 4 层的 iterator_concept 和是否 borrowed。图存档方式：代码中的注释块。至少包含：
   - 底层 raw_lines（ref_view 或 span）
   - 经过 transform(parse_line) 后
   - 经过 filter(has_value) 后（降为 bidirectional，非 borrowed）
   - 经过 transform(unwrap) 后
   - ranges::to 之后（已脱离 view，成为具体容器）

2. **解析阶段**：用 `views::transform` 把每行文本解析为 `optional<LogRecord>`，再用 `views::filter` 过滤掉 `nullopt`，再用 `views::transform` 解包。非法行（字段不足、level 非法）产出 `nullopt`，全部被过滤出管道。

3. **按 level 分组计数**：用 `ranges::count_if` 分别统计 INFO / WARN / ERROR 条数。要求使用 `projection` 而非在 lambda 里访问成员：
   ```cpp
   auto err_count = std::ranges::count_if(
       records, [](Level l){ return l == Level::ERROR; }, &LogRecord::level);
   ```

4. **按 user_id 分组计数**：把每条记录的 user_id 投影出来，用 `ranges::to` 或手动 fold 建立频次表。至少用一次 `ranges::to` 收束为真实容器。

5. **按时间戳排序**：用 `ranges::sort` + projection 对收束后的 `vector<LogRecord>` 排序：
   ```cpp
   std::ranges::sort(records, std::less{}, &LogRecord::timestamp);
   ```

6. **最终用 ranges::to 收束**：把前 3 条 ERROR 记录收束为 `std::vector<LogRecord>`，把用户频次表收束为 `std::map<std::string, int>`。

7. **输出报告**：打印总条数、按 level 计数、按 user_id 计数、前 3 条 ERROR 的完整内容。

8. **附对象关系图说明**：在代码末尾的注释块里，写出每层 view 的 iterator_concept 和是否 borrowed。至少覆盖：底层 raw_lines、经过 filter 之后、经过 transform 之后、ranges::to 之后。

## 进阶任务

1. **把输入改成 std::generator**（C++23）：实现 `std::generator<std::string_view> line_producer(std::span<const std::string_view> src)`，观察 generator 的 iterator_concept（input_iterator，move-only），以及它与 `istream_view` 的类比关系。

2. **把按 user_id 分组改成 chunk_by**（C++23）：先按 user_id 排序，再用 `views::chunk_by` 分组，对比与手动 `map` 的语义差异。注意 `chunk_by` 需要相邻比较，必须先排序。

3. **加一层 zip 把记录和行号配对**（C++23）：
   ```cpp
   auto numbered = std::views::zip(std::views::iota(1), parsed_records);
   ```
   验证 `zip_view` 的 iterator_concept（random_access）vs iterator_category（input，proxy reference 导致双轨差异）。

4. **写一份 1 页小结**（注释形式）：
   - filter_view 的 begin() 缓存在哪里最可能导致意外？
   - 哪一处最体现惰性的价值（省去了哪些中间容器）？
   - 如果把输入换成 std::generator，哪些地方的 API 调用需要改变？

## 固定交付物

- `main.cpp`：完整管道代码，含 view 依赖图注释
- `README.md`（本文件）：任务说明与验收标准
- view 依赖图：以代码注释块形式存档在 main.cpp 末尾
- 1 页复盘小结（进阶任务 4，可作为注释块）

## 验收点

- **iterator_concept 链**：能指出管道每层的 iterator_concept，并说出降级发生在哪里（通常是第一个 `filter`）。
- **真正触发迭代的位置**：能指出哪行代码是消费端（for-range / ranges::to / ranges::sort / ranges::count_if），以及在此之前所有 view 均未迭代。
- **borrowed 判定链**：能指出管道里哪些层是 borrowed_range，哪些不是，以及对右值输入调用返回迭代器算法时为何得到 `ranges::dangling`。
- **const 存储限制**：能说明为什么含 `filter_view` 的管道不能 const 存储，以及 `views::as_const` 不解决这个问题。
- **无共享可变状态**：没有使用共享的可变大对象让多段 view 同时写入；所有收束操作均在单一消费端完成。

## 观察点

- 解析管道中，三层 view（transform / filter / transform）在 ranges::to 之前均未迭代，只有 ranges::to 触发了真正的遍历——体现惰性的价值：不需要为中间 optional 结果分配容器。
- `filter_view` 的 begin() 非 const：如果把解析管道 const 存储（`const auto parsed = ...`），后续对 parsed 调用 begin() 会编译失败——这是最容易触发的意外之一。
- projection 的价值：`ranges::count_if(records, pred, &LogRecord::level)` 比 `ranges::count_if(records, [](const LogRecord& r){ return pred(r.level); })` 更简洁，且意图更清晰。

## 常见坑

- **把 filter_view const 存储**：导致后续无法调用 begin()，编译失败。解决方案：不要 const 存储含 filter_view 的管道，或者先 ranges::to 收束再 const。
- **对 view 调用两次 begin() 期望结果一致**：filter_view 的 begin() 第一次调用会扫描找到第一个满足条件的位置并缓存，第二次调用返回缓存结果；但如果底层 range 已经被修改，缓存会失效——本项目里底层是 const vector，不会出现这个问题，但要知道这个约束。
- **忘记排序就用 chunk_by 分组**：chunk_by 按相邻元素条件分块，如果 user_id 不排序，同一个 user_id 的记录可能分散在不同块中。
- **对 ranges::to 结果继续用管道语法**：ranges::to 收束后得到的是具体容器（vector/map），不是 view；如果想继续用管道，需要再套一个 views::all 或直接用容器的 begin/end。

## 复盘问题

1. view 依赖图里，哪几层嵌套关系最能说明"管道是类型嵌套而非函数调用链"？
2. 把 `projection` 换成等价的 lambda 表达式，代码量如何变化？表达意图是否更清晰还是更混乱？
3. `borrowed_range` 判定链在本项目里最关键的节点是哪个？如果底层输入是右值临时 `vector`，哪行代码会在编译期触发 `ranges::dangling`？
4. 管道 vs 显式循环：这个项目里哪一处用管道明显优于显式循环，哪一处用显式循环可能更易读？
5. C++23 的 `views::zip` / `views::chunk_by` 在本项目里提供的增益是语义层面的（表达意图更清晰），还是性能层面的（减少迭代次数），还是两者都有？

## 对应官方参考

- P0896R4：C++20 ranges 基础（filter_view / transform_view / take_view）
- P1206R7：C++23 `ranges::to`（惰性管道收束为容器）
- P2321R2：C++23 `views::zip`（proxy reference 双轨）
- P2442R1：C++23 `views::chunk_by`
- P2502R2：C++23 `std::generator`（协程源，可选进阶）
- P2278R4：C++23 `views::as_const`（与 filter_view begin() 缓存问题的正交关系）
- cppreference：[`std::ranges::to`](https://en.cppreference.com/w/cpp/ranges/to)
- cppreference：[`std::ranges::filter_view`](https://en.cppreference.com/w/cpp/ranges/filter_view)
