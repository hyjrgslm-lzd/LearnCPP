#include "lua_boundary.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <string.h>

typedef struct transform_args {
  const uint8_t* input;
  size_t input_size;
  uint8_t* output;
  size_t output_capacity;
  c18_lua_result* result;
} transform_args;

static void set_message(c18_lua_result* result, const char* message) {
  if (result == NULL) {
    return;
  }
  if (message == NULL) {
    message = "lua error";
  }
  strncpy(result->message, message, sizeof(result->message) - 1);
  result->message[sizeof(result->message) - 1] = '\0';
}

static int push_error(lua_State* L, transform_args* args, const char* message) {
  if (args != NULL && args->result != NULL) {
    args->result->status = C18_LUA_ERROR;
    set_message(args->result, message);
  }
  lua_pushstring(L, message);
  return lua_error(L);
}

static int transform_trampoline(lua_State* L) {
  transform_args* args = (transform_args*)lua_touserdata(L, 1);
  static const char script[] =
      "local s = ...\n"
      "local out = {}\n"
      "for i = 1, #s do\n"
      "  local b = string.byte(s, i)\n"
      "  if b >= 97 and b <= 122 then b = b - 32 end\n"
      "  out[i] = string.char(b)\n"
      "end\n"
      "return table.concat(out)\n";
  const char* bytes = NULL;
  size_t length = 0;
  int status = LUA_OK;

  if (args == NULL || args->result == NULL) {
    return push_error(L, args, "missing C transform arguments");
  }
  luaL_openlibs(L);
  if (args->input_size != 0 && args->input == NULL) {
    return push_error(L, args, "null input with nonzero length");
  }
  if (args->output_capacity < args->input_size) {
    args->result->status = C18_LUA_BUFFER_TOO_SMALL;
    args->result->written = args->input_size;
    return 0;
  }
  if (args->input_size != 0 && args->output == NULL) {
    return push_error(L, args, "null output with nonzero length");
  }

  status = luaL_loadbufferx(L, script, sizeof(script) - 1, "c18_lua_transform", "t");
  if (status != LUA_OK) {
    return lua_error(L);
  }
  lua_pushlstring(L, args->input_size == 0 ? "" : (const char*)args->input, args->input_size);
  status = lua_pcall(L, 1, 1, 0);
  if (status != LUA_OK) {
    return lua_error(L);
  }
  bytes = lua_tolstring(L, -1, &length);
  if (bytes == NULL) {
    return push_error(L, args, "transform did not return a Lua string");
  }
  if (length > args->output_capacity) {
    args->result->status = C18_LUA_BUFFER_TOO_SMALL;
    args->result->written = length;
    return 0;
  }
  if (length != 0) {
    memcpy(args->output, bytes, length);
  }
  args->result->status = C18_LUA_OK;
  args->result->written = length;
  return 0;
}

int c18_lua_transform_once(const uint8_t* input,
                           size_t input_size,
                           uint8_t* output,
                           size_t output_capacity,
                           c18_lua_result* result) {
  lua_State* L = NULL;
  transform_args args;
  int status = LUA_OK;

  if (result == NULL) {
    return C18_LUA_ERROR;
  }
  memset(result, 0, sizeof(*result));
  args.input = input;
  args.input_size = input_size;
  args.output = output;
  args.output_capacity = output_capacity;
  args.result = result;

  L = luaL_newstate();
  if (L == NULL) {
    result->status = C18_LUA_ERROR;
    set_message(result, "luaL_newstate failed");
    return result->status;
  }
  if (!lua_checkstack(L, 2)) {
    result->status = C18_LUA_ERROR;
    set_message(result, "lua_checkstack failed during protected bootstrap");
    lua_close(L);
    return result->status;
  }
  /* Lua 5.4 zero-upvalue C function and lightuserdata pushes do not allocate payload objects. */
  lua_pushcfunction(L, transform_trampoline);
  lua_pushlightuserdata(L, &args);
  status = lua_pcall(L, 1, 0, 0);
  if (status != LUA_OK && result->status == 0) {
    size_t len = 0;
    const char* message = lua_tolstring(L, -1, &len);
    (void)len;
    result->status = C18_LUA_ERROR;
    set_message(result, message);
  }
  lua_close(L);
  return result->status;
}
