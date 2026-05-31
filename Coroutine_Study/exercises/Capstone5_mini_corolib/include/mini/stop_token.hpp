// =============================================================================
// mini/stop_token.hpp —— 协作式取消
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第八层"
//
// 设计选择：
//   - 直接 typedef 标准库 std::stop_token / std::stop_source（最稳）；
//   - 高级用户可换为 in_place_stop_source（不分配，stdexec 风格）。
// =============================================================================

#pragma once

#include <stop_token>

namespace mini {

using stop_token  = std::stop_token;
using stop_source = std::stop_source;

// in_place_stop_source 占位（参考 stdexec 的 in_place_stop_token）
// TODO[进阶]: 实现一个不分配的 in-place 版本：
//   - bool stop_requested() const noexcept;
//   - 注册回调链表；
//   - request_stop() 触发链表回调。
class in_place_stop_source {
    std::stop_source impl_;
public:
    auto get_token() const noexcept { return impl_.get_token(); }
    bool request_stop() noexcept { return impl_.request_stop(); }
    bool stop_requested() const noexcept { return impl_.stop_requested(); }
};

} // namespace mini
