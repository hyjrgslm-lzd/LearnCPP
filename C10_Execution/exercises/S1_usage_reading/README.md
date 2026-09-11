# S1_usage_reading：固定版本源码阅读

这是观察与阅读单元，没有Student完成标记。共同行为程序在main.cpp；完整阅读路线、问题和解析见[源码导读](../../chapters/14-source-reading.md)。

S1实际运行惰性then、stdexec::task、exec::task和async_scope三个工作收束；S2实际运行when_all的值组合和外部jthread驱动的run_loop。程序检查值、调用计数、真实执行线程和退出顺序；不使用打印出来的模拟trace冒充库行为。

按[构建指南](../BUILD_GUIDE.md)设置STDEXEC_ROOT，可单独配置本目录或root选择S1_usage_reading。CTest通过只证明本例执行了声明的检查。读者还需完成对象图、错误/停止路径、源码入口与教学实现差异说明；不能由程序退出0代替阅读任务。

固定stdexec为nvhpc-26.05 / 6d7ad689f4d4831c5136e4abe1c601f9a3b64e43。实际标准库std::execution能力由F01独立测试，本单元使用stdexec参考实现和明确标注的exec扩展。
