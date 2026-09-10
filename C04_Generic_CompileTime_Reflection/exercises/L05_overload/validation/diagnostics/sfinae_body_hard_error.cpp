struct no_size {};

template<class T>
int probe(T& value) {
    return value.size();
}

int main() {
    no_size value{};
    return probe(value);
}
