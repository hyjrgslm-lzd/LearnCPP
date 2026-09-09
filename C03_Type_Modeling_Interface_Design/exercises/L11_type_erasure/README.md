# L11：类型擦除

先读 `../../chapters/11-type-erasure.md`。你只编辑 `src/student/any_shape.hpp`，实现 heap-only 拥有型 `l11::AnyShape`。

接口固定：

```cpp
class AnyShape {
public:
    AnyShape() noexcept;

    template<class T>
    AnyShape(T value);

    AnyShape(const AnyShape& other);
    AnyShape(AnyShape&& other) noexcept;
    AnyShape& operator=(const AnyShape& other);
    AnyShape& operator=(AnyShape&& other) noexcept;
    ~AnyShape();

    explicit operator bool() const noexcept;
    std::string name() const;
    Dimensions dimensions() const;
    const void* target_address() const noexcept;
    void swap(AnyShape& other) noexcept;
};
```

Part 1：默认构造和移动后为空，`operator bool()` 为 false。访问空态抛 `std::logic_error("empty AnyShape")`，不能 UB。

Part 2：构造时复制或移动一个满足 `ShapeObject` 的目标类型到 heap。`name()` 和 `dimensions()` 通过操作表做 const dispatch。

Part 3：复制构造和复制赋值做深复制。两个 `AnyShape` 不能共享同一个目标地址。

Part 4：复制赋值用 copy-and-swap。若目标 copy 构造抛出，赋值左侧保持调用前的可观察状态。

Part 5：移动、copy self-assignment、move self-assignment 和析构都保持对象可用；析构释放目标一次。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L11_type_erasure -B build/c03-l11 -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-l11 --config Debug
ctest --test-dir build/c03-l11 -C Debug --output-on-failure
```

解析：`AnyShape` 不是继承层次，也不是模板容器。它用一张操作表把具体类型的 clone/destroy/name/dimensions 藏起来，对外提供一个可复制、可移动、可为空的值类型。固定 heap-only 是本题边界；不要加小对象优化或 allocator 框架。

