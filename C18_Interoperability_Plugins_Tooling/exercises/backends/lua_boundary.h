#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  C18_LUA_OK = 0,
  C18_LUA_ERROR = 1,
  C18_LUA_BUFFER_TOO_SMALL = 2
};

typedef struct c18_lua_result {
  int status;
  size_t written;
  char message[160];
} c18_lua_result;

int c18_lua_transform_once(const uint8_t* input,
                           size_t input_size,
                           uint8_t* output,
                           size_t output_capacity,
                           c18_lua_result* result);

#ifdef __cplusplus
}
#endif
