#if defined(_MSC_VER)
// 未填 TODO 故意抛异常；MSVC /O2 会把后续成功分支诊断成 C4702。
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
#include "checks.hpp"
#include <exception>
#include <iostream>

int main() {
    try {
        student_checks::run(); // 预检实际调用全部学生步骤；通过后才启动 worker。
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "student FAIL: " << error.what() << '\n';
        return 1; // 未完成或错误都不是能力缺失，不能返回 77。
    }
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
