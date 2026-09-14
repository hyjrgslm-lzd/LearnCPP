#pragma once

#include "../../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <stdexcept>

namespace c18_lua_l13 {

struct Box {
  int* closes;
  int* gc_calls;
};

struct Args {
  int closes;
  int gc_calls;
  bool metatable;
};

inline int gc_box(lua_State* L) {
  auto* box = static_cast<Box*>(lua_touserdata(L, 1));
  if (box != nullptr) {
    ++*box->gc_calls;
    ++*box->closes;
  }
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
    luaL_newmetatable(L, "c18.BadBox");
    lua_pushcfunction(L, gc_box);
    lua_setfield(L, -2, "__gc");
    auto* box = static_cast<Box*>(lua_newuserdatauv(L, sizeof(Box), 0));
    *box = Box{&args->closes, &args->gc_calls};
    lua_pushvalue(L, -2);
    args->metatable = lua_setmetatable(L, -2) != 0;
    ++args->closes;
    lua_pop(L, 2);
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
