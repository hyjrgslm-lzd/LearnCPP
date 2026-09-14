#pragma once

#include "../../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <stdexcept>

namespace c18_lua_l12 {

struct Args {
  int fail;
  int cleanup_count;
};

inline int c_entry(lua_State* L) {
  auto* args = static_cast<Args*>(lua_touserdata(L, 1));
  ++args->cleanup_count;
  if (args->fail) {
    lua_pushliteral(L, "helper failed but caller swallowed it");
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
  int status = lua_pcall(L, 1, 1, 0);
  RunResult result{status, true, setup_checked, args.cleanup_count, 0, {}};
  if (status != LUA_OK) {
    size_t length = 0;
    const char* message = lua_type(L, -1) == LUA_TSTRING ? lua_tolstring(L, -1, &length) : nullptr;
    if (message != nullptr) {
      result.message.assign(message, length);
    }
    result.status = LUA_OK;
  }
  lua_settop(L, 0);
  result.top_after = lua_gettop(L);
  lua_close(L);
  return result;
}

}
