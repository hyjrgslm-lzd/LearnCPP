struct Base {
    int base;
};

struct Derived : Base {
    int member;
};

constexpr Derived make_derived() {
    return Derived{.base = 1, .member = 2};
}

static_assert(make_derived().base == 1);
static_assert(make_derived().member == 2);
