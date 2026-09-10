#include <type_pipelines.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

struct LvalueOnly {
    std::string operator()(short) & noexcept;
    long operator()(long) &;
};

struct RvalueOnly {
    int operator()(int) && noexcept;
};

int main()
{
    using namespace c04_pipeline;

    using product = variant_product_t<std::variant<short, long>, std::variant<char>>;
    static_assert(std::is_same_v<product, type_list<type_list<short, char>, type_list<long, char>>>);

    using transformed = transform_completion_signatures_t<
        LvalueOnly&,
        type_list<value_sig<short>, value_sig<long>, error_sig<std::runtime_error>, stopped_sig>>;
    static_assert(std::is_same_v<transformed,
        type_list<value_sig<std::string>, value_sig<long>, error_sig<std::exception_ptr>,
            error_sig<std::runtime_error>, stopped_sig>>);

    static_assert(!transformable<RvalueOnly&, type_list<value_sig<int>>>);
}
