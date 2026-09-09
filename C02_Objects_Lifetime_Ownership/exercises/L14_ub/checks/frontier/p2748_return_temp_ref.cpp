const int& returns_temporary_reference() {
    return 42;
}

const int& returns_static_reference() {
    static const int value = 42;
    return value;
}

void baseline_use() {
    (void)returns_static_reference();
}
