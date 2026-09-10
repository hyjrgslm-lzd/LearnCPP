#include <explicit_object.hpp>

#include <memory>

int main() {
    c04_explicit::slot<std::unique_ptr<int>> slot{std::make_unique<int>(1)};
    return *std::move(slot).take() == 1 ? 0 : 1;
}
