#pragma once

#include "../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <stdexcept>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace c18_lua_l14 {

inline int continuation(lua_State* L, int, lua_KContext ctx) {
  auto* result = reinterpret_cast<CoroutineResult*>(ctx);
  result->continuation_seen = true;
  lua_pushinteger(L, 3);
  return 1;
}

inline int c_yield(lua_State* L) {
  auto* result = static_cast<CoroutineResult*>(lua_touserdata(L, lua_upvalueindex(1)));
  lua_pushinteger(L, 2);
  return lua_yieldk(L, 1, reinterpret_cast<lua_KContext>(result), continuation);
}

class OwnerQueue {
public:
  void post(void (*fn)(void*), void* arg) {
    std::scoped_lock guard(lock_);
    jobs_.push({fn, arg});
  }

  void drain() {
    for (;;) {
      Job job{};
      {
        std::scoped_lock guard(lock_);
        if (jobs_.empty()) {
          return;
        }
        job = jobs_.front();
        jobs_.pop();
      }
      job.fn(job.arg);
    }
  }

private:
  struct Job {
    void (*fn)(void*);
    void* arg;
  };
  std::mutex lock_;
  std::queue<Job> jobs_;
};

struct ResumeArgs {
  lua_State* co;
  CoroutineResult* result;
};

struct SetupArgs {
  CoroutineResult* result;
  lua_State* co;
  int ref;
};

inline int setup_entry(lua_State* L) {
  auto* args = static_cast<SetupArgs*>(lua_touserdata(L, 1));
  lua_pushlightuserdata(L, args->result);
  lua_pushcclosure(L, c_yield, 1);
  lua_setglobal(L, "c_yield");
  args->co = lua_newthread(L);
  args->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  if (luaL_loadstring(args->co, "coroutine.yield(1); return c_yield()") != LUA_OK) {
    lua_pushliteral(L, "failed to load coroutine chunk");
    return lua_error(L);
  }
  return 0;
}

inline void resume_once(void* raw) {
  auto& args = *static_cast<ResumeArgs*>(raw);
  lua_State* co = args.co;
  CoroutineResult& result = *args.result;
  result.last_resume_thread = std::this_thread::get_id();
  int nres = 0;
  const int status = lua_resume(co, nullptr, 0, &nres);
  if (status != LUA_OK && status != LUA_YIELD) {
    result.final_status = status;
    return;
  }
  if (nres == 1) {
    result.yielded.push_back(static_cast<int>(lua_tointeger(co, -1)));
    lua_pop(co, 1);
  }
  result.final_status = status == LUA_OK ? 0 : LUA_YIELD;
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
  SetupArgs setup{&result, nullptr, LUA_NOREF};
  lua_pushcfunction(L, setup_entry);
  lua_pushlightuserdata(L, &setup);
  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    lua_close(L);
    throw std::runtime_error("protected coroutine setup failed");
  }
  ResumeArgs args{setup.co, &result};
  resume_once(&args);
  OwnerQueue queue;
  std::thread worker([&] {
    result.worker_thread = std::this_thread::get_id();
    result.queued_from_worker = true;
    queue.post(resume_once, &args);
  });
  worker.join();
  queue.drain();
  resume_once(&args);
  luaL_unref(L, LUA_REGISTRYINDEX, setup.ref);
  lua_close(L);
  return result;
}

}
