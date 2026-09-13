#include <solution.hpp>
#include <check.hpp>
#include <array>
int main(){
    using namespace c12;
    dictionary values{{"old","kept"}};
    const std::array invalid{mutation{"new","leaked"},mutation{"","bad"}};
    auto rejected=solution::apply(values,invalid);
    if(!rejected&&rejected.error().code==errc::unfinished){std::cerr<<"UNFINISHED\n";return 1;}
    check(!rejected&&values==dictionary{{"old","kept"}},"atomic rejection");
    const std::array valid{mutation{"old",{}},mutation{"new","first"},mutation{"new","last"}};
    check(solution::apply(values,valid).has_value()&&values==dictionary{{"new","last"}},"ordered duplicate and delete");
    auto previous=values;
    const std::array too_large{mutation{"ok","value"},mutation{"large",std::string(max_value+1,'x')}};
    check(!solution::apply(values,too_large)&&values==previous,"value limit preserves state");
    check(!solution::apply(values,{})&&values==previous,"empty batch rejected");
    const std::array long_key{mutation{"ok","value"},mutation{std::string(max_key+1,'k'),"x"}};
    check(!solution::apply(values,long_key)&&values==previous,"key limit preserves state");
    std::vector<mutation> too_many(max_batch+1,mutation{"new","changed"});
    check(!solution::apply(values,too_many)&&values==previous,"batch count preserves state");
    const std::array edge{mutation{std::string(max_key,'k'),std::string(max_value,'v')}};
    check(solution::apply(values,edge).has_value(),"exact key and value limits");
    std::vector<mutation> exact(max_batch,mutation{"new","last"});
    check(solution::apply(values,exact).has_value(),"exact batch limit");
    const std::array empty_value{mutation{"new",std::string{}}};
    check(solution::apply(values,empty_value).has_value()&&values.contains("new")&&values.at("new").empty(),"empty value is present");
    const std::array erase{mutation{"new",{}}};
    check(solution::apply(values,erase).has_value()&&!values.contains("new"),"delete is absent");
    values.clear();for(std::size_t n=0;n<max_keys;++n)values.emplace(std::to_string(n),"x");
    previous=values;
    const std::array overflow{mutation{"overflow","x"}};
    check(!solution::apply(values,overflow)&&values==previous,"index capacity");
    std::cout<<"batch implementation checks passed\n";
}
