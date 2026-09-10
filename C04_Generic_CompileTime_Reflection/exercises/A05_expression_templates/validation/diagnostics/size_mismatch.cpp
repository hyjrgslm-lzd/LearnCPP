#include <expression_templates.hpp>
int main() {
    c04_expr::vec<2> a{{1, 2}};
    c04_expr::vec<3> b{{1, 2, 3}};
    (void)c04_expr::eval(a + b);
}
