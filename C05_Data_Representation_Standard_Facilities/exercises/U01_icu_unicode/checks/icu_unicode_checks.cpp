#include <c05/utf.hpp>
#include <check.hpp>

#include <unicode/localpointer.h>
#include <unicode/locid.h>
#include <unicode/normalizer2.h>
#include <unicode/ubrk.h>
#include <unicode/unistr.h>
#include <unicode/uversion.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct ParsedText {
    icu::UnicodeString text;
    std::vector<int32_t> utf16_offsets{0};
    std::vector<std::size_t> utf8_offsets{0};
};

std::string trim_ascii(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) {
        text.remove_suffix(1);
    }
    return std::string(text);
}

std::vector<std::string> split_semicolon(std::string_view line)
{
    std::vector<std::string> out;
    while (true) {
        const auto pos = line.find(';');
        if (pos == std::string_view::npos) {
            out.push_back(trim_ascii(line));
            return out;
        }
        out.push_back(trim_ascii(line.substr(0, pos)));
        line.remove_prefix(pos + 1);
    }
}

std::optional<char32_t> parse_hex_scalar(std::string_view token)
{
    std::uint32_t value{};
    auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value, 16);
    if (ec != std::errc{} || ptr != token.data() + token.size() || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) {
        return std::nullopt;
    }
    return static_cast<char32_t>(value);
}

std::size_t utf8_width(char32_t cp)
{
    return cp <= 0x7f ? 1U : cp <= 0x7ff ? 2U : cp <= 0xffff ? 3U : 4U;
}

ParsedText parse_codepoints(std::string_view field)
{
    ParsedText result;
    while (!field.empty()) {
        while (!field.empty() && field.front() == ' ') {
            field.remove_prefix(1);
        }
        if (field.empty()) {
            break;
        }
        const auto end = field.find(' ');
        const auto token = field.substr(0, end);
        auto cp = parse_hex_scalar(token);
        check(cp.has_value(), "bad code point token in official data");
        result.text.append(static_cast<UChar32>(*cp));
        result.utf16_offsets.push_back(result.text.length());
        result.utf8_offsets.push_back(result.utf8_offsets.back() + utf8_width(*cp));
        if (end == std::string_view::npos) {
            break;
        }
        field.remove_prefix(end + 1);
    }
    return result;
}

icu::UnicodeString normalize_with(const icu::Normalizer2* normalizer, const icu::UnicodeString& input)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString out;
    normalizer->normalize(input, out, status);
    check(U_SUCCESS(status), "ICU normalization succeeds");
    return out;
}

