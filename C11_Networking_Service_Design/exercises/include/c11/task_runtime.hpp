#pragma once
#include <c11/task_registry.hpp>
#include <boost/asio.hpp>
#include <future>
#include <functional>
#include <thread>
#include <array>
#include <mutex>
#include <condition_variable>

namespace c11::tasks {
template<class Policy> class runtime : public std::enable_shared_from_this<runtime<Policy>> {
    struct done {id task;completion result;};
    struct slot {bool busy=false;std::optional<work> job;std::optional<done> result;};
    std::array<slot,2> slots_;
    std::mutex mutex_;
    std::condition_variable_any changed_;
    std::array<std::jthread,2> workers_;
    boost::asio::steady_timer timer_;
    std::shared_future<void> fixture_gate_;
    bool stopping_=false,started_=false,joined_=false;
    time stop_deadline_{};
    std::function<void()> on_drained_;
    void notify_drained(){auto completed=std::move(on_drained_);if(completed)completed();}
    void worker(std::size_t index,std::stop_token stop) {
        for(;;) {
            std::optional<work> next;
            {
                std::unique_lock lock(mutex_);
                changed_.wait(lock,stop,[&]{return slots_[index].job.has_value();});
                if(stop.stop_requested() && !slots_[index].job)return;
                next=std::move(slots_[index].job);slots_[index].job.reset();
            }
            completion result=std::unexpected(worker_error::computation_failed);
            try {
                // Gate is a controlled local fixture, never a request field.
                if(fixture_gate_.valid())while(fixture_gate_.wait_for(milliseconds(5))!=std::future_status::ready && !stop.stop_requested()){}
                for(std::uint32_t i=0;i<next->input.steps && !next->signal->stop.stop_requested() && !stop.stop_requested();++i) {
                    std::this_thread::sleep_for(milliseconds(1)); // Declared simulated work.
                    next->signal->progress.store(i+1,std::memory_order_relaxed);
                }
                result=static_cast<std::int64_t>(next->input.left)+next->input.right;
            }catch(...){result=std::unexpected(worker_error::computation_failed);}
            {
                std::lock_guard lock(mutex_);
                slots_[index].result=done{next->task,result}; // No allocation and no throwing owner post.
            }
            if(stop.stop_requested())return;
        }
    }
    void collect() {
        std::lock_guard lock(mutex_);
        for(auto& slot:slots_)if(slot.result) {
            (void)registry.finish(slot.result->task,slot.result->result,clock::now());
            slot.result.reset();slot.busy=false;
        }
    }
    void join_workers() {
        for(auto& worker:workers_)worker.request_stop();
        changed_.notify_all();
        for(auto& worker:workers_)if(worker.joinable())worker.join();
        joined_=true;
    }
    void emergency_stop() {
        stopping_=true;registry.cancel_all(clock::now());
        join_workers();
        try{collect();}catch(...){if(!failure)failure=std::current_exception();}
        notify_drained();
    }
    void pump() {
        std::lock_guard lock(mutex_);
        for(auto& slot:slots_)if(!slot.busy) {
            auto next=registry.start_next(clock::now());if(!next)break;
            slot.job=std::move(next);slot.busy=true; // Transfer into an existing slot cannot allocate.
        }
        changed_.notify_all();
    }
    void poll() {
        auto self=this->shared_from_this();
        timer_.expires_after(milliseconds(5));
        timer_.async_wait([self](boost::system::error_code error) {
            if(error)return;
            try {
                if(self->stopping_ && clock::now()>=self->stop_deadline_)self->deadline_exceeded=true;
                self->registry.tick(clock::now());self->collect();
                if(self->stopping_ && clock::now()>=self->stop_deadline_ && self->registry.live()!=0) {
                    self->deadline_exceeded=true;self->registry.cancel_all(clock::now());
                }
                self->pump();
                if(self->stopping_ && self->registry.live()==0) {
                    self->join_workers();
                    if(clock::now()>=self->stop_deadline_)self->deadline_exceeded=true;
                    self->notify_drained();
                }else self->poll();
            }catch(...){self->failure=std::current_exception();self->emergency_stop();}
        });
    }
public:
    basic_registry<Policy> registry;
    bool deadline_exceeded=false;
    std::exception_ptr failure;
    explicit runtime(boost::asio::io_context& owner,limits bound={},std::shared_future<void> gate={})
        :timer_(owner),fixture_gate_(std::move(gate)),registry(bound){}
    ~runtime(){join_workers();}
    void start() {
        if(started_)return;
        joined_=false;
        try{
            for(std::size_t i=0;i<workers_.size();++i)workers_[i]=std::jthread([this,i](std::stop_token stop){worker(i,stop);});
            poll();started_=true;
        }catch(...){join_workers();throw;}
    }
    result<snapshot> submit(const spec& input,std::string_view key) {
        if(!started_ || failure)return std::unexpected(error::shutting_down);
        auto value=registry.submit(input,key,clock::now());if(value)pump();return value;
    }
    void stop(std::function<void()> drained) {
        if(joined_){if(drained)drained();return;}
        if(stopping_)return;
        stopping_=true;on_drained_=std::move(drained);registry.begin_drain();
        stop_deadline_=clock::now()+std::chrono::seconds(5);
    }
    bool joined()const noexcept{return joined_;}
};
}
