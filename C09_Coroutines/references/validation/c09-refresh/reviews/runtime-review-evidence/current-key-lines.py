from pathlib import Path
items = [
('C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp', [(36,49),(70,78),(290,307),(310,326)]),
('C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp', [(42,53),(180,197),(203,224),(291,350),(390,395),(445,525),(572,638)]),
]
for f,ranges in items:
    lines=Path(f).read_text(encoding='utf-8').splitlines()
    print('###', f)
    for a,b in ranges:
        for i in range(a,min(b,len(lines))+1): print(f'{i}:{lines[i-1]}')
        print()
