#include <stdexcept>

void throw_once()
{
    throw std::runtime_error("planned failure");
}

int main()
{
    try {
        throw_once();
    } catch (const std::runtime_error&) {
        return 0;
    }
    return 1;
}
