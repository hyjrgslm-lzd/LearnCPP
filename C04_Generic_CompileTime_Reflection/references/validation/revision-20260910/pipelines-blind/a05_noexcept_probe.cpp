#include <expression_templates.hpp>

int main()
{
    c04_expr::vec<3> a{{1.0, 2.0, 3.0}};
    c04_expr::vec<3> b{{4.0, 5.0, 6.0}};

    static_assert(noexcept(a[0]));
    static_assert(noexcept(c04_expr::reverse(a)));
    static_assert(noexcept(c04_expr::eval(a + b)));
    static_assert(noexcept(c04_expr::assign(a, b)));
}
