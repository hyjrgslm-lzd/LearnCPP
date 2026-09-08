int definite_null_deref() {
    int* value = nullptr;
    return *value;
}
