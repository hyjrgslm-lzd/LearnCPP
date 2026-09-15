# L10：静态多态观察

先读 `../../chapters/10-static-polymorphism.md`。本练习不需要改 Student 文件；运行观察程序并解释每个类型为什么满足或不满足 `ShapeLike`。

程序覆盖：

- `ShapeLike` concept 检查 `name()` 与 `dimensions()` 的语法接口。
- `describe()` 在调用点按具体类型实例化。
- CRTP facade 通过 `static_cast<Derived&>(*this)` 调到派生实现。
- 依赖基类成员用 `this->` 参与查找。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L10_static_polymorphism -B build/c03-l10 -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-l10 --config Debug
ctest --test-dir build/c03-l10 -C Debug --output-on-failure
```

解析：concept 是接口门，不是语义证明。`ShapeLike` 能挡住缺少成员的类型；宽高为正、名称稳定这些业务承诺仍要由实现和检查维持。CRTP 成立的前提是派生类确实继承 `Facade<自己>`，并且不要在基类构造/析构期调用派生实现。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L10_static_polymorphism_observation`，可直接设为启动项运行观察或专项入口。
没有 Student 编辑入口；运行/阅读：`observation/`。这些入口保留在题目分组内，作为观察、探测或负例诊断。
构建主项目后运行 CTest 或 README 中列出的检查命令；负例/探测入口只读，用于观察诊断。
