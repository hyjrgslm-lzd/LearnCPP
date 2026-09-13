#pragma once
#include <c11/task_types.hpp>
#include <algorithm>
#include <atomic>
#include <deque>
#include <memory>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <vector>

namespace c11::tasks {
struct control {std::stop_source stop;std::atomic<std::uint32_t> progress{0};};
struct work {id task;spec input;std::shared_ptr<control> signal;};
// Policy is the exercise seam: all admission and terminal decisions consume the
// selected Student/Reference/good/bad. Ownership and transport plumbing are provided.
template<class Policy> class basic_registry {
    struct record {
        spec input;std::string key;snapshot current;time deadline,ended{};
        std::shared_ptr<control> signal;std::optional<state> stop_reason;
    };
    struct watch {
        id identity,task;std::deque<snapshot> events;bool overflow=false;std::shared_ptr<std::atomic_bool> owner_alive;
    };
    limits bound_;std::vector<record> records_;std::vector<watch> watches_;
    id next_=1,next_watch_=1;bool exhausted_=false,watch_exhausted_=false,draining_=false;
    std::size_t live_=0;
    record* find(id value) {
        auto r=std::find_if(records_.begin(),records_.end(),[&](const record& x){return x.current.task==value;});
        return r==records_.end()?nullptr:&*r;
    }
    void publish(record& r) {
        ++r.current.revision;
        for(auto& w:watches_) if(w.task==r.current.task && !w.overflow) {
            if(w.events.size()>=bound_.watch_queue) {w.events.clear();w.overflow=true;continue;}
            try {w.events.push_back(r.current);} catch(const std::bad_alloc&) {w.events.clear();w.overflow=true;}
        }
    }
    void stop_record(record& r,state reason,time now) {
        if(terminal(r.current.phase) || r.stop_reason) return;
        r.stop_reason=reason;r.current.stop_requested=true;r.signal->stop.request_stop();
        if(r.current.phase==state::queued) {r.current.phase=reason;r.ended=now;--live_;++completed;}
        publish(r);
    }
public:
    std::uint64_t accepted=0,duplicates=0,rejected=0,completed=0;
    explicit basic_registry(limits bound={}) : bound_(bound) {
        if(!bound.live || bound.live>128 || bound.records<bound.live || bound.records>1024 ||
            !bound.watches || bound.watches>32 || !bound.watch_queue || bound.watch_queue>8 || bound.retention.count()<=0 || !bound.id_ceiling)
            throw std::invalid_argument("invalid service bounds");
        records_.reserve(bound.records);watches_.reserve(bound.watches);
    }
    void tick(time now) {
        std::erase_if(watches_,[](const watch& w){return w.owner_alive && !w.owner_alive->load(std::memory_order_relaxed);});
        for(auto& r:records_) {
            if(!terminal(r.current.phase) && now>=r.deadline) stop_record(r,state::expired,now);
            if(r.current.phase==state::running) {
                const auto progress=std::min(r.input.steps,r.signal->progress.load(std::memory_order_relaxed));
                if(progress>r.current.progress) {r.current.progress=progress;publish(r);}
            }
        }
        std::erase_if(records_,[&](const record& r){return terminal(r.current.phase) && now>=r.ended && now-r.ended>=bound_.retention;});
    }
    result<snapshot> submit(const spec& input,std::string_view key,time now) {
        tick(now);
        // ponytail: at most1024 records, bounded linear lookup; use a hash index
        // only if profiling shows this table dominates the chosen workload.
        auto old=std::find_if(records_.begin(),records_.end(),[&](const record& r){return r.key==key;});
        std::optional<previous_submission> previous;
        if(old!=records_.end()) previous=previous_submission{old->input,old->current};
        auto decision=Policy::admit({input,key,previous?&*previous:nullptr,draining_,live_,records_.size(),bound_});
        if(!decision) {++rejected;return std::unexpected(decision.error());}
        if(*decision==admission::reuse) {
            if(!previous) throw std::logic_error("policy tried to reuse a missing task");
            ++duplicates;return previous->current;
        }
        if(exhausted_) return std::unexpected(error::id_exhausted);
        // Guard teaching-policy mistakes from turning a failed exercise into an
        // unbounded allocation. Correct policy still determines observable errors.
        if(live_>=bound_.live || records_.size()>=bound_.records) return std::unexpected(error::overloaded);
        const auto budget=std::chrono::duration_cast<clock::duration>(milliseconds(input.budget_ms));
        if(now>time::max()-budget) return std::unexpected(error::invalid);
        auto signal=std::make_shared<control>();
        records_.push_back({input,std::string(key),{next_,state::queued,{},0,0,false},now+budget,{},std::move(signal),{}});
        ++live_;++accepted;
        if(next_==bound_.id_ceiling) exhausted_=true;else ++next_;
        return records_.back().current;
    }
    result<snapshot> get(id value,time now) {tick(now);auto* r=find(value);if(!r)return std::unexpected(error::not_found);return r->current;}
    std::optional<work> start_next(time now) {
        tick(now);
        for(auto& r:records_) if(r.current.phase==state::queued) {
            r.current.phase=state::running;publish(r);return work{r.current.task,r.input,r.signal};
        }
        return {};
    }
    result<snapshot> cancel(id value,time now) {
        tick(now);auto* r=find(value);if(!r)return std::unexpected(error::not_found);
        stop_record(*r,state::cancelled,now);return r->current;
    }
    result<snapshot> finish(id value,completion outcome,time now) {
        tick(now);auto* r=find(value);if(!r)return std::unexpected(error::not_found);
        if(terminal(r->current.phase)) return r->current;
        if(r->current.phase!=state::running) return std::unexpected(error::invalid);
        const auto selected=Policy::finish(r->stop_reason,!outcome);
        if(!terminal(selected)) throw std::logic_error("policy must produce a terminal state");
        r->current.phase=selected;
        if(selected==state::succeeded && outcome) r->current.value=*outcome;
        r->ended=now;--live_;++completed;publish(*r);return r->current;
    }
    void begin_drain() noexcept {draining_=true;}
    void cancel_all(time now) {draining_=true;for(auto& r:records_) stop_record(r,state::cancelled,now);}
    std::size_t live() const noexcept {return live_;}
    std::size_t retained() const noexcept {return records_.size();}
    std::size_t running() const {return static_cast<std::size_t>(std::count_if(records_.begin(),records_.end(),[](const record& r){return r.current.phase==state::running;}));}
    result<id> subscribe(id task,time now,std::shared_ptr<std::atomic_bool> owner_alive={}) {
        tick(now);auto* r=find(task);if(!r)return std::unexpected(error::not_found);
        if(watches_.size()>=bound_.watches)return std::unexpected(error::overloaded);
        if(watch_exhausted_)return std::unexpected(error::id_exhausted);
        watch w{next_watch_,task,{r->current},false,std::move(owner_alive)};
        watches_.push_back(std::move(w));
        const auto result=next_watch_;
        if(next_watch_==bound_.id_ceiling) watch_exhausted_=true;else ++next_watch_;
        return result; // Snapshot+registration are atomic with respect to the owner.
    }
    result<std::optional<snapshot>> next_event(id subscription) {
        auto it=std::find_if(watches_.begin(),watches_.end(),[&](const watch& w){return w.identity==subscription;});
        if(it==watches_.end())return std::unexpected(error::not_found);
        if(it->overflow) {watches_.erase(it);return std::unexpected(error::slow_consumer);}
        if(it->events.empty())return std::optional<snapshot>{};
        auto value=it->events.front();it->events.pop_front();
        if(terminal(value.phase))watches_.erase(it);
        return std::optional<snapshot>{value};
    }
    void unsubscribe(id subscription) {std::erase_if(watches_,[&](const watch& w){return w.identity==subscription;});}
    std::size_t subscriptions() const noexcept{return watches_.size();}
};
}
