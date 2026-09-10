#include <schema_evolution.hpp>

#include "../../fixtures/golden.hpp"
#include "../provided/v1_reader.hpp"

#include <check.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <vector>

static std::vector<std::byte> bytes(std::span<const unsigned char> input)
{
    std::vector<std::byte> out;
    out.reserve(input.size());
    for (auto b : input) {
        out.push_back(static_cast<std::byte>(b));
    }
    return out;
}

template<std::size_t N>
static std::vector<std::byte> bytes(const std::array<unsigned char, N>& input)
{
    return bytes(std::span<const unsigned char>(input));
}

static std::vector<unsigned char> octets(std::span<const std::byte> input)
{
    std::vector<unsigned char> out;
    out.reserve(input.size());
    for (auto b : input) {
        out.push_back(std::to_integer<unsigned char>(b));
    }
    return out;
}

static void put_u16(std::vector<std::byte>& v, std::size_t pos, std::uint16_t x)
{
    v[pos] = static_cast<std::byte>((x >> 8) & 0xff);
    v[pos + 1] = static_cast<std::byte>(x & 0xff);
}

static void put_u32(std::vector<std::byte>& v, std::size_t pos, std::uint32_t x)
{
    v[pos] = static_cast<std::byte>((x >> 24) & 0xff);
    v[pos + 1] = static_cast<std::byte>((x >> 16) & 0xff);
    v[pos + 2] = static_cast<std::byte>((x >> 8) & 0xff);
    v[pos + 3] = static_cast<std::byte>(x & 0xff);
}

static void insert_raw_field(std::vector<std::byte>& v, std::uint16_t tag, std::initializer_list<unsigned char> payload)
{
    std::vector<std::byte> field;
    field.push_back(static_cast<std::byte>((tag >> 8) & 0xff));
    field.push_back(static_cast<std::byte>(tag & 0xff));
    field.push_back(std::byte{0});
    field.push_back(std::byte{0});
    field.push_back(std::byte{0});
    field.push_back(static_cast<std::byte>(payload.size()));
    for (auto b : payload) {
        field.push_back(static_cast<std::byte>(b));
    }
    const auto old_len = (static_cast<std::uint32_t>(std::to_integer<unsigned char>(v[12])) << 24)
        | (static_cast<std::uint32_t>(std::to_integer<unsigned char>(v[13])) << 16)
        | (static_cast<std::uint32_t>(std::to_integer<unsigned char>(v[14])) << 8)
        | static_cast<std::uint32_t>(std::to_integer<unsigned char>(v[15]));
    v.insert(v.end(), field.begin(), field.end());
    put_u32(v, 12, old_len + static_cast<std::uint32_t>(field.size()));
}

static std::vector<std::byte> with_path(std::initializer_list<unsigned char> path)
{
    auto v = bytes(c05::fixtures::v1_octets);
    std::vector<std::byte> field{
        std::byte{0}, std::byte{3},
        std::byte{0}, std::byte{0}, std::byte{0}, static_cast<std::byte>(path.size())
    };
    for (auto b : path) {
        field.push_back(static_cast<std::byte>(b));
    }
    v.erase(v.begin() + 33, v.begin() + 40);
    v.insert(v.begin() + 33, field.begin(), field.end());
    put_u32(v, 12, static_cast<std::uint32_t>(52 - 7 + field.size()));
    return v;
}

static void expect_decode_fail(const std::vector<std::byte>& packet, std::string_view message)
{
    auto got = c05_ex::decode_manifest(packet);
    check(!got, message);
}

