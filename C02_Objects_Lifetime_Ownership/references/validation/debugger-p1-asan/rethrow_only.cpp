#include <stdexcept>

void rethrow_once()
{
    try {
        throw std::runtime_error("planned failure");
    } catch (...) {
        throw;
    }
}

int main()
{
    try {
        rethrow_once();
    } catch (const std::runtime_error&) {
        return 0;
    }
    return 1;
}
