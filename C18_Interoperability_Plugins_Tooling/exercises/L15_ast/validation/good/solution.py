def source() -> str:
    return r"""
struct Api {
  int (*process)(int);
};

int increment(int value) {
  return value + 1;
}

int run(Api& api) {
  api.process = increment;
  return api.process(4);
}
"""
