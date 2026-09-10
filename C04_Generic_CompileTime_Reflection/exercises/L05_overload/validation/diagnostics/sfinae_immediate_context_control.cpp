struct no_size {};

template<class T>
auto probe(T& value) -> decltype(value.size(), int{}) {
    return 1;
}

int probe(...) {
    return 0;
}

int main() {
    no_size value{};
    return probe(value);
}
