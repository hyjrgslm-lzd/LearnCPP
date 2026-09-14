def source() -> str:
    return r"""
struct Api {
  int (*process)(int);
};

int run(Api&) {
  return 0;
}
"""
