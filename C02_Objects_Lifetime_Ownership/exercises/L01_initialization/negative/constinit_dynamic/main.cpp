int runtime_value();
constinit int value = runtime_value();

int runtime_value() {
    return 3;
}

int main() {
    return value;
}
