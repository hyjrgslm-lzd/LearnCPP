#include "c18/abi.h"
#include "c18/bytes.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

struct c18_context {
  c18_host_api host{};
  std::atomic<bool> closing{false};
  std::atomic<int> active_processes{0};
  std::atomic<int> active_callbacks{0};
};

struct active_count {
  std::atomic<int>& count;
  explicit active_count(std::atomic<int>& value) : count(value) { ++count; }
  active_count(const active_count&) = delete;
  auto operator=(const active_count&) -> active_count& = delete;
  ~active_count() { --count; }
};

static void emit(c18_context* ctx, const char* event_name) {
  if (ctx != nullptr && ctx->host.on_event != nullptr) {
    active_count lease{ctx->active_callbacks};
    try {
      ctx->host.on_event(ctx->host.userdata, event_name);
    } catch (...) {
    }
  }
}

static c18_status C18_CALL create_context(const c18_host_api* host, c18_context** out) {
  if (out != nullptr) {
    *out = nullptr;
  }
  if (host == nullptr || out == nullptr || host->version != C18_ABI_VERSION ||
      host->struct_size < sizeof(c18_host_api)) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  try {
    auto* ctx = new c18_context;
    ctx->host = *host;
    *out = ctx;
    return C18_STATUS_OK;
  } catch (...) {
    return C18_STATUS_PLUGIN_ERROR;
  }
}

static c18_status C18_CALL process(c18_context* ctx,
                                   const uint8_t* input,
                                   size_t input_size,
                                   uint8_t* output,
                                   size_t output_capacity,
                                   size_t* written) {
  if (ctx == nullptr || written == nullptr || (input == nullptr && input_size != 0) ||
      (output == nullptr && output_capacity != 0)) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  *written = input_size;
  if (ctx->closing.load()) {
    return C18_STATUS_CLOSING;
  }
  if (output_capacity < input_size) {
    return c18::transform_bytes(input, input_size, output, output_capacity, written);
  }
  active_count lease{ctx->active_processes};
  for (size_t i = 0; i < input_size; ++i) {
    if (input[i] == static_cast<uint8_t>('!')) {
      emit(ctx, "process:bang");
    }
  }
  return c18::transform_bytes(input, input_size, output, output_capacity, written);
}

static c18_status C18_CALL request_stop(c18_context* ctx) {
  if (ctx == nullptr) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  ctx->closing.store(true);
  emit(ctx, "stop:called");
  return C18_STATUS_OK;
}

static c18_status C18_CALL destroy(c18_context* ctx) {
  if (ctx != nullptr && (ctx->active_processes.load() != 0 || ctx->active_callbacks.load() != 0)) {
    return C18_STATUS_BUSY;
  }
#ifdef C18_L04_BUSY_DESTROY
  static std::atomic<int> destroy_attempts{0};
  if (destroy_attempts.fetch_add(1) == 0) {
    return C18_STATUS_BUSY;
  }
#endif
  emit(ctx, "destroy:called");
  delete ctx;
  return C18_STATUS_OK;
}

extern "C" C18_EXPORT c18_status C18_CALL c18_get_api(uint32_t requested_version,
                                                       uint32_t host_struct_size,
                                                       c18_api* out_api) {
  if (out_api == nullptr || host_struct_size < sizeof(c18_api)) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  if (requested_version != C18_ABI_VERSION) {
    return C18_STATUS_UNSUPPORTED_VERSION;
  }
  *out_api = c18_api{C18_ABI_VERSION, sizeof(c18_api), create_context, process, request_stop, destroy};
  return C18_STATUS_OK;
}
