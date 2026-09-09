#include "mini_ref/mini.hpp"

#include <cstdlib>

int main() {
    mini_ref::stop_source source;
    mini_ref::stop_token token = source.get_token();
    if (token.stop_requested()) std::abort();
    if (!source.request_stop()) std::abort();
    if (!token.stop_requested()) std::abort();
}
