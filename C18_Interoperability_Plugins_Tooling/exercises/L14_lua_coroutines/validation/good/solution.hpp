#pragma once

#include "../../contract.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <stdexcept>
#include <mutex>
#include <queue>
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

struct Job {
  void (*fn)(void*);
  void* arg;
};

class Queue {
public:
  void push(Job job) {
    std::scoped_lock guard(lock_);
    jobs_.push(job);
  }

  void run() {
    while (true) {
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
  std::mutex lock_;
  std::queue<Job> jobs_;
};

struct Args {
  lua_State* co;
  CoroutineResult* out;
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

inline void resume_and_take(void* raw) {
  auto& args = *static_cast<Args*>(raw);
  lua_State* co = args.co;
  CoroutineResult& out = *args.out;
  out.last_resume_thread = std::this_thread::get_id();
  int nres = 0;
  const int status = lua_resume(co, nullptr, 0, &nres);
  if (nres == 1) {
    out.yielded.push_back(static_cast<int>(lua_tointeger(co, -1)));
    lua_pop(co, 1);
  }
  out.final_status = status;
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
  Queue queue;
  resume_and_take(&args);
  std::thread worker([&] {
    result.worker_thread = std::this_thread::get_id();
    result.queued_from_worker = true;
    queue.push({resume_and_take, &args});
  });
  worker.join();
  queue.run();
  resume_and_take(&args);
  result.final_status = result.final_status == LUA_OK ? 0 : result.final_status;
  luaL_unref(L, LUA_REGISTRYINDEX, setup.ref);
  lua_close(L);
  return result;
}

}
