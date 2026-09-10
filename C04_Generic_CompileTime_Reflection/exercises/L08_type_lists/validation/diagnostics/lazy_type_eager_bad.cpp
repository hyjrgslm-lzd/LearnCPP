template<class T>
struct provider {
    using type = T;
};

struct explosive;

namespace c04 {
template<bool ChooseThen, class Then, class Else>
struct eager_lazy_type {
    using type = typename Else::missing_type;
};

template<bool ChooseThen, class Then, class Else>
using eager_lazy_type_t = typename eager_lazy_type<ChooseThen, Then, Else>::type;
} // namespace c04

using selected = c04::eager_lazy_type_t<true, provider<int>, explosive>;

int main() {
    return sizeof(selected);
}
