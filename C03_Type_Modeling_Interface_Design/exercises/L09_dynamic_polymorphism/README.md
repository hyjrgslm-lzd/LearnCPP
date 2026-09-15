# L09：动态多态

先读 `../../chapters/09-dynamic-polymorphism.md`。你只编辑 `src/student/shape.hpp`，实现抽象基类 `l09::Shape`、派生类 `Rectangle` 和 `Circle`。

接口固定：

```cpp
struct Dimensions { int width; int height; };

class Shape {
public:
    virtual std::string name() const = 0;
    virtual Dimensions dimensions() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual ~Shape() = default;
};
```

Part 1：`Rectangle` 与 `Circle` 公开继承 `Shape`，`name()` 分别返回 `"rectangle"` 和 `"circle"`，`dimensions()` 返回构造时保存的正尺寸。非法尺寸抛异常。

Part 2：`clone()` 返回新的动态对象，观察值相同，地址不同，动态类型不变。

Part 3：通过 `std::unique_ptr<Shape>` 删除派生对象时，派生析构必须执行。练习中的 `alive_count()` 用来观察这一点。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L09_dynamic_polymorphism -B build/c03-l09 -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-l09 --config Debug
ctest --test-dir build/c03-l09 -C Debug --output-on-failure
```

解析：动态多态的核心不是共享字段，而是通过基类契约替换调用。`override` 让签名错误在编译期暴露；虚析构让拥有基类指针能正确释放派生部分；`clone()` 负责把动态类型复制出来，避免切片和别名。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L09_dynamic_polymorphism_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/shape.hpp`。 `checks/`、`validation/`、`src/reference/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
