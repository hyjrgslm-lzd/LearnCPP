#include <iostream>
#include <stdexcept>
#include <vector>

std::vector<const char*> events;

struct Member {
    Member() { events.push_back("Member()"); }
    ~Member() { events.push_back("~Member()"); }
};

struct Delegating {
    Member member;
    explicit Delegating(int) { events.push_back("target body"); }
    Delegating() : Delegating(1) {
        events.push_back("delegating body");
        throw std::runtime_error("delegating body failed");
    }
    ~Delegating() { events.push_back("~Delegating()"); }
};

int main() {
    try {
        Delegating value;
        (void)value;
    } catch (const std::runtime_error&) {
        events.push_back("caught");
    }

    for (auto event : events) {
        std::cout << event << '\n';
    }
}