int main()
{
    auto v1 = bytes(c05::fixtures::v1_octets);
    auto v2_empty_note = bytes(c05::fixtures::v2_empty_note_octets);

    auto encoded_v1 = c05_ex::encode_manifest(c05::fixtures::manifest_v1(), c05::WireVersion::v1);
    check(encoded_v1 && octets(*encoded_v1) == std::vector<unsigned char>(c05::fixtures::v1_octets.begin(), c05::fixtures::v1_octets.end()), "v1 writer matches independent golden bytes");

    auto encoded_v2 = c05_ex::encode_manifest(c05::fixtures::manifest_v2_empty_note(), c05::WireVersion::v2);
    check(encoded_v2 && octets(*encoded_v2) == std::vector<unsigned char>(c05::fixtures::v2_empty_note_octets.begin(), c05::fixtures::v2_empty_note_octets.end()), "v2 empty note writer matches independent golden bytes");

    auto decoded_v1 = c05_ex::decode_manifest(v1);
    check(decoded_v1 && *decoded_v1 == c05::fixtures::manifest_v1(), "v2 reader reads v1 golden without note");
    auto decoded_v2 = c05_ex::decode_manifest(v2_empty_note);
    check(decoded_v2 && *decoded_v2 == c05::fixtures::manifest_v2_empty_note(), "v2 reader preserves empty note as present");

    auto empty_encoded = c05_ex::encode_manifest(c05::Manifest{}, c05::WireVersion::v2);
    check(empty_encoded && empty_encoded->size() == 12, "empty manifest writes header only");
    auto empty_decoded = c05_ex::decode_manifest(*empty_encoded);
    check(empty_decoded && empty_decoded->records.empty(), "empty manifest decodes");

    for (std::size_t n = 0; n != v2_empty_note.size(); ++n) {
        expect_decode_fail(std::vector<std::byte>(v2_empty_note.begin(), v2_empty_note.begin() + static_cast<std::ptrdiff_t>(n)), "rejects every truncate point");
    }

    auto trailing = v1;
    trailing.push_back(std::byte{0});
    expect_decode_fail(trailing, "rejects trailing package data");

    auto minor0 = v1;
    put_u16(minor0, 6, 0);
    expect_decode_fail(minor0, "rejects minor zero");

    auto major2 = v1;
    put_u16(major2, 4, 2);
    expect_decode_fail(major2, "rejects unknown major");

    auto duplicate_known = v1;
    duplicate_known.insert(duplicate_known.begin() + 26, v1.begin() + 16, v1.begin() + 26);
    put_u32(duplicate_known, 12, 0x3e);
    expect_decode_fail(duplicate_known, "rejects duplicate known field");

    auto missing_name = v1;
    missing_name.erase(missing_name.begin() + 26, missing_name.begin() + 33);
    put_u32(missing_name, 12, 0x2d);
    expect_decode_fail(missing_name, "rejects missing required field");

    auto bad_id_len = v1;
    put_u32(bad_id_len, 18, 3);
    expect_decode_fail(bad_id_len, "rejects wrong scalar width");

    auto long_name_wire = v1;
    put_u32(long_name_wire, 28, static_cast<std::uint32_t>(c05::max_text_bytes + 1));
    expect_decode_fail(long_name_wire, "rejects oversized wire string length");

    expect_decode_fail(std::vector<std::byte>(c05::max_package_bytes + 1, std::byte{0}), "rejects oversized package");

    auto dup_unknown = v1;
    insert_raw_field(dup_unknown, 99, {'x'});
    insert_raw_field(dup_unknown, 99, {'y'});
    put_u16(dup_unknown, 6, 9);
    expect_decode_fail(dup_unknown, "rejects duplicate unknown field");

    auto future = v1;
    insert_raw_field(future, 77, {'z'});
    put_u16(future, 6, 9);
    auto future_decoded = c05_ex::decode_manifest(future);
    check(future_decoded && *future_decoded == c05::fixtures::manifest_v1(), "future minor skips bounded unknown field");
    auto future_reencoded = c05_ex::encode_manifest(*future_decoded, c05::WireVersion::v2);
    check(future_reencoded && future_reencoded->size() < future.size(), "re-encode drops unknown fields");

    auto old_read_new = c05_l15::decode_manifest_v1(v2_empty_note);
    check(old_read_new && *old_read_new == c05::fixtures::manifest_v1(), "v1 reader skips v2 note field");

    auto v1_note = c05_ex::encode_manifest(c05::fixtures::manifest_v2_empty_note(), c05::WireVersion::v1);
    check(!v1_note, "v1 writer rejects note");

    auto invalid_version = c05_ex::encode_manifest(c05::fixtures::manifest_v1(), static_cast<c05::WireVersion>(99));
    check(!invalid_version, "writer rejects invalid WireVersion enum");

    auto bad_manifest = c05::fixtures::manifest_v1();
    bad_manifest.records[0].id = 0;
    check(!c05_ex::validate_manifest(bad_manifest), "rejects zero id");
    bad_manifest = c05::fixtures::manifest_v1();
    bad_manifest.records.push_back(bad_manifest.records[0]);
    check(!c05_ex::validate_manifest(bad_manifest), "rejects duplicate ids");
    bad_manifest = c05::fixtures::manifest_v1();
    bad_manifest.records[0].name.clear();
    check(!c05_ex::validate_manifest(bad_manifest), "rejects empty name");
    bad_manifest = c05::fixtures::manifest_v1();
    bad_manifest.records[0].name = std::string("A\0B", 3);
    check(!c05_ex::validate_manifest(bad_manifest), "rejects nul in name");
    bad_manifest = c05::fixtures::manifest_v1();
    bad_manifest.records[0].name.assign(c05::max_text_bytes + 1, 'x');
    check(!c05_ex::validate_manifest(bad_manifest), "rejects oversized string");
    bad_manifest = c05::fixtures::manifest_v1();
    bad_manifest.records[0].path = "/abs";
    check(!c05_ex::validate_manifest(bad_manifest), "rejects absolute path in manifest");
    bad_manifest.records[0].path = "C:asset";
    check(!c05_ex::validate_manifest(bad_manifest), "rejects drive path in manifest");
    bad_manifest.records[0].path = "dir\\asset";
    check(!c05_ex::validate_manifest(bad_manifest), "rejects backslash path in manifest");
    bad_manifest.records[0].path = "dir//asset";
    check(!c05_ex::validate_manifest(bad_manifest), "rejects empty path segment in manifest");
    bad_manifest.records[0].path = "dir/./asset";
    check(!c05_ex::validate_manifest(bad_manifest), "rejects dot path segment in manifest");
    bad_manifest.records[0].path = "dir/../asset";
    check(!c05_ex::validate_manifest(bad_manifest), "rejects dotdot path segment in manifest");
    bad_manifest.records[0].path = std::string("a\0b", 3);
    check(!c05_ex::validate_manifest(bad_manifest), "rejects nul in path");
    bad_manifest = c05::fixtures::manifest_v1();
    bad_manifest.records[0].modified_at_ms = c05::max_timestamp_ms + 1;
    check(!c05_ex::validate_manifest(bad_manifest), "rejects timestamp outside range");
    bad_manifest.records[0].modified_at_ms = c05::min_timestamp_ms - 1;
    check(!c05_ex::validate_manifest(bad_manifest), "rejects timestamp below range");
    c05::Manifest too_many;
    too_many.records.assign(c05::max_records + 1, c05::fixtures::manifest_v1().records[0]);
    check(!c05_ex::validate_manifest(too_many), "rejects too many records");

    auto wire_abs = c05_ex::decode_manifest(with_path({'/', 'a'}));
    check(!wire_abs && wire_abs.error().offset == 39, "wire absolute path reports packet byte offset");
    auto wire_drive = c05_ex::decode_manifest(with_path({'C', ':', 'a'}));
    check(!wire_drive && wire_drive.error().offset == 39, "wire drive path reports packet byte offset");
    auto wire_backslash = c05_ex::decode_manifest(with_path({'a', '\\', 'b'}));
    check(!wire_backslash && wire_backslash.error().offset == 40, "wire backslash path reports packet byte offset");
    auto wire_empty_segment = c05_ex::decode_manifest(with_path({'a', '/', '/', 'b'}));
    check(!wire_empty_segment && wire_empty_segment.error().offset == 41, "wire empty path segment reports packet byte offset");
    auto wire_dot = c05_ex::decode_manifest(with_path({'a', '/', '.', '/', 'b'}));
    check(!wire_dot && wire_dot.error().offset == 41, "wire dot segment reports packet byte offset");
    auto wire_dotdot = c05_ex::decode_manifest(with_path({'a', '/', '.', '.', '/', 'b'}));
    check(!wire_dotdot && wire_dotdot.error().offset == 41, "wire dotdot segment reports packet byte offset");
    auto wire_bad_utf = c05_ex::decode_manifest(with_path({0xe2, 0x28, 0xa1}));
    check(!wire_bad_utf && wire_bad_utf.error().offset == 39, "wire invalid utf8 path reports packet byte offset");
    auto wire_nul = c05_ex::decode_manifest(with_path({'a', 0, 'b'}));
    check(!wire_nul && wire_nul.error().offset == 39, "wire nul path is rejected");
}
