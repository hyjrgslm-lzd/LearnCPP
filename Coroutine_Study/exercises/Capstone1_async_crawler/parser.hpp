// =====================================================================
// Capstone1 / parser.hpp
// 用 std::generator 把响应字符串按行惰性 yield 成 Record。
// 重点：parse 不一次性把所有行加载到内存，而是按需产出。
// =====================================================================
#pragma once

#include <generator>
#include <string>
#include <string_view>

namespace capstone1 {

struct Record {
    std::string name;
    int         score = 0;
    bool        ok    = true; // 解析失败时仍 yield，但 ok=false
};

inline std::generator<Record>
parse_lines(std::string body) {
    // TODO [必做 6]：
    //   1. 跳过首行 header（"name,score"）。
    //   2. 按 '\n' 拆分剩余行，逐行 co_yield 一个 Record。
    //   3. 跳过空行；格式错误的行 yield Record{ok=false} 但继续处理。
    //
    //   占位实现：按 '\n' 拆分，每行 yield 一条 Record，让 total_lines 不为 0。
    //   完整实现请按 header / 逗号拆分 / 数字解析补全。
    std::size_t pos = 0;
    std::size_t line_no = 0;
    while (pos < body.size()) {
        auto nl = body.find('\n', pos);
        std::string_view line = (nl == std::string_view::npos)
            ? std::string_view{body}.substr(pos)
            : std::string_view{body}.substr(pos, nl - pos);
        ++line_no;
        if (line_no == 1) {
            // 占位：跳过首行 header
            if (nl == std::string_view::npos) break;
            pos = nl + 1;
            continue;
        }
        if (!line.empty()) {
            co_yield Record{std::string{line}, 0, true};
        }
        if (nl == std::string_view::npos) break;
        pos = nl + 1;
    }
}

} // namespace capstone1
