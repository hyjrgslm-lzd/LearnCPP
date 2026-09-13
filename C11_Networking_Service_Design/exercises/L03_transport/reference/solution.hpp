#pragma once
#include <c11/dns.hpp>
namespace exercise { inline auto decode_name(std::span<const std::uint8_t> p, std::size_t at) { return c11::dns::read_name(p,at); } }
