#include "c18/abi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct c18_context {
  c18_host_api host;
  bool closing;
};

struct c18_l05_counts {
  uint32_t get_api_calls;
  uint32_t create_calls;
  uint32_t process_calls;
  uint32_t request_stop_calls;
  uint32_t destroy_calls;
};

static uint32_t g_get_api_calls = 0;
static uint32_t g_create_calls = 0;
static uint32_t g_process_calls = 0;
static uint32_t g_request_stop_calls = 0;
static uint32_t g_destroy_calls = 0;
static bool g_destroy_busy_once = false;

C18_EXPORT void C18_CALL c18_l05_reset_counts(void) {
  g_get_api_calls = 0;
  g_create_calls = 0;
  g_process_calls = 0;
  g_request_stop_calls = 0;
  g_destroy_calls = 0;
  g_destroy_busy_once = false;
}

C18_EXPORT void C18_CALL c18_l05_set_destroy_busy_once(uint32_t enabled) {
  g_destroy_busy_once = enabled != 0;
}

C18_EXPORT c18_status C18_CALL c18_l05_query_counts(struct c18_l05_counts* out) {
  if (out == NULL) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  out->get_api_calls = g_get_api_calls;
  out->create_calls = g_create_calls;
  out->process_calls = g_process_calls;
  out->request_stop_calls = g_request_stop_calls;
  out->destroy_calls = g_destroy_calls;
  return C18_STATUS_OK;
}

static c18_status C18_CALL create_context(const c18_host_api* host, c18_context** out) {
  ++g_create_calls;
  if (host == NULL || out == NULL || host->version != C18_ABI_VERSION ||
      host->struct_size < sizeof(c18_host_api)) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  c18_context* ctx = (c18_context*)calloc(1, sizeof(c18_context));
  if (ctx == NULL) {
    return C18_STATUS_PLUGIN_ERROR;
  }
  ctx->host = *host;
  ctx->closing = false;
  *out = ctx;
  return C18_STATUS_OK;
}

static c18_status transform_bytes(const uint8_t* input, size_t length, uint8_t* output,
                                  size_t capacity, size_t* written) {
  if (written == NULL) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  *written = 0;
  if (length != 0 && input == NULL) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  if (capacity < length) {
    *written = length;
    return C18_STATUS_BUFFER_TOO_SMALL;
  }
  if (length != 0 && output == NULL) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  for (size_t i = 0; i < length; ++i) {
    uint8_t byte = input[i];
    output[i] = (byte >= (uint8_t)'a' && byte <= (uint8_t)'z') ? (uint8_t)(byte - 32) : byte;
  }
  *written = length;
  return C18_STATUS_OK;
}

static c18_status C18_CALL process(c18_context* ctx,
                                   const uint8_t* input,
                                   size_t input_size,
                                   uint8_t* output,
                                   size_t output_capacity,
                                   size_t* written) {
  ++g_process_calls;
  if (ctx == NULL || written == NULL || (input == NULL && input_size != 0) ||
      (output == NULL && output_capacity != 0)) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  *written = input_size;
  if (ctx->closing) {
    return C18_STATUS_CLOSING;
  }
  if (output_capacity < input_size) {
    return transform_bytes(input, input_size, output, output_capacity, written);
  }
  for (size_t i = 0; i < input_size; ++i) {
    if (input[i] == (uint8_t)'!' && ctx->host.on_event != NULL) {
      ctx->host.on_event(ctx->host.userdata, "process:bang");
    }
  }
  return transform_bytes(input, input_size, output, output_capacity, written);
}

static c18_status C18_CALL request_stop(c18_context* ctx) {
  ++g_request_stop_calls;
  if (ctx == NULL) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  ctx->closing = true;
  return C18_STATUS_OK;
}

static c18_status C18_CALL destroy(c18_context* ctx) {
  ++g_destroy_calls;
  if (ctx == NULL) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  if (g_destroy_busy_once) {
    g_destroy_busy_once = false;
    return C18_STATUS_BUSY;
  }
  free(ctx);
  return C18_STATUS_OK;
}

C18_EXPORT c18_status C18_CALL c18_get_api(uint32_t requested_version,
                                           uint32_t host_struct_size,
                                           c18_api* out_api) {
  ++g_get_api_calls;
  if (out_api == NULL || host_struct_size < sizeof(c18_api)) {
    return C18_STATUS_BAD_ARGUMENT;
  }
  if (requested_version != C18_ABI_VERSION) {
    return C18_STATUS_UNSUPPORTED_VERSION;
  }
  out_api->version = C18_ABI_VERSION;
  out_api->struct_size = sizeof(c18_api);
  out_api->create = create_context;
  out_api->process = process;
  out_api->request_stop = request_stop;
  out_api->destroy = destroy;
  return C18_STATUS_OK;
}
