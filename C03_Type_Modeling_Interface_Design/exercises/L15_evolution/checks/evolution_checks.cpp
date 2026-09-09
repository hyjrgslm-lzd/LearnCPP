#include <compat_adapter.hpp>

#include <check.hpp>

#include <iostream>

#include <string>
#include <string_view>

namespace {

std::string old_client_call(const l15::CompatClient& client, std::string_view name)
{
    return client.fetch(name);
}

void check_v1_default_behavior()
{
    l15::CompatClient client;
    check(old_client_call(client, "alice") == "alice|timeout=1000|legacy",
        "v1 default timeout remains 1000");
}

void check_explicit_timeout_keeps_legacy_format()
{
    l15::CompatClient client;
    check(client.fetch("bob", 3000) == "bob|timeout=3000|legacy",
        "explicit v1 timeout keeps legacy format");
}

void check_abi_surface_model()
{
    check(l15::abi_model::public_layout_changed,
        "public data layout change is an ABI risk model");
    std::cout << "public data layout change is an ABI risk model\n";
}

void check_new_behavior_is_explicit()
{
    l15::CompatClient client;
    check(client.fetch_compact("carol") == "carol|timeout=250|compact",
        "compact behavior is explicit new entry point");
}

} // namespace

int main()
{
    check_v1_default_behavior();
    check_explicit_timeout_keeps_legacy_format();
    check_new_behavior_is_explicit();
    check_abi_surface_model();
}
