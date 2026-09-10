#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <span>

namespace {
struct handle {
    bool owns = false;
    bool protects = false;
    friend bool operator==(const handle&, const handle&) = default;
};

void make_batch(std::span<handle> handles, int fail_after = -1) {
    std::array<handle, 4> staged{};
    cs::check(handles.size() <= staged.size(), "model capacity covers this exercise");
    for (std::size_t i = 0; i < handles.size(); ++i) staged[i] = handles[i];
    int constructed = 0;
    for (std::size_t i = 0; i < handles.size(); ++i) {
        if (staged[i].owns) continue;
        if (fail_after >= 0 && constructed == fail_after) throw std::bad_alloc();
        staged[i] = {.owns = true, .protects = false};
        ++constructed;
    }
    for (std::size_t i = 0; i < handles.size(); ++i) handles[i] = staged[i];
}

void clear_batch(std::span<handle> handles) noexcept {
    for (auto& h : handles) {
        h.owns = false;
        h.protects = false;
    }
}
} // namespace

int main() try {
    std::cout << "OBSERVATION: teaching model for C++29 hazard pointer batches; native body is in solution.cpp\n";
    std::array<handle, 3> handles{{{true, true}, {false, false}, {true, true}}};
    make_batch(std::span<handle>{});
    cs::check(handles[0].owns && handles[0].protects, "empty make is no-op");

    const auto before = handles;
    bool threw = false;
    try {
        make_batch(handles, 0);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    cs::check(threw && handles == before, "make batch keeps strong exception guarantee");

    make_batch(handles);
    cs::check(handles[0].protects && handles[2].protects, "make leaves existing nonempty handles unchanged");
    clear_batch(handles);
    clear_batch(handles);
    cs::check(!handles[0].owns && !handles[1].owns && !handles[2].owns, "clear destroys owned hazard pointers");
    cs::check(!handles[0].protects && !handles[1].protects && !handles[2].protects, "clear leaves all elements empty");
    std::cout << "F02 model OK: empty/mixed/make-preserves-nonempty/strong-exception/repeated-clear-destroys-handles\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
