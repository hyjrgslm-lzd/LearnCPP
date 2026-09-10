template<int Value>
consteval int checked() {
    static_assert(Value >= 0);
    return Value;
}
static_assert(checked<7>() == 7);
int main() { return checked<0>(); }
