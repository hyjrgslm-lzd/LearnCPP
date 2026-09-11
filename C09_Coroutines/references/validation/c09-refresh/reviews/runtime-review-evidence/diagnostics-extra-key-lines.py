from pathlib import Path
files = [
 'C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/as_awaitable_test.cpp',
 'C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/run_loop_test.cpp',
]
for f in files:
    print('###', f)
    for i,line in enumerate(Path(f).read_text(encoding='utf-8').splitlines(), 1):
        print(f'{i}:{line}')