void check_pairs(const char* name, const icu::Normalizer2* normalizer,
    std::initializer_list<std::pair<const icu::UnicodeString*, const icu::UnicodeString*>> pairs,
    std::size_t line_number)
{
    for (const auto [src, expected] : pairs) {
        if (normalize_with(normalizer, *src) != *expected) {
            std::cerr << name << " mismatch at NormalizationTest line " << line_number << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

void check_normalization_line(const std::vector<std::string>& fields, std::size_t line_number)
{
    auto c1 = parse_codepoints(fields[0]).text;
    auto c2 = parse_codepoints(fields[1]).text;
    auto c3 = parse_codepoints(fields[2]).text;
    auto c4 = parse_codepoints(fields[3]).text;
    auto c5 = parse_codepoints(fields[4]).text;

    UErrorCode status = U_ZERO_ERROR;
    const auto* nfc = icu::Normalizer2::getNFCInstance(status);
    check(U_SUCCESS(status), "gets NFC normalizer");
    status = U_ZERO_ERROR;
    const auto* nfd = icu::Normalizer2::getNFDInstance(status);
    check(U_SUCCESS(status), "gets NFD normalizer");
    status = U_ZERO_ERROR;
    const auto* nfkc = icu::Normalizer2::getNFKCInstance(status);
    check(U_SUCCESS(status), "gets NFKC normalizer");
    status = U_ZERO_ERROR;
    const auto* nfkd = icu::Normalizer2::getNFKDInstance(status);
    check(U_SUCCESS(status), "gets NFKD normalizer");

    check_pairs("NFC", nfc, {{&c1, &c2}, {&c2, &c2}, {&c3, &c2}, {&c4, &c4}, {&c5, &c4}}, line_number);
    check_pairs("NFD", nfd, {{&c1, &c3}, {&c2, &c3}, {&c3, &c3}, {&c4, &c5}, {&c5, &c5}}, line_number);
    check_pairs("NFKC", nfkc, {{&c1, &c4}, {&c2, &c4}, {&c3, &c4}, {&c4, &c4}, {&c5, &c4}}, line_number);
    check_pairs("NFKD", nfkd, {{&c1, &c5}, {&c2, &c5}, {&c3, &c5}, {&c4, &c5}, {&c5, &c5}}, line_number);
}

void run_normalization_test(const std::filesystem::path& path)
{
    std::ifstream input(path);
    check(input.good(), "opens NormalizationTest.txt");
    std::string line;
    std::size_t line_number = 0;
    std::size_t checked = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (auto comment = line.find('#'); comment != std::string::npos) {
            line.resize(comment);
        }
        line = trim_ascii(line);
        if (line.empty() || line.front() == '@') {
            continue;
        }
        auto fields = split_semicolon(line);
        check(fields.size() >= 5, "NormalizationTest line has five fields");
        check_normalization_line(fields, line_number);
        ++checked;
    }
    check(checked > 19000, "runs full NormalizationTest data, not a sample");
    std::cout << "NormalizationTest records " << checked << '\n';
}

ParsedText parse_grapheme_line(std::string_view line, std::vector<int32_t>& expected_breaks)
{
    ParsedText parsed;
    const std::string_view divide = reinterpret_cast<const char*>(u8"\u00f7");
    const std::string_view multiply = reinterpret_cast<const char*>(u8"\u00d7");
    while (!line.empty()) {
        while (!line.empty() && line.front() == ' ') {
            line.remove_prefix(1);
        }
        if (line.empty()) {
            break;
        }
        if (line.starts_with(divide)) {
            expected_breaks.push_back(parsed.text.length());
            line.remove_prefix(divide.size());
            continue;
        }
        if (line.starts_with(multiply)) {
            line.remove_prefix(multiply.size());
            continue;
        }
        const auto end = line.find(' ');
        const auto token = line.substr(0, end);
        auto cp = parse_hex_scalar(token);
        check(cp.has_value(), "bad grapheme code point token");
        parsed.text.append(static_cast<UChar32>(*cp));
        parsed.utf16_offsets.push_back(parsed.text.length());
        parsed.utf8_offsets.push_back(parsed.utf8_offsets.back() + utf8_width(*cp));
        if (end == std::string_view::npos) {
            break;
        }
        line.remove_prefix(end + 1);
    }
    check(!expected_breaks.empty() && expected_breaks.back() == parsed.text.length(), "grapheme test ends at boundary");
    return parsed;
}

std::vector<int32_t> actual_grapheme_breaks(const icu::UnicodeString& text)
{
    UErrorCode status = U_ZERO_ERROR;
    UBreakIterator* raw = ubrk_open(UBRK_CHARACTER, "", text.getBuffer(), text.length(), &status);
    check(U_SUCCESS(status) && raw != nullptr, "opens root grapheme break iterator");
    icu::LocalPointer<UBreakIterator> iterator(raw);
    std::vector<int32_t> out;
    for (auto pos = ubrk_first(iterator.getAlias()); pos != UBRK_DONE; pos = ubrk_next(iterator.getAlias())) {
        out.push_back(pos);
    }
    return out;
}

void run_grapheme_test(const std::filesystem::path& path)
{
    std::ifstream input(path);
    check(input.good(), "opens GraphemeBreakTest.txt");
    std::string line;
    std::size_t line_number = 0;
    std::size_t checked = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (auto comment = line.find('#'); comment != std::string::npos) {
            line.resize(comment);
        }
        line = trim_ascii(line);
        if (line.empty()) {
            continue;
        }
        std::vector<int32_t> expected;
        auto parsed = parse_grapheme_line(line, expected);
        auto actual = actual_grapheme_breaks(parsed.text);
        if (actual != expected) {
            std::cerr << "grapheme mismatch at GraphemeBreakTest line " << line_number << '\n';
            std::exit(EXIT_FAILURE);
        }
        for (auto boundary : actual) {
            auto it = std::find(parsed.utf16_offsets.begin(), parsed.utf16_offsets.end(), boundary);
            check(it != parsed.utf16_offsets.end(), "grapheme boundary maps to scalar boundary");
            const auto index = static_cast<std::size_t>(it - parsed.utf16_offsets.begin());
            check(index < parsed.utf8_offsets.size(), "UTF-16 boundary maps to UTF-8 byte offset");
        }
        ++checked;
    }
    check(checked > 1000, "runs full GraphemeBreakTest data, not a sample");
    std::cout << "GraphemeBreakTest records " << checked << '\n';
}

icu::UnicodeString strict_from_utf8(std::string_view bytes)
{
    auto ok = c05::validate_utf8(bytes);
    check(ok.has_value(), "strict UTF-8 validation runs before ICU conversion");
    check(bytes.size() <= static_cast<std::size_t>(std::numeric_limits<int32_t>::max()), "ICU StringPiece length fits int32");
    return icu::UnicodeString::fromUTF8(icu::StringPiece(bytes.data(), static_cast<int32_t>(bytes.size())));
}

std::string to_utf8(const icu::UnicodeString& text)
{
    std::string out;
    text.toUTF8String(out);
    return out;
}

void run_api_smoke()
{
    UVersionInfo icu_version{};
    UVersionInfo unicode_version{};
    u_getVersion(icu_version);
    u_getUnicodeVersion(unicode_version);
    check(icu_version[0] == 77 && icu_version[1] == 1, "ICU runtime is 77.1");
    check(unicode_version[0] == 16 && unicode_version[1] == 0, "Unicode data is 16.0");
    std::cout << "ICU " << static_cast<int>(icu_version[0]) << '.' << static_cast<int>(icu_version[1])
              << " Unicode " << static_cast<int>(unicode_version[0]) << '.' << static_cast<int>(unicode_version[1]) << '\n';

    const std::string invalid = "\xed\xa0\x80";
    check(!c05::validate_utf8(invalid), "strict UTF-8 rejects surrogate before ICU replacement can occur");

    auto explicit_length = strict_from_utf8(std::string("A\0B", 3));
    check(explicit_length.length() == 3, "UnicodeString conversion uses explicit UTF-8 byte length");

    auto sharp_s = strict_from_utf8(reinterpret_cast<const char*>(u8"Stra\u00dfe"));
    sharp_s.foldCase();
    check(to_utf8(sharp_s) == "strasse", "foldCase is locale-neutral matching text");

    icu::UnicodeString i;
    i.append(static_cast<UChar32>('I'));
    auto turkish = i;
    turkish.toLower(icu::Locale("tr"));
    check(to_utf8(turkish) == reinterpret_cast<const char*>(u8"\u0131"), "locale case mapping differs from foldCase");
}
}

int main()
{
    run_api_smoke();
    const std::filesystem::path data_dir = C05_U01_DATA_DIR;
    run_normalization_test(data_dir / "NormalizationTest.txt");
    run_grapheme_test(data_dir / "GraphemeBreakTest.txt");
}
