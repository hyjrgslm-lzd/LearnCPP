#include <check.hpp>

#include <c05/paths.hpp>

#include <filesystem>
#include <string>

int main()
{
    namespace fs = std::filesystem;

    const fs::path native = fs::path{"alpha"} / "beta.txt";
    check(native.generic_string() == "alpha/beta.txt", "generic_string uses slash separators");

    const fs::path lexical = fs::path{"alpha/./beta/../beta.txt"}.lexically_normal();
    check(lexical.generic_string() == "alpha/beta.txt", "lexically_normal removes dot segments without touching filesystem");

    std::error_code ec;
    const auto cwd = fs::current_path(ec);
    check(!ec && !cwd.empty(), "filesystem error_code overload reports environment errors without throwing");

    check(c05::validate_resource_path("assets/texture.png").has_value(), "resource path accepts generic relative path");
    check(!c05::validate_resource_path("/assets/texture.png"), "resource path rejects root");
    check(!c05::validate_resource_path("C:/assets/texture.png"), "resource path rejects drive");
    check(!c05::validate_resource_path("assets\\texture.png"), "resource path rejects backslash");
    check(!c05::validate_resource_path("assets//texture.png"), "resource path rejects empty segment");
    check(!c05::validate_resource_path("assets/../secret.txt"), "resource path rejects parent segment");
    check(!c05::validate_resource_path(std::string{"assets/\0name.txt", 16}), "resource path rejects nul");
}
