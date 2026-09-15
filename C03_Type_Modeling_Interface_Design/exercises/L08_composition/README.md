# L08：组合与继承观察

先读 `../../chapters/08-composition-and-inheritance.md`。本练习不需要改 Student 文件；运行观察程序，解释每个 `check` 为什么成立。

程序覆盖：

- 组合表达“有一个矩形和一个标签”，不会公开矩形的全部接口。
- `public` 继承表达可替换为基类，派生类必须保持正尺寸后置条件。
- NVI 在非虚入口里统一检查派生实现返回值。
- 构造阶段调用 virtual 不会派发到尚未构造的派生部分。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L08_composition -B build/c03-l08 -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-l08 --config Debug
ctest --test-dir build/c03-l08 -C Debug --output-on-failure
```

解析：组合是默认建模工具；继承只在调用者需要按基类契约替换派生对象时使用。NVI 能集中检查公开后置条件，但不能让错误派生实现自动正确。构造/析构期不要依赖派生虚行为。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L08_composition_observation`，可直接设为启动项运行观察或专项入口。
没有 Student 编辑入口；运行/阅读：`observation/`。这些入口保留在题目分组内，作为观察、探测或负例诊断。
构建主项目后运行 CTest 或 README 中列出的检查命令；负例/探测入口只读，用于观察诊断。
