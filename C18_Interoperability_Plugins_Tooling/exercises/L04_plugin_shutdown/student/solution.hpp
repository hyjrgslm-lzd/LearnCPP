#pragma once

#include "c18/abi.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>
#include <vector>

namespace c18_l04 {

struct ProcessResult {
  c18_status status{C18_STATUS_PLUGIN_ERROR};
  std::vector<uint8_t> bytes{};
  size_t required{};
};

class PluginSession {
public:
  auto open(const std::filesystem::path&) -> c18_status { return C18_STATUS_PLUGIN_ERROR; }
  auto set_callback(std::function<void(std::string_view)>) -> void {}
  auto process(const std::vector<uint8_t>& input, size_t) -> ProcessResult {
    return {C18_STATUS_PLUGIN_ERROR, {}, input.size()};
  }
  auto request_stop() -> c18_status { return C18_STATUS_PLUGIN_ERROR; }
  auto close() -> c18_status { return C18_STATUS_OK; }
  auto try_lock_probe() -> bool { return true; }
  auto set_observer(std::function<void(std::string_view)>) -> void {}
  auto fail_next_release_for_test() -> void {}
};

}  // namespace c18_l04
