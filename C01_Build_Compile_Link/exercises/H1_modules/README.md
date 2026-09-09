# 练习 H1：从文本包含走到模块依赖与包消费

先读[08 Modules](../../chapters/08-modules.md)。本题是完整可运行的观察/实验起点，不把修改Reference当成学生完成。先预测，再运行、检查产物、解释；需要扩展时使用下方独立工作区。命令从LearnCPP根目录的x64 Native Tools环境运行，使用本机已有MSVC、CMake与Ninja。

## Part 1：建立可比较基线

`header_baseline/`给出同一行为的头文件版本；`reference/geometry.ixx`、`geometry.partition.ixx`、`geometry.cpp`分别承担primary interface、导出partition和implementation unit。

```powershell
cmake -S C01_Build_Compile_Link/exercises/H1_modules -B C01_Build_Compile_Link/exercises/build/learner-h1 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build C01_Build_Compile_Link/exercises/build/learner-h1 --verbose
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-h1 --output-on-failure
```

这里构建这个leaf的全部正常目标，再运行其全部注册检查，避免只构建一个exe却测试另几个尚不存在的程序。故意失败目标被`EXCLUDE_FROM_ALL`隔离。

**解析：** 两条正常路径的值可以相同，但构建过程不同。header由各TU预处理；module interface产生object和BMI/IFC，importer先依赖可用BMI，再产生自己的object，最后链接。找到`.ddi`、module map和实际编译行，标出哪些是编译顺序依赖、哪些是最终链接输入；不要只凭`.ixx`扩展名猜。

## Part 2：名字不可见不等于类型不可用

先预测`visibility_main.cpp`能否使用`make_hidden()`的返回值、读取`state.value`、调用`read_hidden(state)`。再运行`H1_visibility_boundary`，读`visibility.ixx`的导出边界。

**解析：** 未导出的`hidden_state`名字不能被importer直接查找到，但类型定义可以可达。工厂返回它时，消费者用`auto`获得对象并访问公开成员；公开函数签名包含这个类型并不因此非法。把“无法直接拼写类型名”误说成“完全不能使用该类型”，会误判合法模块接口。

显式构建直接命名隐藏类型的反例：

```powershell
cmake --build C01_Build_Compile_Link/exercises/build/learner-h1 --target H1_hidden_type_negative --verbose
```

此命令应失败，不属于普通build通过路径。检查失败发生在consumer编译阶段，诊断指向`hidden_state`不可见；找不到编译器、模块没构建、链接器错误或timeout都不是这个反例。

## Part 3：global/private fragment

`fragments.ixx`/`fragments_main.cpp`提供真实单文件模块正例。比较legacy头引入的global fragment与private fragment中的实现，运行`H1_global_private_fragment`。

**解析：** global fragment用于预处理传统头及宏配置，不自动把宏导出给importer。private fragment位于primary interface尾部，把实现部分挡在importer可达范围之外；该module应只有这一unit。这个唯一unit约束含no-diagnostic-required边界，不能要求所有编译器对违例都给同一错误。不要把此例与多单元geometry的partition/implementation混为一个module。

## Part 4：安装后独立消费

`H1_modules_package_consumer`会配置`package/library`、安装module interface源码和导出metadata，然后配置独立consumer。阅读原始日志中的prefix、module源位置、重新生成的consumer BMI和安装库的链接位置。

**解析：** package不只是复制一个`.ifc`。consumer根据安装接口及自身配置构建BMI，再链接安装的库；这证明当前组合的源码/metadata交付链，不能推出任意编译器、版本和flags能共享BMI。普通header包的重定位与版本兼容继续见[J1](../J1_package/README.md)。

## 独立工作区

要改接口、partition或consumer做扩展，先建立自己的副本，保留原Reference。不要把整个`exercises`递归复制到它自己的build里面。

```powershell
$work = 'C01_Build_Compile_Link/exercises/build/learner-h1-workspace'
if (Test-Path -LiteralPath $work) { throw '请选择尚不存在的工作目录' }
New-Item -ItemType Directory -Path $work | Out-Null
Copy-Item -LiteralPath C01_Build_Compile_Link/exercises/H1_modules -Destination "$work/H1_modules" -Recurse
Copy-Item -LiteralPath C01_Build_Compile_Link/exercises/cmake -Destination "$work/cmake" -Recurse
Copy-Item -LiteralPath C01_Build_Compile_Link/exercises/include -Destination "$work/include" -Recurse
cmake -S "$work/H1_modules" -B "$work/out" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$work/out"
```

只改副本。尝试增加一个导出函数并调用，再把一个只供实现使用的声明保持非导出，观察名字查找差别；预期、实际命令和解释分别记录。

**扩展解析：** 导出函数需要接口声明、可达的必要类型和实际定义；链接库不能替编译阶段补声明。`export import :part`和普通`import :part`传播的接口不同；partition只在同一个named module内部被导入。扩展检查成功不意味着已覆盖所有模块规则，仍须说明自己改变的边界。
