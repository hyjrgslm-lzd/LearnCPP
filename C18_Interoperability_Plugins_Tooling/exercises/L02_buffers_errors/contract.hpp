#pragma once
#include <cstddef>
enum class Status { ok, invalid, too_small, unfinished };
struct Result { Status status; std::size_t written; };
