from pathlib import Path
for rel in ['C09_Coroutines/exercises/G1_shared_task/main.cpp','C09_Coroutines/exercises/G1_shared_task/solution.cpp','C09_Coroutines/exercises/G1_shared_task/README.md','C09_Coroutines/exercises/G1_shared_task/CMakeLists.txt']:
    print(f'FILE {rel}')
    for i,line in enumerate(Path(rel).read_text(encoding='utf-8').splitlines(),1):
        print(f'{i:4}: {line}')
