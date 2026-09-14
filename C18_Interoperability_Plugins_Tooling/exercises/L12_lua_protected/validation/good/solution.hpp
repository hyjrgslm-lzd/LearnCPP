#pragma once

#include "../../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <stdexcept>

namespace c18_lua_l12 {

struct PodError {
  int code;
  const char* message;
};

struct Args {
  int fail;
  int cleanup_count;
};

inline PodError do_cpp_work(Args* args) noexcept {
  ++args->cleanup_count;
  const int fail = args->fail;
  return fail ? PodError{1, "helper returned POD error"} : PodError{0, ""};
}

inline int c_entry(lua_State* L) {
  auto* args = static_cast<Args*>(lua_touserdata(L, 1));
  const PodError error = do_cpp_work(args);
  if (error.code != 0) {
    lua_pushstring(L, error.message);
    return lua_error(L);
  }
  lua_pushboolean(L, 1);
  return 1;
}

inline RunResult run_case(bool fail) {
  lua_State* L = luaL_newstate();
  if (L == nullptr) {
    throw std::runtime_error("luaL_newstate failed");
  }
  const bool setup_checked = lua_checkstack(L, 2) != 0;
  if (!setup_checked) {
    lua_close(L);
    return {LUA_ERRMEM, false, false, 0, 0, "lua_checkstack failed"};
  }
  Args args{fail ? 1 : 0, 0};
  lua_pushcfunction(L, c_entry);
  lua_pushlightuserdata(L, &args);
  const int status = lua_pcallk(L, 1, 1, 0, 0, nullptr);
  RunResult result{status, true, setup_checked, args.cleanup_count, 0, {}};
  if (status != LUA_OK) {
    size_t length = 0;
    const char* message = lua_type(L, -1) == LUA_TSTRING ? lua_tolstring(L, -1, &length) : nullptr;
    if (message != nullptr) {
      result.message.assign(message, length);
    }
  }
  lua_settop(L, 0);
  result.top_after = lua_gettop(L);
  lua_close(L);
  return result;
}

}
