#include <explicit_object.hpp>

#include <memory>
#include <utility>

int main() {
    const c04_explicit::slot<std::unique_ptr<int>> slot{std::make_unique<int>(1)};
    auto moved = std::move(slot).take();
    return moved ? 0 : 1;
}
