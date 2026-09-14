def source() -> str:
    return r"""
struct Api {
  int (*process)(int);
};

static int twice(int value) {
  return value * 2;
}

int use(Api* api) {
  api->process = twice;
  return api->process(21);
}
"""
