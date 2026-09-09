#include <check.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct counters {
    int first_alive = 0;
    int second_alive = 0;
    int next_id = 1;
    int throw_on_acquire = 0;
    std::vector<std::string> events;
};

counters model;

int acquire_first() {
    if (model.throw_on_acquire == 1) {
        model.events.push_back("throw first");
        throw std::runtime_error("first acquire failed");
    }
    const int id = model.next_id++;
    ++model.first_alive;
    model.events.push_back("acquire first " + std::to_string(id));
    return id;
}

int acquire_second() {
    if (model.throw_on_acquire == 2) {
        model.events.push_back("throw second");
        throw std::runtime_error("second acquire failed");
    }
    const int id = model.next_id++;
    ++model.second_alive;
    model.events.push_back("acquire second " + std::to_string(id));
    return id;
}

void release_first(int id) {
    --model.first_alive;
    model.events.push_back("release first " + std::to_string(id));
}

void release_second(int id) {
    --model.second_alive;
    model.events.push_back("release second " + std::to_string(id));
}

void reset_model(int throw_on_acquire = 0) {
    model = counters{};
    model.throw_on_acquire = throw_on_acquire;
}

void manual_success_baseline() {
    reset_model();
    const int first = acquire_first();
    const int second = acquire_second();
    release_second(second);
    release_first(first);

    check(model.first_alive == 0, "manual success releases first");
    check(model.second_alive == 0, "manual success releases second");
    check(model.events.size() == 4, "manual success event count");
    check(model.events[0] == "acquire first 1", "manual first acquire order");
    check(model.events[1] == "acquire second 2", "manual second acquire order");
    check(model.events[2] == "release second 2", "manual second release order");
    check(model.events[3] == "release first 1", "manual first release order");
}

void manual_partial_failure_leaks_first() {
    reset_model(2);
    bool threw = false;
    try {
        const int first = acquire_first();
        const int second = acquire_second();
        release_second(second);
        release_first(first);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "manual failure must throw");
    check(model.first_alive == 1, "manual failure leaves first alive");
    check(model.second_alive == 0, "manual failure never acquired second");
}

class first_owner {
public:
    explicit first_owner(int id) noexcept : id_(id) {}
    ~first_owner() { release_first(id_); }

    first_owner(const first_owner&) = delete;
    first_owner& operator=(const first_owner&) = delete;

private:
    int id_;
};

class second_owner {
public:
    explicit second_owner(int id) noexcept : id_(id) {}
    ~second_owner() { release_second(id_); }

    second_owner(const second_owner&) = delete;
    second_owner& operator=(const second_owner&) = delete;

private:
    int id_;
};

struct raii_pair {
    first_owner first;
    second_owner second;

    raii_pair()
        : first(acquire_first()),
          second(acquire_second()) {}
};

void raii_partial_failure_releases_first() {
    reset_model(2);
    bool threw = false;
    try {
        raii_pair pair;
        (void)pair;
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "raii failure must throw");
    check(model.first_alive == 0, "raii failure releases first");
    check(model.second_alive == 0, "raii failure leaves no second");
    check(model.events.size() == 3, "raii failure event count");
    check(model.events[0] == "acquire first 1", "raii first acquired");
    check(model.events[1] == "throw second", "raii second throws");
    check(model.events[2] == "release first 1", "raii first unwound");
}

} // namespace

int main() {
    manual_success_baseline();
    manual_partial_failure_leaks_first();
    raii_partial_failure_releases_first();
    std::cout << "L07_raii_observation OK\n";
}
