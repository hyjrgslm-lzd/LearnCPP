# 03 模块 B：generator 与 task 的使用

模块 A 讲清了三个关键字各自触发什么机制。模块 B 开始把这些机制组合起来：generator 可以嵌套生产，task 可以顺序等待另一个 task，回调 API 可以被包装成 awaiter。

这三件事的共同点是“把后续工作保存到某个明确对象里”。递归 generator 把子生产者保存进父生产者的推进过程；父 task 在 `co_await child` 时保存自己的恢复点；callback awaiter 把当前协程的非拥有 handle 接到回调完成事件上。

<a id="b1"></a>

## 一、递归 generator：生产者里还有生产者

先从一棵二叉树看起。中序遍历的同步写法是：遍历左子树，访问当前节点，遍历右子树。generator 写法也表达同一件事，只是每个“访问节点”变成一次 `co_yield`。

```cpp
std::generator<int> inorder(Node* root) {
    if (!root) co_return;
    for (int v : inorder(root->left)) co_yield v;
    co_yield root->value;
    for (int v : inorder(root->right)) co_yield v;
}
```

每次递归调用都会创建一个新的 generator 对象和一份新的协程状态。父 generator 运行到左子树循环时，会驱动子 generator 产出值；子树产出的每个值再由父 generator 转发给外层消费者。

```text
main 推进 root generator
  root generator 正在等待 left generator 的下一个值
    left generator 正在等待更深 left generator 的下一个值
      leaf generator co_yield leaf.value
```

每一层都有独立的协程帧。父层并未丢失状态，它停在“等待子 generator 下一个值”的循环位置。消费者继续推进时，恢复链会从外层一路触达当前活跃的内层生产者。

### `elements_of` 表达递归产出

C++23 `std::generator` 提供 `std::ranges::elements_of`，用于把一个内层 range 的元素作为外层 generator 的产出序列：

```cpp
std::generator<int> inorder(Node* root) {
    if (!root) co_return;
    co_yield std::ranges::elements_of(inorder(root->left));
    co_yield root->value;
    co_yield std::ranges::elements_of(inorder(root->right));
}
```

这表达的是“从这里开始，当前 generator 的输出接上子 generator 的输出”。标准 generator 为嵌套 generator 维护 active stack，让控制在父子 generator 间转交。它和手写 `for + co_yield` 的输出可以相同，但库能把递归产出作为 generator 协议的一部分处理，避免让用户代码层层循环转发成为主要机制。

本仓库 reference 当前采用两个可运行路径：普通递归 generator 和显式栈 flattening。starter 里保留 `elements_of` 练习入口，适合在支持该语法的标准库上补全。若当前编译器或标准库暂不支持，先用 reference 的普通递归与显式栈版本确认中序语义，再把 `elements_of` 作为版本能力实验。

在 [练习 B-1](exercises/B1_recursive_generator/README.md) 中，关键观察是每个父帧停在哪：左子树未结束前，当前节点的 `co_yield root->value` 尚未执行；左子树结束后，父帧才继续产出当前节点。

<a id="b2"></a>

## 二、task 顺序链：父协程暂停，子协程完成后回来

`lazy_task<T>` 可以被另一个协程 `co_await`。父协程写出线性代码：

```cpp
auto user = co_await fetch_user(id);
auto profile = co_await parse_profile(user);
auto result = co_await validate_profile(profile);
co_return format(result);
```

执行上，它是一条父子 task 链。父协程运行到 `co_await fetch_user(id)` 时暂停，把自己的恢复点交给 child task 的 awaiter；child task 运行并 `co_return User`；child 的 final suspend 通知父协程；父协程恢复，`await_resume()` 返回 `User`。后两步重复同样模式。

这条链的价值是数据流和错误流都在代码里保持线性。`User` 通过第一个 `co_await` 的返回值进入父协程；`Profile` 通过第二个 `co_await` 进入父协程。中间值不需要塞进全局状态，也不需要拆成多层回调参数。

异常也是同一条链。若 `fetch_user` 抛出，它自己的 promise 保存异常；父协程恢复到 `co_await fetch_user` 时，`await_resume()` 重新抛出。父协程可以在这行外层用 `try/catch` 处理；如果不处理，父协程的 `unhandled_exception()` 会继续保存，最终由 `sync_wait` 抛给 main。

对照回调写法，差异就清楚了：

```cpp
fetch_user(id, [](User user) {
    parse_profile(user, [](Profile profile) {
        validate_profile(profile, [](ValidatedProfile result) {
            use(result);
        });
    });
});
```

回调版本把后续逻辑拆进闭包对象。每一层都要决定如何传递错误和中间值。协程版本仍然有状态保存，只是状态在协程帧中，恢复点由 awaiter 管理，源代码保留顺序结构。

在 [练习 B-2](exercises/B2_task_sequential/README.md) 中，用日志确认 `fetch;parse;validate;` 顺序，再用负 id 触发 `fetch_user` 异常。验收时要能解释为什么 parse/validate 没有执行：异常在第一个 `co_await` 点重新抛出，父协程没有进入后续语句。

<a id="b3"></a>

## 三、回调 API 到 awaiter：把完成事件接到 `resume()`

很多旧接口形状是“启动操作，完成时调用回调”：

