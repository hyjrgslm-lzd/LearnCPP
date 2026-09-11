from pathlib import Path
for f, ranges in {
 'C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp': [(36,85),(129,170),(290,326)],
 'C09_Coroutines/exercises/include/coroutine_study/runtime.hpp': [(1,230)],
 'C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp': [(1,220),(290,540),(570,625)],
 'C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/stdexec_awaitable.hpp': [(1,170)],
}.items():
    lines=Path(f).read_text(encoding='utf-8').splitlines()
    print(f'### {f}')
    for a,b in ranges:
        print(f'--- {a}-{b} ---')
        for i in range(a, min(b, len(lines))+1):
            print(f'{i:04d}: {lines[i-1]}')
