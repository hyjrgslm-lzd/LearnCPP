template<class T>
concept broken_check = requires(T value) {
    missing_global_name();
};

struct item {};

int main() {
    static_assert(!broken_check<item>);
}
