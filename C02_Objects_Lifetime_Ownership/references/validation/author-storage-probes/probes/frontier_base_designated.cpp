struct Base {
    int base;
};

struct Derived : Base {
    int member;
};

int main()
{
    Derived value{.base = 1, .member = 2};
    return value.base == 1 && value.member == 2 ? 0 : 1;
}
