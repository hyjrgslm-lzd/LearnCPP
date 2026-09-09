#include <expected>

enum class Error {
    missing,
};

int main()
{
    int value = 1;
    std::expected<int&, Error> borrowed = value;
    return *borrowed;
}
