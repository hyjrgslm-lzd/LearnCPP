#include <expression_templates.hpp>
int main() {
    c04_expr::vec<3> a{{1, 2, 3}}, b{{4, 5, 6}};
    (void)c04_expr::eval(a + b);
}
