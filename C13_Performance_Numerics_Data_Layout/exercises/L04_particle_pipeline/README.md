# L04 综合项目：从输入粒子到输出坐标

先修：[稳定归约](../L01_stable_reduction/README.md)、[布局](../L02_layout/README.md)、[矩阵](../L03_tiled_matrix/README.md)及[综合正文](../../chapters/10-particle-pipeline.md)。这是一次有界数据处理管线，不是完整物理引擎。

## 要实现什么

编辑 `student/solution.hpp` 中 `student::run_pipeline(n,seed,milliseconds)`：

1. 检查 duration 有限且在 `[0,1000] ms`，空输入也要检查。
2. 用提供的 `make_particles(n,seed)` 生成三维位置和速度。规模上限一百万，随机映射固定。
3. 把毫秒转换成 float 秒，完成一次三轴 `position += velocity*dt`。
4. 对更新后位置应用固定 3×3 矩阵 `[0,-1,0;1,0,0;0,0,1]`。输出每行是 `(-y,x,z)`。
5. 返回拥有存储的行优先 `N×3` 坐标数组及其全部元素的稳定和。不能只返回一个看起来合理的统计数。

可以复用已学会的生成器、布局、参数验证和稳定求和内核；必须自己组织更新、变换和结果。不得直接调用完整 `c13::run_pipeline` 或 Reference 代做。

## 构建与判断

完整构建目标为 `C13_L04_pipeline_student/reference/good/bad`，观察驱动为 `c13_pipeline`。Student 初始退出 1，默认不注册为通过。单题从课根运行 `cmake -S exercises/L04_particle_pipeline -B build/pipeline -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release`，随后构建并运行对应目标。

检查器使用 0、1、257 粒子，逐坐标按独立标量表达式构造答案，再检查归约结果。错误时间拒绝不是输出一组零。bad 故意忽略 duration；它仍返回正确形状、有限数值，却应该被逐元素检查拒绝。

## 解析与观察

Reference 调用完整课内管线；good 直接逐粒子推导 `(-y,x,z)`，没有调用 Reference 或整个管线。两者复用此前稳定求和单元是合理的跨课能力组合，不能据此假装独立验证了求和算法；求和算法自己的检查在 L01。

`c13_pipeline --size 10000 --seed 20260914` 计时从已有输入到结果就绪：含 SoA 转换、更新、矩阵变换、归约以及函数内临时数组清理，不含随机生成、输入销毁、结果销毁和最终核对。它是明确边界的一次处理延迟，不是全进程运行时间。输出核对失败则不会产生有效性能行。
