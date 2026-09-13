#include <c12/wire.hpp>
#include <check.hpp>
#include <array>
int main(){
    using namespace c12;
    const std::array<std::string,3> inputs{"hello","",std::string("a\0b",3)};
    auto encoded=encode_blobs(inputs);check(encoded.has_value(),"bounded table encoding");
    auto view=blob_view::read(*encoded);check(view&&view->size()==3,"borrowed table");
    check(view->at(0)->data()==encoded->data()+20,"payload aliases the retained owner");
    check(view->at(1)->empty()&&view->at(2)->size()==3&&!view->at(3),"empty and binary views");
    auto owned=std::string(reinterpret_cast<const char*>(view->at(0)->data()),view->at(0)->size());
    check(owned=="hello","copy establishes separate ownership");
    for(std::size_t cut=0;cut<encoded->size();++cut)
        check(!blob_view::read(std::span(*encoded).first(cut)),"truncated table");
    auto damaged=*encoded;damaged[7]=std::byte{1};
    check(!blob_view::read(damaged),"first offset must be zero");
    damaged=*encoded;damaged[15]=std::byte{4};
    check(!blob_view::read(damaged),"offsets must be monotone");
    check(encode_blobs({})&&blob_view::read(*encode_blobs({}))->size()==0,"empty table");
    bytes excessive{std::byte{0xff},std::byte{0xff},std::byte{0xff},std::byte{0xff}};
    check(!blob_view::read(excessive),"bound count before allocation");
    const std::array cyclic{graph_input{10,{20,30}},graph_input{20,{10}},graph_input{30,{20}}};
    auto graph=object_graph::resolve(cyclic);
    check(graph&&graph->reachable(10).value()==std::vector<std::uint32_t>{10,20,30},"identity and cycles without recursive expansion");
    const std::array dangling{graph_input{10,{99}}}; const std::array duplicate{graph_input{10,{}},graph_input{10,{}}};
    check(!object_graph::resolve(dangling)&&!object_graph::resolve(duplicate),"resolve all references before publishing");
    check(!graph->reachable(99),"unknown root");
    std::vector<std::string> table_limit(max_keys);
    auto exact_table=encode_blobs(table_limit);
    check(exact_table&&blob_view::read(*exact_table)->size()==max_keys,"exact field count limit");
    table_limit.emplace_back();check(!encode_blobs(table_limit),"field count overflow");
    const std::array<std::string,1> byte_limit{std::string(1024*1024-12,'x')};
    check(encode_blobs(byte_limit).has_value(),"exact table byte budget");
    const std::array<std::string,1> byte_overflow{std::string(1024*1024-11,'x')};
    check(!encode_blobs(byte_overflow),"one byte beyond table budget");
    std::vector<graph_input> graph_limit;
    for(std::uint32_t id=1;id<=max_keys;++id)graph_limit.push_back({id,{}});
    graph_limit[0].links.assign(4096,2);
    auto edge=object_graph::resolve(graph_limit);
    check(edge&&edge->reachable(1).value()==std::vector<std::uint32_t>{1,2},"exact graph budgets and repeated edges");
    graph_limit[0].links.push_back(2);check(!object_graph::resolve(graph_limit),"edge budget rejection");
    graph_limit[0].links.clear();graph_limit.push_back({1025,{}});
    check(!object_graph::resolve(graph_limit),"node budget rejection");
    std::cout<<"borrowed views and bounded object graph checks passed\n";
}
