from pathlib import Path
files = [
 'C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp',
 'C09_Coroutines/exercises/include/coroutine_study/runtime.hpp',
 'C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp',
 'C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/stdexec_awaitable.hpp',
]
for f in files:
    print(f'### {f}')
    for i,line in enumerate(Path(f).read_text(encoding='utf-8').splitlines(), 1):
        print(f'{i:04d}: {line}')
