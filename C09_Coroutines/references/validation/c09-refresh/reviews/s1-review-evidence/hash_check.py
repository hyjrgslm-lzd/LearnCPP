from pathlib import Path
import hashlib
for p in [Path('C09_Coroutines/exercises/runtime_tests/await_resume_exception_test.cpp'), Path('C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp')]:
    b=p.read_bytes()
    lf=b.replace(b'\r\n', b'\n')
    crlf=lf.replace(b'\n', b'\r\n')
    print(p)
    print('raw ', hashlib.sha256(b).hexdigest())
    print('lf  ', hashlib.sha256(lf).hexdigest())
    print('crlf', hashlib.sha256(crlf).hexdigest())
