from pathlib import Path
items = [
('C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp', [(36,50),(290,326)]),
('C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp', [(180,224),(290,402)]),
('C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/shared_task_abandon_test.cpp', [(1,220)]),
('C09_Coroutines/exercises/Capstone5_mini_corolib/CMakeLists.txt', [(70,90)]),
]
for f,ranges in items:
    lines=Path(f).read_text(encoding='utf-8').splitlines()
    print('###', f)
    for a,b in ranges:
        print(f'--- {a}-{b} ---')
        for i in range(a,min(b,len(lines))+1): print(f'{i}:{lines[i-1]}')
        print()
