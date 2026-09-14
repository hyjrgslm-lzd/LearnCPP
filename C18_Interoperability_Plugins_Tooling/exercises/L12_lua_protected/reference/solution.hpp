#pragma once

#include "../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <exception>
#include <stdexcept>

namespace c18_lua_l12 {

struct HelperResult {
  int ok;
  const char* message;
};

struct CallArgs {
  int fail;
  int cleanup_count;
};

inline HelperResult helper(int fail, CallArgs* args) noexcept {
  try {
    if (fail) {
      throw std::runtime_error("helper failed after local cleanup");
    }
    ++args->cleanup_count;
    return {1, ""};
  } catch (...) {
    ++args->cleanup_count;
    return {0, "helper failed after local cleanup"};
  }
}

inline int trampoline(lua_State* L) {
  auto* args = static_cast<CallArgs*>(lua_touserdata(L, 1));
  const HelperResult result = helper(args->fail, args);
  if (!result.ok) {
    lua_pushstring(L, result.message);
    return lua_error(L);
  }
  lua_pushliteral(L, "ok");
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
  CallArgs args{fail ? 1 : 0, 0};
  lua_pushcfunction(L, trampoline);
  lua_pushlightuserdata(L, &args);
  const int status = lua_pcall(L, 1, 1, 0);
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
