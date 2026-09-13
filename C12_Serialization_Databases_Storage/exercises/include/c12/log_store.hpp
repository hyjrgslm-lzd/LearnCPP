#pragma once
#include <c12/file.hpp>
#include <functional>

namespace c12 {
inline constexpr std::uint32_t log_magic=0x4331324cu,snapshot_magic=0x43313253u;
inline bytes frame(std::uint32_t magic,std::uint64_t sequence,std::span<const std::byte> payload){
    bytes out;
    c05::append_be(out,magic);c05::append_be(out,std::uint32_t{1});c05::append_be(out,sequence);
    c05::append_be(out,static_cast<std::uint32_t>(payload.size()));c05::append_be(out,crc32(out));
    append(out,payload);c05::append_be(out,crc32(payload));c05::append_be(out,std::uint32_t{0x434d4954});
    return out;
}
struct frame_view{std::uint64_t sequence;std::span<const std::byte> payload;std::size_t end;};
inline std::optional<frame_view> read_frame(std::span<const std::byte> in,std::size_t begin,
                                           std::uint32_t magic,std::size_t maximum){
    if(begin>in.size())throw failure(errc::corrupt,"frame offset");
    if(in.size()-begin<24)return {};
    auto at=begin;
    const auto found=number<std::uint32_t>(in,at),version=number<std::uint32_t>(in,at);
    const auto sequence=number<std::uint64_t>(in,at);
    const auto length=number<std::uint32_t>(in,at),header_crc=number<std::uint32_t>(in,at);
    if(found!=magic||version!=1||header_crc!=crc32(in.subspan(begin,20))||length>maximum)
        throw failure(errc::corrupt,"frame header",0,begin);
    if(in.size()-at<static_cast<std::size_t>(length)+8)return {};
    auto payload=in.subspan(at,length);at+=length;
    const auto checksum=number<std::uint32_t>(in,at),tail=number<std::uint32_t>(in,at);
    if(checksum!=crc32(payload)||tail!=0x434d4954)throw failure(errc::corrupt,"frame payload",0,begin);
    return frame_view{sequence,payload,at};
}
// Empty during ordinary operation; tests stop the real child at named I/O boundaries.
enum class fault_point{half_record,before_flush,after_flush,half_snapshot,snapshot_flushed};
using fault_hook=std::function<void(fault_point)>;
enum class recovery_path{full_replay,checkpoint};
class log_store{
    std::filesystem::path directory_;
    file log_;
    dictionary values_;
    std::uint64_t sequence_=0;
    std::size_t end_=0;
    bool writable_=true;
    unsigned next_slot_=0;
    fault_hook hook_;
    void signal(fault_point p){if(hook_)hook_(p);}
    log_store(std::filesystem::path directory,file journal,fault_hook hook)
        :directory_(std::move(directory)),log_(std::move(journal)),hook_(std::move(hook)){}
public:
    std::uint64_t checkpoint_sequence=0;
    std::size_t repaired_tail_bytes=0,rejected_snapshots=0;
    std::size_t scanned_bytes=0,decoded_batches=0,applied_batches=0;
    log_store(const log_store&)=delete;log_store& operator=(const log_store&)=delete;
    // std::map move construction can allocate a sentinel on MSVC; infer its exception specification.
    log_store(log_store&&)=default;log_store& operator=(log_store&&)=default;
    static result<log_store> open(const std::filesystem::path& directory,fault_hook hook={},recovery_path path=recovery_path::checkpoint){
        try{
            std::filesystem::create_directories(directory);
            log_store store(directory,file(directory/"journal.bin"),std::move(hook));
            auto raw=store.log_.read_all(max_log);store.scanned_bytes=raw.size();
            std::vector<frame_view> frames;
            std::size_t at=0;
            while(at<raw.size()){
                auto view=read_frame(raw,at,log_magic,max_payload);
                if(!view)break;
                if(view->sequence!=frames.size()+1)throw failure(errc::corrupt,"log sequence",0,at);
                if(decode_operations(view->payload).empty())throw failure(errc::corrupt,"empty batch");
                ++store.decoded_batches;
                frames.push_back(*view);at=view->end;
            }
            store.end_=at;store.sequence_=frames.size();
            for(unsigned slot=0;slot!=2;++slot){
                file snapshot(directory/(slot?"snapshot1.bin":"snapshot0.bin"));
                if(!snapshot.size())continue;
                try{
                    auto data=snapshot.read_all(8*1024*1024);
                    auto view=read_frame(data,0,snapshot_magic,8*1024*1024-32);
                    if(!view||view->end!=data.size()||view->sequence>store.sequence_)
                        throw failure(errc::corrupt,"snapshot frame");
                    std::size_t offset=0;
                    const auto prefix_crc=number<std::uint32_t>(view->payload,offset);
                    const auto prefix_end=view->sequence?frames[static_cast<std::size_t>(view->sequence-1)].end:0;
                    if(prefix_crc!=crc32(std::span(raw).first(prefix_end)))throw failure(errc::corrupt,"snapshot history");
                    auto operations=decode_operations(view->payload.subspan(offset),max_keys);
                    dictionary candidate;
                    for(auto& op:operations){
                        if(!op.value||!candidate.emplace(std::move(op.key),std::move(*op.value)).second)
                            throw failure(errc::corrupt,"snapshot duplicate/delete");
                    }
                    if(view->sequence>=store.checkpoint_sequence){
                        store.checkpoint_sequence=view->sequence;store.next_slot_=1-slot;
                        if(path==recovery_path::checkpoint)store.values_=std::move(candidate);
                    }
                }catch(const failure&){++store.rejected_snapshots;}
            }
            const auto replay_begin=path==recovery_path::checkpoint?static_cast<std::size_t>(store.checkpoint_sequence):0;
            for(std::size_t n=replay_begin;n!=frames.size();++n){
                auto operations=decode_operations(frames[n].payload);++store.decoded_batches;
                auto next=prepare_batch(store.values_,operations);
                if(!next){
                    if(next.error().code==errc::allocation)return std::unexpected(next.error());
                    throw failure(errc::corrupt,"invalid replay state");
                }
                store.values_.swap(*next);++store.applied_batches;
            }
            // Complete frames have all been validated before any recovery mutation.
            store.repaired_tail_bytes=raw.size()-store.end_;
            if(store.repaired_tail_bytes){store.log_.truncate(store.end_);store.log_.flush();}
            return store;
        }catch(const failure& e){return std::unexpected(e.detail);}
        catch(const std::filesystem::filesystem_error& e){return std::unexpected(error{errc::io,e.code().value()});}
        catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
    }
    result<std::optional<std::string>> get(std::string_view key)const{
        if(!writable_)return std::unexpected(error{errc::outcome_unknown});
        if(key.empty()||key.size()>max_key)return std::unexpected(error{errc::invalid});
        try{
            auto it=values_.find(std::string(key));
            if(it==values_.end())return std::optional<std::string>{};
            return std::optional<std::string>{it->second};
        }catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
    }
    result<std::uint64_t> write_batch(std::span<const mutation> operations){
        if(!writable_)return std::unexpected(error{errc::outcome_unknown});
        auto next=prepare_batch(values_,operations);
        if(!next)return std::unexpected(next.error());
        bool io_started=false;
        try{
            auto payload=encode_operations(operations);
            auto record=frame(log_magic,sequence_+1,payload);
            if(sequence_==UINT64_MAX||payload.size()>max_payload||record.size()>max_log-end_)
                return std::unexpected(error{errc::capacity});
            io_started=true;
            const auto half=record.size()/2;
            log_.write_at(end_,std::span(record).first(half));signal(fault_point::half_record);
            log_.write_at(end_+half,std::span(record).subspan(half));signal(fault_point::before_flush);
            log_.flush();signal(fault_point::after_flush);
            values_.swap(*next);end_+=record.size();return ++sequence_;
        }catch(const failure& e){
            if(io_started){writable_=false;return std::unexpected(error{errc::outcome_unknown,e.detail.native});}
            return std::unexpected(e.detail);
        }catch(const std::bad_alloc&){
            if(io_started){writable_=false;return std::unexpected(error{errc::outcome_unknown});}
            return std::unexpected(error{errc::allocation});
        }catch(...){
            if(io_started){writable_=false;return std::unexpected(error{errc::outcome_unknown});}
            throw;
        }
    }
    result<void> checkpoint(){
        if(!writable_)return std::unexpected(error{errc::outcome_unknown});
        try{
            std::vector<mutation> operations;
            for(const auto& [key,value]:values_)operations.push_back({key,value});
            auto log_bytes=log_.read_all(max_log);
            bytes payload;c05::append_be(payload,crc32(log_bytes));append(payload,encode_operations(operations));
            auto record=frame(snapshot_magic,sequence_,payload);
            file snapshot(directory_/(next_slot_?"snapshot1.bin":"snapshot0.bin"));
            snapshot.truncate(0);
            const auto half=record.size()/2;
            snapshot.write_at(0,std::span(record).first(half));signal(fault_point::half_snapshot);
            snapshot.write_at(half,std::span(record).subspan(half));snapshot.flush();signal(fault_point::snapshot_flushed);
            checkpoint_sequence=sequence_;next_slot_=1-next_slot_;return {};
        }catch(const failure& e){return std::unexpected(e.detail);}
        catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
    }
    std::uint64_t sequence()const noexcept{return sequence_;}
    // ponytail: retain complete log up to 64 MiB; production reclamation is the SQLite WAL lesson.
};
}
