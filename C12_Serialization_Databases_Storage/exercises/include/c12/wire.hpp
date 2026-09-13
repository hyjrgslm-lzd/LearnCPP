#pragma once
#include <c12/bytes.hpp>
namespace c12 {
inline bytes varuint(std::uint64_t value){
    bytes out;
    do{
        auto b=static_cast<unsigned char>(value&0x7f);value>>=7;
        out.push_back(static_cast<std::byte>(b|(value?0x80:0)));
    }while(value);
    return out;
}
// C12's numeric exercise accepts only shortest encodings. Protobuf has its own parser contract.
inline result<std::uint64_t> read_varuint(std::span<const std::byte> input,std::size_t& cursor){
    const auto start=cursor;auto probe=cursor;std::uint64_t value=0;
    for(unsigned n=0;n!=10;++n){
        if(probe>=input.size())return std::unexpected(error{errc::corrupt,0,start});
        const auto b=std::to_integer<unsigned char>(input[probe++]);
        if(n==9&&(b&0xfe))return std::unexpected(error{errc::invalid,0,start});
        value|=static_cast<std::uint64_t>(b&0x7f)<<(7*n);
        if(!(b&0x80)){
            if(n&&!(b&0x7f))return std::unexpected(error{errc::invalid,0,start});
            cursor=probe;return value;
        }
    }
    return std::unexpected(error{errc::invalid,0,start});
}
constexpr std::uint64_t zigzag(std::int64_t value){
    const auto bits=static_cast<std::uint64_t>(value);
    return (bits<<1)^(std::uint64_t{0}-(bits>>63));
}
constexpr std::int64_t unzigzag(std::uint64_t value){
    const auto magnitude=static_cast<std::int64_t>(value>>1);
    return value&1?-1-magnitude:magnitude;
}
static_assert(zigzag(INT64_MIN)==UINT64_MAX&&unzigzag(UINT64_MAX)==INT64_MIN);
class blob_view{
    std::span<const std::byte> payload_;
    std::vector<std::uint32_t> offsets_;
public:
    static result<blob_view> read(std::span<const std::byte> input){
        try{
            if(input.size()>1024*1024)return std::unexpected(error{errc::capacity});
            std::size_t at=0;const auto count=number<std::uint32_t>(input,at);
            if(count>max_keys)return std::unexpected(error{errc::capacity});
            blob_view result;result.offsets_.reserve(static_cast<std::size_t>(count)+1);
            for(std::uint32_t n=0;n<=count;++n)result.offsets_.push_back(number<std::uint32_t>(input,at));
            result.payload_=input.subspan(at);
            if(result.offsets_.front()!=0||result.offsets_.back()!=result.payload_.size())
                return std::unexpected(error{errc::corrupt});
            if(!std::is_sorted(result.offsets_.begin(),result.offsets_.end()))
                return std::unexpected(error{errc::corrupt});
            return result;
        }catch(const failure& e){return std::unexpected(e.detail);}
        catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
    }
    std::size_t size()const noexcept{return offsets_.empty()?0:offsets_.size()-1;}
    result<std::span<const std::byte>> at(std::size_t index)const{
        if(index>=size())return std::unexpected(error{errc::invalid});
        return payload_.subspan(offsets_[index],offsets_[index+1]-offsets_[index]);
    }
};
inline result<bytes> encode_blobs(std::span<const std::string> values){
    if(values.size()>max_keys)return std::unexpected(error{errc::capacity});
    try{
        std::size_t total=4+(values.size()+1)*4;
        for(const auto& value:values){
            if(value.size()>1024*1024-total)return std::unexpected(error{errc::capacity});
            total+=value.size();
        }
        bytes out;out.reserve(total);c05::append_be(out,static_cast<std::uint32_t>(values.size()));
        std::uint32_t offset=0;c05::append_be(out,offset);
        for(const auto& value:values){offset+=static_cast<std::uint32_t>(value.size());c05::append_be(out,offset);}
        for(const auto& value:values)append(out,as_bytes(value));
        return out;
    }catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
}
struct graph_input{std::uint32_t id;std::vector<std::uint32_t> links;};
class object_graph{
    struct node{std::uint32_t id;std::vector<std::size_t> links;};
    std::vector<node> nodes_;
    std::map<std::uint32_t,std::size_t> index_;
public:
    static result<object_graph> resolve(std::span<const graph_input> input){
        if(input.size()>max_keys)return std::unexpected(error{errc::capacity});
        try{
            object_graph graph;graph.nodes_.reserve(input.size());
            std::size_t edges=0;
            for(const auto& item:input){
                if(!item.id||!graph.index_.emplace(item.id,graph.nodes_.size()).second)
                    return std::unexpected(error{errc::invalid});
                if(item.links.size()>4096-edges)return std::unexpected(error{errc::capacity});
                edges+=item.links.size();graph.nodes_.push_back({item.id,{}});
            }
            for(std::size_t n=0;n<input.size();++n)for(auto id:input[n].links){
                auto found=graph.index_.find(id);
                if(found==graph.index_.end())return std::unexpected(error{errc::invalid});
                graph.nodes_[n].links.push_back(found->second);
            }
            return graph;
        }catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
    }
    result<std::vector<std::uint32_t>> reachable(std::uint32_t start)const{
        auto found=index_.find(start);if(found==index_.end())return std::unexpected(error{errc::invalid});
        try{
            std::vector<bool> visited(nodes_.size());
            std::vector<std::size_t> queue{found->second};
            std::vector<std::uint32_t> out;visited[found->second]=true;
            for(std::size_t n=0;n<queue.size();++n){
                const auto& current=nodes_[queue[n]];out.push_back(current.id);
                for(auto next:current.links)if(!visited[next]){visited[next]=true;queue.push_back(next);}
            }
            return out;
        }catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
    }
};
}
