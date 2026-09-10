template<class T>
concept safe_check = requires(T value) {
    value.valid();
};

struct item {
    void valid() {}
};

int main() {
    static_assert(safe_check<item>);
}
