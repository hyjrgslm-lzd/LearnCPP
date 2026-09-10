template<class T>
struct box {
    using value_type = int;
};

template<class T>
typename box<T>::value_type make_value() {
    typename box<T>::value_type value{};
    return value;
}

int main() {
    return make_value<int>();
}
