# L04 strings and views

观察目标：`std::string` 拥有字节，`std::string_view` 借用字节。函数可以收 `string_view`，但如果结果要离开函数，就返回拥有型 `std::string`。

Part 1：运行 `L04_strings_views_observation`，观察 view 指向 owner 的当前字节；owner 修改后 view 读到新内容。

Part 2：观察函数返回的 `std::string` 不跟随 owner 修改。解析重点：入参可借用，返回跨过调用边界时应拥有。

Part 3：观察 `char8_t` 到字节级 UTF-8 parser 的显式桥接。`reinterpret_cast` 只在这里建立 byte view，不改变文本合法性，合法性由 L06 检查。

编辑位置：无，本题是观察题。检查目标：`L04_strings_views_observation`。Reference：观察题没有独立 Reference；解析在本 README。
