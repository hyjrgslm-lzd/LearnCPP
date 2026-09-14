#pragma once

#include "../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cstring>
#include <stdexcept>

namespace c18_lua_l11 {

struct Args {
  std::vector<std::uint8_t>* input;
  Result* result;
};

inline int c_entry(lua_State* L) {
  auto* args = static_cast<Args*>(lua_touserdata(L, 1));
  auto& input = *args->input;
  auto& result = *args->result;
  result.top_before = lua_gettop(L);
  lua_pushlstring(L, input.empty() ? "" : reinterpret_cast<const char*>(input.data()), input.size());
  size_t length = 0;
  const char* bytes = lua_tolstring(L, lua_absindex(L, -1), &length);
  result.bytes.assign(reinterpret_cast<const std::uint8_t*>(bytes),
                      reinterpret_cast<const std::uint8_t*>(bytes) + length);
  for (auto& byte : result.bytes) {
    if (byte >= 'a' && byte <= 'z') {
      byte = static_cast<std::uint8_t>(byte - 32);
    }
  }
  lua_settop(L, result.top_before);
  result.top_after = lua_gettop(L);
  return 0;
}

inline Result transform(std::vector<std::uint8_t> input) {
  lua_State* L = luaL_newstate();
  if (L == nullptr) {
    throw std::runtime_error("luaL_newstate failed");
  }
  Result result;
  if (!lua_checkstack(L, 2)) {
    lua_close(L);
    throw std::runtime_error("lua_checkstack failed");
  }
  Args args{&input, &result};
  lua_pushcfunction(L, c_entry);
  lua_pushlightuserdata(L, &args);
  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    lua_close(L);
    throw std::runtime_error("protected stack exercise failed");
  }
  lua_close(L);
  return result;
}

}