```cpp
template <class Callback>
void async_add(worker_group& workers, int a, int b, Callback cb);
```

协程要等待这种操作，只需要一个 awaiter 作为适配层。awaiter 在 `await_suspend` 中启动异步操作，并把当前协程 handle 接进回调：

```cpp
struct async_add_awaiter {
    worker_group& workers;
    int a;
    int b;
    int result = 0;

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        async_add(workers, a, b, [this, h](int value) {
            result = value;
            h.resume();
        });
    }

    int await_resume() const noexcept { return result; }
};
```

这个模式短，但要理解两个生命期。第一，awaiter 对象位于挂起协程的协程状态里，`co_await` 尚未完成时它保持存活，所以回调能在恢复前写 `result`。第二，回调里保存的 `h` 是非拥有 handle；它只允许恢复协程，不能独立保证帧还活着。协程帧的拥有者必须覆盖这次异步操作。

本仓库 B3 reference 用 `worker_group` 拥有所有 `std::jthread`，main 在检查后 `join()`。这样后台工作不会脱离 owner。不要用 `detach()` 隐藏问题；一旦 detach 的线程还拿着 awaiter 的 `this` 或协程 handle，而外部已经销毁 task，就会形成悬挂。

### 同步完成窗口

真实回调 API 可能在 `async_add(...)` 返回前就调用回调。也就是说，`await_suspend` 内部可能发生同步完成。如果回调直接 `h.resume()`，协程会在 `await_suspend` 还没返回时重入，库代码很容易写错。

安全设计通常把“已经完成”放进 `await_ready()` 快路径，或让 `await_suspend` 返回 `false` 表示不挂起、由当前调用链继续。模块 B 的 starter 不要求实现完整同步完成处理，但 README 和复盘必须能指出这个窗口。A3 的 future ready 快路径就是同类问题的简单版本。

### 结果同步

回调先写 `result`，再调用 `h.resume()`。在同一个 worker 线程内，这个顺序保证协程恢复后进入 `await_resume()` 时能读到刚写的值。跨线程场景还要依赖底层 API、mutex/atomic、事件队列或 join 建立可见性。本题 reference 用 mutex 保护状态，教学上更容易看出“写结果”和“恢复协程”是两个动作。

在 [练习 B-3](exercises/B3_callback_to_awaiter/README.md) 中，先按最小 `async_add_awaiter` 跑通，再画出四个点：`await_suspend` 在线程 A，`async_add` 的 worker 在线程 B，回调先写结果，`h.resume()` 后协程的剩余代码在线程 B 继续。

**答案解析：** 图上应把 `await_suspend(h)` 标在启动协程的线程，表示当前协程已经暂停且 handle 被交给 awaiter。`async_add` 提交 worker 后，worker 线程先执行回调、写入 result，再调用 `h.resume()`；恢复后的 `await_resume()` 和后续协程语句也在这个 worker 线程继续。图中还要标出 `h` 非拥有、awaiter 在协程帧内，避免把回调里的 handle 画成 owner。

## 模块 B 完成后

你应该能解释：

- 递归 generator 的每一层是独立生产者，父帧停在等待子生产者的位置。

  **答案解析：** B1 递归到最左叶子时，root generator、left generator、leaf generator 分别有自己的协程帧。父层没有继续执行当前节点输出，而是停在转发左子 generator 的循环或 `elements_of` 上；叶子 `co_yield value` 后，值沿转发链回到最外层消费者。这个图能解释为什么递归 generator 仍要关心嵌套恢复链。

- `co_await task` 让父 task 暂停，子 task 的完成值或异常通过 `await_resume()` 回到父协程。

  **答案解析：** B2 的 `process_user` 先 `co_await fetch_user`，拿到 `User` 后才进入 `parse_profile`，再进入 `validate_profile`。如果 `fetch_user(-1)` 抛异常，子 promise 保存异常，父协程在第一个 `await_resume()` 处重新抛出，因此 parse/validate 没有启动。值路径和异常路径都回到同一行 `co_await`。

- 回调 awaiter 的核心是保存当前协程 handle，并在完成回调中先写结果、再恢复协程。

  **答案解析：** B3 的 `await_suspend(h)` 把 `h` 放进 `async_add` 的回调，后台 worker 算出 `7 + 5` 后先写 result，再调用 `h.resume()`。恢复后协程执行 `await_resume()` 读取 result，随后继续做乘法得到 36。先写结果再恢复，是因为恢复后的代码马上会读这个结果。

- 每条跨挂起点的引用、指针和 handle 都要有清楚的 owner 覆盖其使用期。

  **答案解析：** awaiter 对象通常在协程帧中，回调里捕获的 `this` 或 handle 都依赖这块帧在恢复前仍然活着。frame owner 由 task 或 `sync_wait` 契约负责；后台线程 owner 由 `worker_group` 负责。B3 当前路径里，回调写结果并 `resume()` 后不再访问 awaiter，随后线程退出并由 `worker_group.join()` 收束；若用 `detach()` 或让外部引用先析构，就会把问题推迟到恢复点爆出。

下一模块把这些能力放进并发拓扑：取消如何传播，多个 task 如何汇合，动态任务如何被 scope 收束。
