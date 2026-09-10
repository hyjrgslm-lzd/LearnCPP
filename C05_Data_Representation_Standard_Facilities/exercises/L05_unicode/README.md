# L05 Unicode units

观察目标：byte、UTF-8 code unit、UTF-16 code unit、code point、scalar value 不是一回事。

Part 1：运行 `L05_unicode_observation`，观察欧元符号 U+20AC 是一个 Unicode scalar value，但在 UTF-8 中占 3 个 code unit，在 UTF-16 中占 1 个 code unit。

Part 2：观察 U+1F642 在 UTF-8 中占 4 字节，在 UTF-16 中是代理对，占 2 个 code unit。

Part 3：观察通用转码保留 BOM。解析重点：是否把开头 BOM 当签名剥掉，是上层文本格式规则；通用转换只做编码合法性和码元变换。

编辑位置：无，本题是观察题。检查目标：`L05_unicode_observation`。Reference：公共实现见 `../include/c05/utf.hpp`；解析在本 README。
