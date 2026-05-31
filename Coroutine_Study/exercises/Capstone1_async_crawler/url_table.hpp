// =====================================================================
// Capstone1 / url_table.hpp
// 模拟 URL 表：固定 3~5 条记录，每条带"内容"和"延迟"。
// 用字符串模拟网络响应，避免引入真实 HTTP 客户端。
// =====================================================================
#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace capstone1 {

struct UrlRecord {
    std::string url;
    std::string body;                       // 模拟响应正文
    std::chrono::milliseconds latency{0};   // 模拟网络延迟
};

inline const std::vector<UrlRecord>& url_table() {
    using namespace std::chrono_literals;
    static const std::vector<UrlRecord> tbl{
        {"data/1.csv",
         "name,score\nAlice,85\nBob,92\n",
         150ms},
        {"data/2.csv",
         "name,score\nCarol,78\nDave,88\n",
         100ms},
        {"data/3.csv",
         "name,score\nEve,95\nFrank,82\n",
         300ms},
    };
    return tbl;
}

} // namespace capstone1
