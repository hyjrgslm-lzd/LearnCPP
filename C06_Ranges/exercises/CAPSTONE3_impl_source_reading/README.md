# 结课项目 3：实现级源码对照

> 对应文件：`12-结课项目2-实现级源码阅读.md` §"结课项目 3：实现级源码对照"
> 阶段定位：阶段二（实现层）结课项目，综合考察模块 E-H 全部知识。

---

## 项目目标

把模块 E-H 学到的技术（CPO/niebloid / iterator_concept / view_interface /
range_adaptor_closure / `__non_propagating_cache` / common_iterator /
iter_move / basic_const_iterator）带回到 stdlib 源码，从"这段代码在干什么"升级到
"它用了什么模式，为什么这样选"。

与第一阶段结课项目 2 的本质区别：上一次用**对象关系语言**读懂结构；
这一次用**实现模式语言**说出理由。

---

## 必做任务

### 任务 1（`main.cpp` TODO [必做] 1）：重读四份实现，标注关键模式

阅读 `single_view` / `empty_view` / `iota_view` / `transform_view` /
`filter_view` / `join_view` 源码，对每份实现标注以下要点：

- `view_interface<D>` CRTP 继承关系与注入方法
- inner iterator 持有的双成员（`_M_current` + `_M_parent`）
- `__non_propagating_cache` 成员的类型、拷贝行为、触发条件
- xvalue 内层子范围的缓存机制（join_view 特有）

**交付**：在 `notes/01_object_diagram.md` 补全五节点关系图；在 `main.cpp` TODO 1 区取消注释并验证通过。

### 任务 2（`main.cpp` TODO [必做] 2）：inner iterator 模式验证

对 transform_view 验证：contiguous 底层降为 random_access；
`iter_move` hidden-friend 定制路径。

**交付**：`main.cpp` TODO 2 区 static_assert 全部通过；在 `notes/02_pattern_table.md` 填写 transform_view 行。

### 任务 3（`main.cpp` TODO [必做] 3）：对照表四行全部填写

完成 `notes/02_pattern_table.md` 技术模式对照表全部四行，"你之前的误解"列有实质内容。

**交付**：`notes/02_pattern_table.md` 无空格，iterator_concept 列有具体 min 表达式。

### 任务 4（`main.cpp` TODO [必做] 4 + `notes/03_cpo_checklist.md`）：CPO 清单 + 设计哲学对比

完成 `notes/03_cpo_checklist.md` 中 8 项 CPO 的定制意图说明；
补充 ranges CPO vs stdexec tag_invoke 对比表至少 4 个维度。

**交付**：`notes/03_cpo_checklist.md` 八项全部填写；对比表至少 4 行有实质内容。

### 任务 5（`main.cpp` TODO [必做] 5 + `notes/04_reading_note.md`）：读书笔记

以《我现在如何向别人解释 stdlib ranges 的实现架构》为题写读书笔记，
出现 CPO / view_interface / range_adaptor_closure / `__non_propagating_cache` /
iterator_concept / borrowed_range 六个关键词，且每个词有实质解释。

**交付**：`notes/04_reading_note.md` 正文完成，六关键词全部出现。

---

## 进阶任务

### 进阶 1：阅读 `zip_view` 的 proxy reference / iter_move / iter_swap

验证：`iterator_concept = random_access_iterator_tag` 但
`iterator_category = input_iterator_tag`（双轨现象）。
（对应 `main.cpp` TODO [进阶] 1）

### 进阶 2：阅读 `range_adaptor_closure` 基类 + `operator|` 的两条路径

libstdc++ 中搜索 `__adaptor` 命名空间；找到 `_Pipe<C1,C2>` 的组合 closure 结构。
（对应 `main.cpp` TODO [进阶] 2）

### 进阶 3：ranges adaptor vs sender adaptor 结构对比

完成对比表（拉模型 vs 推模型；closure 类型 / 内部状态传播 / 惰性点 / 类型安全）。
（对应 `main.cpp` TODO [进阶] 3）

---

## 固定交付物

1. `notes/01_object_diagram.md` — 实现层对象关系图（五节点，边标注 owns/points-to）
2. `notes/02_pattern_table.md` — 技术模式对照表（四行全部填写，含"你之前的误解"列）
3. `notes/03_cpo_checklist.md` — CPO 清单（八项，含 ranges CPO vs tag_invoke 对比）
4. `notes/04_reading_note.md` — 读书笔记（含六关键词，不罗列 view 名字列表）

---

## 验收点

- **对象关系图**：出现 view_interface / range_adaptor_closure / iterator / sentinel /
  cache 五节点；每条边标注"持有（owns）"或"指向（points-to）"；
  `__non_propagating_cache` 节点有"拷贝时重置"标注
- **技术模式对照表**：四行全部填完，无空格；iterator_concept 推导列有具体 min 表达式；
  "你之前的误解"列有实质内容
- **CPO 清单**：至少列出 ranges::begin / end / iter_move / iter_swap / advance /
  distance / size / data 八项，每项标注定制意图
- **读书笔记**：出现 CPO / view_interface / range_adaptor_closure /
  `__non_propagating_cache` / iterator_concept / borrowed_range 六个关键词；
  不直接罗列 view 名字列表
- **main.cpp**：取消注释的 static_assert 全部通过，`make` 无错误无警告

---

## 复盘问题

1. `__non_propagating_cache` 拷贝时重置缓存，与 O(1) copy 公理如何挂钩？
   如果缓存传播会破坏什么？
2. `transform_view` 把 contiguous 降为 random_access，`operator[]` 的返回类型如何体现这一降级？
3. `join_view` 的 xvalue 缓存和 `filter_view` 的 begin 缓存，缓存动机相同吗？
   分别解决什么问题？
4. ranges CPO 的"成员优先"设计和 stdexec tag_invoke 的"tag 携带语义"设计，
   各自在什么场景下更有优势？
5. `view_interface` 注入的 `size()` 需要底层满足什么 concept？
   `filter_view` 底层时这个注入如何静默失效？

---

## 对应官方参考

| 资源 | 内容 |
|------|------|
| libstdc++ `<bits/ranges_base.h>` | CPO 实现，`view_interface`，`range_adaptor_closure` |
| libstdc++ `<bits/ranges_factories.h>` | `single_view`，`iota_view`，`empty_view` |
| libstdc++ `<bits/ranges_adaptors.h>` | `transform_view`，`filter_view`，`join_view`，`zip_view` |
| MSVC STL `<ranges>` | 对照实现，注意命名约定不同（`_M_` → `_Myfoo` 等） |
| cppreference `std::ranges::filter_view` | 验收点参考，特别是 begin() const 限制说明 |
| P2387R3 | `range_adaptor_closure` 的正式化设计文档 |
| P0896R4 | ranges 总体设计，`borrowed_range` 引入动机 |
