# C13 标准与实现索引

资料核对日：2026-09-14。规范状态、第三方实现和本机能力是不同轴；固定源码及许可证见 [依赖清单](dependencies.json)。读者机器上的成功与否由实际编译决定，不能从本页的规范归属推断。

| 能力 | 规范依据 | 本课实际入口 |
|---|---|---|
| span / mdspan | C++20 / C++23 | 核心代码直接使用标准库；L03 与综合项目实际实例化原生 mdspan |
| CPU SIMD | C++26 固定草案 N5050 | 原生 F01 使用 `<simd>`、`std::simd::vec`；普通实验复用 C08 scalar/SSE2/xsimd，各自明确标名 |
| linalg | C++26 固定草案 N5050 | 原生 `<linalg>` 探针与独立主体；L11 用 stdBLAS 参考实现，不冒充原生支持 |
| submdspan、padded layouts、aligned accessor | C++26 草案设施 | F01 分别探测和保留完整主体；L03 的 C++23 主线不依赖这些扩展 |
| mdspan copy/fill | P3242R4，由 N5055 记录纳入 C++29 工作草案 | F01 `copy_fill.cpp`；不能用普通指针 copy 或 stdBLAS copy 代替原生能力判断 |
| quantities / units | 独立生态和标准化活动 | L10 固定 mp-units 2.5.0，不宣称它已经是 C++29 标准库接口 |

固定规范入口：[N5051 编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5051.html)、[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)、[N5055 编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)、[P3242R4](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3242r4.html)。这些固定版本方便核对教学接口；更新工具链或草案时，应重新核对受影响表达式。

## 第三方消费方式与真实名字

- [mp-units 2.5 接入说明](https://mpusz.github.io/mp-units/2.5/getting_started/installation_and_usage/)：使用 `mp_units` 命名空间和库自身的 `src` CMake 工程；保留 gsl-lite 合约，采用现有 `std::format`，不开模块和上游测试。
- [stdBLAS 固定源码](https://github.com/kokkos/stdBLAS/tree/146f4743a953e51d7319156f50d932bca7a6507f)：本快照通过 `<experimental/linalg>` 提供 `std::experimental::linalg`，其内部版本命名空间不应被当作用户接口。教学构建生成上游 `linalg_config.h.in`，仅启用 serial header 实现，不启用外部 BLAS/TBB/Kokkos 后端。
- [Kokkos mdspan 固定源码](https://github.com/kokkos/mdspan/tree/8989f70749e28f337e6f7aa210db88659dba6f2f)：`<experimental/mdspan>` 兼容入口决定命名空间宏及兼容导入。本课把它和 stdBLAS 放进独立可执行目标，避免与本机 `<mdspan>` 重复定义或误混类型；这不是把第三方头重命名为标准库。
- xsimd 13.2.0：复用 C08 的 `xsimd::batch<...,xsimd::sse2>` 路径，不静默提高目标 ISA。源码、版本与本地目标名字见依赖清单。

stdBLAS 的这个快照仍用 `mdspan(i,j)`，而所固定 mdspan 在 C++23 下默认选择多参数下标。构建仅为该参考目标设置上游开关 `MDSPAN_USE_PAREN_OPERATOR=1`，保留它所需的兼容操作符；没有修改第三方源文件，也不影响原生 mdspan 目标。

只构建 C13 自己的 consumer，不跑上述库的完整测试套件。依赖源码保留原许可证；源码本身和下载归档不纳入本仓库提交范围。

## 配置、缺能力和失败怎样区分

`C13_ENABLE_THIRD_PARTY=OFF` 是主动关闭。打开后缺依赖是配置错误，不能通过“跳过所有 consumer”伪造完整第三方编译成功。`C13_FETCH_DEPS=ON` 允许按固定提交下载；关闭时需要缓存或对应的 `FETCHCONTENT_SOURCE_DIR_*`，本地覆盖来源由调用者负责，不能把任意副本自动宣称为清单版本。

`C13_ENABLE_FRONTIER=OFF` 也是主动关闭。打开时，每项原生能力先编译一个最小真实表达式；缺失则明确输出 unavailable，并保留完整实验源码。具备最小能力后，主体构建或运行失败就是失败，不能重新标成“工具链不支持”。本机输出写入构建目录的 F01 `capabilities.txt`，不作为教材内的固定测量结果链接。

工具链宣称支持 C++26、头文件存在、特性宏出现、某个表达式可编译、完整主体通过是不同证据。本页只划分教学和规范责任，不宣称穷尽某个标准库实现的全部接口。
