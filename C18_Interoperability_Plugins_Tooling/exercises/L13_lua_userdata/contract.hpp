#pragma once

namespace c18_lua_l13 {

struct GcResult {
  int closes{};
  int gc_calls{};
  bool metatable{};
};

GcResult exercise_userdata();

}
