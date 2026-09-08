#include "check.hpp"

int main(int argc, char**) {
    check(argc == 1, "intentional check self-test failure");
    return 0;
}
