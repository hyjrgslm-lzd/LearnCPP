#include "twice.hpp"

int main() {
    return l01_extern::twice<int>(2) == 4 ? 0 : 1;
}
