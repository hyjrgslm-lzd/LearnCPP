$ErrorActionPreference = 'Stop'

$chapterPath = 'C06_Ranges/11-模块H-高级实现模式.md'
$readmePath = 'C06_Ranges/exercises/H3_generator_const_iter/README.md'

$chapter = Get-Content -LiteralPath $chapterPath -Raw
$readme = Get-Content -LiteralPath $readmePath -Raw

$required = @(
    @{ file = $chapterPath; text = 'std::common_reference_t<const value_type&&, std::iter_reference_t<I>>' },
    @{ file = $chapterPath; text = 'vector<int>::iterator：iter_reference_t = int&，结果是 const int&' },
    @{ file = $chapterPath; text = 'vector<bool>::iterator：iter_reference_t 是可写 proxy，结果是 bool，因此包装后不可写' },
    @{ file = $chapterPath; text = 'move_iterator<vector<int>::iterator>：iter_reference_t = int&&，结果是 const int&&' },
    @{ file = $chapterPath; text = '这个教学 wrapper 不是 std::basic_const_iterator 的完整复刻' },
    @{ file = $readmePath; text = 'std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>' },
    @{ file = $readmePath; text = '典型结果：`vector<int>` 为 `const int&`，`vector<bool>` 为不可写 `bool`，`move_iterator<vector<int>::iterator>` 为 `const int&&`' },
    @{ file = $readmePath; text = '教学 wrapper 不实现 `std::basic_const_iterator` 的全部成员' }
)

$missing = foreach ($item in $required) {
    $text = if ($item.file -eq $chapterPath) { $chapter } else { $readme }
    if (-not $text.Contains($item.text)) {
        [ordered]@{ file = $item.file; missing = $item.text }
    }
}

$oldAliasPattern = 'std::conditional_t<\s*[\r\n\s]*std::is_reference_v<std::iter_reference_t<I>>\s*,\s*[\r\n\s]*std::common_reference_t<const value_type&&,\s*std::iter_reference_t<I>>\s*,\s*[\r\n\s]*std::iter_reference_t<I>>'
$oldAliasPresent = [regex]::IsMatch($chapter, $oldAliasPattern)

$result = [ordered]@{
    required_count = $required.Count
    missing = @($missing)
    old_conditional_alias_present = $oldAliasPresent
}

$result | ConvertTo-Json -Depth 4

if (@($missing).Count -ne 0 -or $oldAliasPresent) {
    exit 1
}
