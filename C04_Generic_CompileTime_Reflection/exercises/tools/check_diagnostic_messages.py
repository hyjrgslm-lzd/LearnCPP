"""Small controls for semantic diagnostics versus incidental build output."""
from compile_case import diagnostic_messages, is_infrastructure_failure

assert diagnostic_messages('Note: including file: C:/constant/fmt.hpp\nconstant_error.cpp\n') == ''
assert diagnostic_messages('x.cpp(4,3): error C2131: expression is not constant [p.vcxproj]') == (
    'error C2131: expression is not constant')
assert diagnostic_messages('x.cpp:5:9: error: invalid format specifier') == (
    'error: invalid format specifier')
assert is_infrastructure_failure('error C1083: cannot open include file')
assert is_infrastructure_failure('fatal error C1083: cannot open include file')
assert is_infrastructure_failure('error C1001: internal compiler error')
assert not is_infrastructure_failure('error C7510: dependent type name requires typename')
assert not is_infrastructure_failure('error C2039: member is absent')
print('diagnostic message controls passed')
