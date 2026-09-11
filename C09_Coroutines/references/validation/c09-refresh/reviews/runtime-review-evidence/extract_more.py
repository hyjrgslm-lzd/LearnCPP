from pathlib import Path
f='C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp'
lines=Path(f).read_text(encoding='utf-8').splitlines()
for a,b in [(220,289),(533,640)]:
    print(f'--- {f}:{a}-{b} ---')
    for i in range(a, min(b,len(lines))+1): print(f'{i:04d}: {lines[i-1]}')
