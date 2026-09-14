#pragma once

#include "../../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <stdexcept>
#include <thread>

namespace c18_lua_l14 {

inline int after_yield(lua_State* L, int, lua_KContext ctx) {
  auto* result = reinterpret_cast<CoroutineResult*>(ctx);
  result->continuation_seen = true;
  lua_pushinteger(L, 3);
  return 1;
}

inline int c_pause(lua_State* L) {
  auto* result = static_cast<CoroutineResult*>(lua_touserdata(L, lua_upvalueindex(1)));
  lua_pushinteger(L, 2);
  return lua_yieldk(L, 1, reinterpret_cast<lua_KContext>(result), after_yield);
}

struct Args {
  lua_State* co;
  CoroutineResult* result;
};

struct Setup {
  CoroutineResult* result;
  lua_State* co;
  int ref;
};

inline int setup_entry(lua_State* L) {
  auto* setup = static_cast<Setup*>(lua_touserdata(L, 1));
  lua_pushlightuserdata(L, setup->result);
  lua_pushcclosure(L, c_pause, 1);
  lua_setglobal(L, "c_pause");
  setup->co = lua_newthread(L);
  setup->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  if (luaL_loadstring(setup->co, "coroutine.yield(1); return c_pause()") != LUA_OK) {
    lua_pushliteral(L, "load failed");
    return lua_error(L);
  }
  return 0;
}

inline bool checked_resume(Args& args, std::thread::id owner) {
  if (std::this_thread::get_id() != owner) {
    args.result->resumed_on_foreign_thread = true;
    args.result->last_resume_thread = std::this_thread::get_id();
    return false;
  }
  int nres = 0;
  const int status = lua_resume(args.co, nullptr, 0, &nres);
  if (nres == 1) {
    args.result->yielded.push_back(static_cast<int>(lua_tointeger(args.co, -1)));
    lua_pop(args.co, 1);
  }
  args.result->final_status = status == LUA_OK ? 0 : status;
  args.result->last_resume_thread = std::this_thread::get_id();
  return true;
}

inline CoroutineResult run_coroutine() {
  lua_State* L = luaL_newstate();
  if (L == nullptr) {
    throw std::runtime_error("luaL_newstate failed");
  }
  CoroutineResult result;
  result.owner_thread = std::this_thread::get_id();
  if (!lua_checkstack(L, 2)) {
    lua_close(L);
    throw std::runtime_error("lua_checkstack failed");
  }
  Setup setup{&result, nullptr, LUA_NOREF};
  lua_pushcfunction(L, setup_entry);
  lua_pushlightuserdata(L, &setup);
  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    lua_close(L);
    throw std::runtime_error("protected coroutine setup failed");
  }
  Args args{setup.co, &result};
  checked_resume(args, result.owner_thread);
  std::thread worker([&] {
    result.worker_thread = std::this_thread::get_id();
    result.queued_from_worker = true;
    (void)checked_resume(args, result.owner_thread);
  });
  worker.join();
  checked_resume(args, result.owner_thread);
  checked_resume(args, result.owner_thread);
  luaL_unref(L, LUA_REGISTRYINDEX, setup.ref);
  lua_close(L);
  return result;
}

}
