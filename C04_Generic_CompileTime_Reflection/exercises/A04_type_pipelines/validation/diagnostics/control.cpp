#include <type_pipelines.hpp>
#include <type_traits>

int main() {
    using got = c04_pipeline::zip_t<c04_pipeline::type_list<int>, c04_pipeline::type_list<char>>;
    static_assert(std::is_same_v<got, c04_pipeline::type_list<c04_pipeline::type_list<int, char>>>);
}
