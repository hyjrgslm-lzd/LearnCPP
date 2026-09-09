import h1_fragments;

#include <check.hpp>

int main() {
    check(fragment_answer() == 42, "global/private fragment module changed behavior");
}
