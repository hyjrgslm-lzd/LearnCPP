struct Token {
    Token() = default;
    Token(Token const&) = delete;
    Token(Token&&) = delete;
};

Token make_prvalue() {
    return Token{};
}

Token make_named() {
    Token local;
    return local;
}

int main() {
    auto token = make_prvalue();
    (void)token;
}
