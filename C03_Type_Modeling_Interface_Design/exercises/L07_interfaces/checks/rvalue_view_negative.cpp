#include <owner.hpp>

int main()
{
    auto view = l07::Owner{1, 2}.view();
    return static_cast<int>(view.size());
}

