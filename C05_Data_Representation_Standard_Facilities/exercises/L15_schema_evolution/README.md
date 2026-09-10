# L15 schema evolution

目标：实现 Manifest 的最小 schema 演进。学生编辑 `src/student/schema_evolution.hpp`，提供 `c05_ex::validate_manifest`、`c05_ex::encode_manifest`、`c05_ex::decode_manifest`。

Part 1：验证内存对象。最多 1024 条记录；id 非零且唯一；name/path 非空、严格 UTF-8、不含 NUL；note 可空但 present note 也要严格 UTF-8 且不含 NUL；timestamp 必须在 `c05::min_timestamp_ms..c05::max_timestamp_ms`。path 规则必须调用公共 `c05::validate_resource_path`，缺少该头就是集成错误，不允许默默通过。

Part 2：写 wire header。固定 `C05M | major:u16 | minor:u16 | record_count:u32`，整数 big endian。只支持 `WireVersion::v1` 和 `WireVersion::v2`；其他枚举值返回 `unsupported_version`。

Part 3：写 record body。字段为 `tag:u16 | length:u32 | payload`。tags 1-5 是 id/name/path/size/mtime；tag 6 是 v2 optional note。v1 writer 遇到 present note 必须拒绝，空 note 也算 present。

Part 4：读 wire。接受 `major == 1 && minor >= 1`，拒绝 minor 0 和未知 major。每个 record 用 body length 限界；known tag 拒绝重复；unknown tag 也拒绝重复；未知字段只做有界 skip。缺必需字段、标量长度错误、尾随包数据都失败。

Part 5：兼容实验。v2 reader 读 v1 后 note 是 absent；独立 v1 reader 读 v2 后跳过 tag 6；future minor unknown 字段可跳过；re-encode 不保留 unknown 字段，这是有意信息丢失。

检查目标：`L15_schema_evolution_reference`、`L15_schema_evolution_validation_good`、`L15_schema_evolution_validation_bad_rejected`；开启 student preset 后学生占位应失败。Reference 在 `src/reference/schema_evolution.hpp`，公共实现为 `../include/c05/manifest.hpp`。`validation/bad` 只犯一个 schema 错误：v1 写出时静默丢 note，检查器用 `v1 writer rejects note` 拒绝。

解析正文见 [15：schema 演进不是多加一个字段](../../chapters/15-schema-evolution.md)。黄金输入来自 `../fixtures/golden.hpp`，由手写 octet 预检，不由 writer 生成 expected。
