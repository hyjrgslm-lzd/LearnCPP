#include <check.hpp>
#include <expression_templates.hpp>

#include <cmath>
#include <iostream>
#include <type_traits>

template<std::size_t N>
void close(const c04_expr::vec<N>& got, const c04_expr::vec<N>& want, const char* message) {
    for (std::size_t i = 0; i < N; ++i) check(std::abs(got[i] - want[i]) < 1e-9, message);
}

int main() {
    const auto empty = c04_expr::eval(c04_expr::reverse(c04_expr::vec<0>{}));
    check(empty.data.empty(), "empty expression evaluation performs no element access");
    c04_expr::vec<3> a{{1.0, 2.0, 3.0}};
    c04_expr::vec<3> b{{4.0, 5.0, 6.0}};
    auto expr = a + b * 2.0 + c04_expr::reverse(c04_expr::vec<3>{{10.0, 20.0, 30.0}});
    close(c04_expr::eval(expr), c04_expr::vec<3>{{39.0, 32.0, 25.0}}, "nested temporary expression evaluates correctly");
    b[0] = 40.0;
    close(c04_expr::eval(a + b), c04_expr::vec<3>{{41.0, 7.0, 9.0}}, "lvalue operands are borrowed");
    auto owned = c04_expr::eval(c04_expr::vec<3>{{1.0, 1.0, 1.0}} + b);
    b[1] = 50.0;
    close(owned, c04_expr::vec<3>{{41.0, 6.0, 7.0}}, "eval returns an owning result");
    c04_expr::vec<4> r{{1.0, 2.0, 3.0, 4.0}};
    c04_expr::assign(r, c04_expr::reverse(r));
    close(r, c04_expr::vec<4>{{4.0, 3.0, 2.0, 1.0}}, "assign materializes before writing to handle reverse alias");
    std::cout << "A05 expression template checks OK\n";
}
