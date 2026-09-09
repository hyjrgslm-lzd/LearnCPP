export module visibility_boundary;

struct hidden_state {
    int value;
};

export hidden_state make_hidden() {
    return hidden_state{42};
}

export int read_hidden(hidden_state const& state) {
    return state.value;
}
