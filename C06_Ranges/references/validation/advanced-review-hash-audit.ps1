$paths = @(
    "C06_Ranges\exercises\CAPSTONE4_mini_ranges\src\reference\my_ranges\*.hpp",
    "C06_Ranges\exercises\CAPSTONE4_mini_ranges\validation\good\my_ranges\*.hpp",
    "C06_Ranges\exercises\CAPSTONE4_mini_ranges\validation\bad\my_ranges\*.hpp",
    "C06_Ranges\exercises\CAPSTONE4_mini_ranges\src\student\my_ranges\*.hpp",
    "C06_Ranges\exercises\H1_non_propagating_cache\src\reference\filter_cache.hpp",
    "C06_Ranges\exercises\H1_non_propagating_cache\validation\good\filter_cache.hpp",
    "C06_Ranges\exercises\H2_common_iter_proxy\src\reference\common_proxy.hpp",
    "C06_Ranges\exercises\H2_common_iter_proxy\validation\good\common_proxy.hpp",
    "C06_Ranges\exercises\H3_generator_const_iter\src\reference\generator_const_iter.hpp",
    "C06_Ranges\exercises\H3_generator_const_iter\validation\good\generator_const_iter.hpp"
)

Get-ChildItem $paths | Sort-Object FullName | Get-FileHash | ForEach-Object {
    "$($_.Path) $($_.Hash)"
}
