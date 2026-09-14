#pragma once

#include "../../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <stdexcept>

namespace c18_lua_l13 {

struct Handle {
  bool alive;
  int* closes;
  int* gc_calls;
};

struct Args {
  int closes;
  int gc_calls;
  bool metatable;
};

inline void close_once(Handle* handle) {
  if (handle != nullptr && handle->alive) {
    handle->alive = false;
    ++*handle->closes;
  }
}

inline int finalizer(lua_State* L) {
  auto* handle = static_cast<Handle*>(lua_touserdata(L, 1));
  if (handle != nullptr) {
    ++*handle->gc_calls;
  }
  close_once(handle);
  return 0;
}

inline GcResult exercise_userdata() {
  lua_State* L = luaL_newstate();
  if (L == nullptr) {
    throw std::runtime_error("luaL_newstate failed");
  }
  if (!lua_checkstack(L, 2)) {
    lua_close(L);
    throw std::runtime_error("lua_checkstack failed");
  }
  Args args{};
  lua_pushcfunction(L, [](lua_State* L) -> int {
    auto* args = static_cast<Args*>(lua_touserdata(L, 1));
    luaL_newmetatable(L, "c18.Handle");
    lua_pushcfunction(L, finalizer);
    lua_setfield(L, -2, "__gc");
    auto* handle = static_cast<Handle*>(lua_newuserdatauv(L, sizeof(Handle), 1));
    *handle = Handle{true, &args->closes, &args->gc_calls};
    lua_pushvalue(L, -2);
    args->metatable = lua_setmetatable(L, -2) != 0;
    close_once(handle);
    lua_pop(L, 2);
    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_gc(L, LUA_GCCOLLECT, 0);
    return 0;
  });
  lua_pushlightuserdata(L, &args);
  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    lua_close(L);
    throw std::runtime_error("protected userdata exercise failed");
  }
  lua_close(L);
  return {args.closes, args.gc_calls, args.metatable};
}

}
