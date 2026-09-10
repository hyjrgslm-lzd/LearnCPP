template<class T> struct provider { using type = T; };
struct explosive;
template<bool Choose, class Then, class Else>
struct eager {
    using then_type = typename Then::type;
    using else_type = typename Else::type;
    using type = then_type;
};
using bad = eager<true, provider<int>, explosive>::type;
int main() {}
