template<class T>
int classify(T) {
    return 0;
}

template<class T>
int classify(T*) {
    return 1;
}

int main() {
    int value = 0;
    return classify(&value);
}
