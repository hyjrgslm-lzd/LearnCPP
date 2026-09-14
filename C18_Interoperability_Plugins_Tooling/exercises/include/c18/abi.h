#ifndef C18_ABI_H
#define C18_ABI_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#  ifdef C18_PLUGIN_BUILD
#    define C18_EXPORT __declspec(dllexport)
#  else
#    define C18_EXPORT
#  endif
#  define C18_CALL __cdecl
#else
#  ifdef C18_PLUGIN_BUILD
#    define C18_EXPORT __attribute__((visibility("default")))
#  else
#    define C18_EXPORT
#  endif
#  define C18_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define C18_ABI_VERSION 1u

typedef struct c18_context c18_context;

typedef uint32_t c18_status;
enum {
  C18_STATUS_OK = 0u,
  C18_STATUS_BAD_ARGUMENT = 1u,
  C18_STATUS_UNSUPPORTED_VERSION = 2u,
  C18_STATUS_BUFFER_TOO_SMALL = 3u,
  C18_STATUS_CLOSING = 4u,
  C18_STATUS_BUSY = 5u,
  C18_STATUS_PLUGIN_ERROR = 6u,
  C18_STATUS_MISSING_SYMBOL = 7u
};

typedef void(C18_CALL *c18_host_event_fn)(void* userdata, const char* event_name);

typedef struct c18_host_api {
  uint32_t version;
  uint32_t struct_size;
  void* userdata;
  c18_host_event_fn on_event;
} c18_host_api;

typedef c18_status(C18_CALL *c18_create_fn)(const c18_host_api* host, c18_context** out);
typedef c18_status(C18_CALL *c18_process_fn)(c18_context* ctx,
                                             const uint8_t* input,
                                             size_t input_size,
                                             uint8_t* output,
                                             size_t output_capacity,
                                             size_t* written);
typedef c18_status(C18_CALL *c18_request_stop_fn)(c18_context* ctx);
typedef c18_status(C18_CALL *c18_destroy_fn)(c18_context* ctx);

typedef struct c18_api {
  uint32_t version;
  uint32_t struct_size;
  c18_create_fn create;
  c18_process_fn process;
  c18_request_stop_fn request_stop;
  c18_destroy_fn destroy;
} c18_api;

typedef c18_status(C18_CALL *c18_get_api_fn)(uint32_t requested_version,
                                             uint32_t host_struct_size,
                                             c18_api* out_api);

C18_EXPORT c18_status C18_CALL c18_get_api(uint32_t requested_version,
                                           uint32_t host_struct_size,
                                           c18_api* out_api);

#ifdef __cplusplus
}
#endif

#endif
