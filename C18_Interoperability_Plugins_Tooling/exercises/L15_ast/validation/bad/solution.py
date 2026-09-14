def source() -> str:
    return r"""
struct Api {
  int (*process)(int);
};

struct Other {
  int (*process)(int);
};

static int one(int value) {
  return value + 1;
}

int run(Api&, Other& other) {
  other.process = one;
  return other.process(4);
}
"""
