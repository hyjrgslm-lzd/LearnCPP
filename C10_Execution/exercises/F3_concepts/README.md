# F3 sender/receiver concepts

实现最小 sender、receiver、receiver_of、sender_to concept，区分语法满足与语义约束。

Part:

1. `my_sender<S>` 检查 sender 标记和 completion signatures。
2. `my_receiver<R>` 检查 receiver 标记和 stopped 入口。
3. `my_receiver_of<R, Sigs>` 检查 receiver 能处理每个 value/error/stopped 签名。
4. `my_sender_to<S, R>` 把 sender 的签名合同接到 receiver 能力。
5. bad 版本只检查 receiver 语法，不检查 value 参数，所以会错误接受 `partial_receiver`。

Student 初态提供合法类型，但 concept 都是 false，checker 会以 `valid sender satisfies syntax concept` 拒绝首个语法概念检查，预期 exit 1；这不是任意非零。
