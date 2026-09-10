consteval int broken(int value) {
    static_assert(value >= 0);
    return value;
}
int main() { return broken(0); }
