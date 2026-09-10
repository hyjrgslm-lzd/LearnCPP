template<class T>
struct box {
    using value_type = int;
};

template<class T>
box<T>::value_type make_value() {
    box<T>::value_type value{};
    return value;
}

int main() {
    return make_value<int>();
}
