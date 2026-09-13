#pragma once
#include "tasks.grpc.pb.h"
#include <c11/task_runtime.hpp>
#include <c11/task_policy.hpp>
#include <grpcpp/grpcpp.h>
#include <semaphore>
#include <condition_variable>

namespace c11::grpc_service {
using runtime=tasks::runtime<tasks::task_policy>;
inline grpc::Status status(tasks::error error) {
    using code=grpc::StatusCode;
    switch(error) {
    case tasks::error::invalid:return {code::INVALID_ARGUMENT,"invalid task"};
    case tasks::error::conflict:return {code::ALREADY_EXISTS,"idempotency key conflict"};
    case tasks::error::not_found:return {code::NOT_FOUND,"task not found"};
    case tasks::error::request_cancelled:return {code::CANCELLED,"caller abandoned request"};
    case tasks::error::request_timeout:return {code::DEADLINE_EXCEEDED,"request budget exhausted"};
    case tasks::error::shutting_down:return {code::UNAVAILABLE,"service stopping"};
    default:return {code::RESOURCE_EXHAUSTED,"resource limit"};
    }
}
inline void assign(rpc::TaskSnapshot& output,const tasks::snapshot& source) {
    output.set_id(source.task);output.set_state(std::string(tasks::state_name(source.phase)));
    output.set_progress(source.progress);output.set_revision(source.revision);output.set_stop_requested(source.stop_requested);
    if(source.value)output.set_result(*source.value);else output.clear_result();
}
template<int Count> struct permit {
    std::counting_semaphore<Count>& semaphore;bool acquired;
    explicit permit(std::counting_semaphore<Count>& value):semaphore(value),acquired(value.try_acquire()){}
    ~permit(){if(acquired)semaphore.release();}
};
class service final:public rpc::Tasks::Service {
    boost::asio::io_context& owner_;std::shared_ptr<runtime> runtime_;
    std::counting_semaphore<128> calls_{128};std::counting_semaphore<32> watches_{32};
    template<class F> auto invoke(grpc::ServerContext& context,F action) {
        using result=decltype(action(*runtime_));
        try {
            auto active=std::make_shared<std::atomic_bool>(true);
            std::packaged_task<result()> work([engine=runtime_,active,action=std::move(action)]()mutable->result{
                if(!active->load(std::memory_order_relaxed))return std::unexpected(tasks::error::request_cancelled);
                return action(*engine);
            });
            auto future=work.get_future();
            boost::asio::post(owner_,[work=std::move(work)]()mutable{work();});
            const auto deadline=tasks::clock::now()+std::chrono::seconds(5);
            while(future.wait_for(std::chrono::milliseconds(5))!=std::future_status::ready) {
                if(context.IsCancelled()){*active=false;return result(std::unexpected(tasks::error::request_cancelled));}
                if(tasks::clock::now()>=deadline){*active=false;return result(std::unexpected(tasks::error::request_timeout));}
            }
            return future.get();
        }catch(const std::bad_alloc&){return result(std::unexpected(tasks::error::overloaded));}
    }
public:
    service(boost::asio::io_context& owner,std::shared_ptr<runtime> engine):owner_(owner),runtime_(std::move(engine)){}
    grpc::Status Submit(grpc::ServerContext* context,const rpc::SubmitRequest* request,rpc::TaskSnapshot* reply) override {
        permit guard(calls_);if(!guard.acquired)return status(tasks::error::overloaded);
        if(!request->has_left() || !request->has_right())return status(tasks::error::invalid);
        const tasks::spec input{request->left(),request->right(),request->steps(),request->has_budget_ms()?request->budget_ms():5000};
        auto value=invoke(*context,[input,key=request->idempotency_key()](runtime& engine){return engine.submit(input,key);});
        if(!value)return status(value.error());assign(*reply,*value);return grpc::Status::OK;
    }
    grpc::Status Get(grpc::ServerContext* context,const rpc::TaskId* request,rpc::TaskSnapshot* reply) override {
        permit guard(calls_);if(!guard.acquired)return status(tasks::error::overloaded);
        auto value=invoke(*context,[id=request->id()](runtime& engine){return engine.registry.get(id,tasks::clock::now());});
        if(!value)return status(value.error());assign(*reply,*value);return grpc::Status::OK;
    }
    grpc::Status Cancel(grpc::ServerContext* context,const rpc::TaskId* request,rpc::TaskSnapshot* reply) override {
        permit guard(calls_);if(!guard.acquired)return status(tasks::error::overloaded);
        auto value=invoke(*context,[id=request->id()](runtime& engine){return engine.registry.cancel(id,tasks::clock::now());});
        if(!value)return status(value.error());assign(*reply,*value);return grpc::Status::OK;
    }
    grpc::Status Watch(grpc::ServerContext* context,const rpc::TaskId* request,grpc::ServerWriter<rpc::TaskSnapshot>* writer) override {
        permit calls(calls_);permit watching(watches_);
        if(!calls.acquired || !watching.acquired)return status(tasks::error::overloaded);
        auto alive=std::make_shared<std::atomic_bool>(true);
        struct lifetime {std::shared_ptr<std::atomic_bool> alive;~lifetime(){*alive=false;}} lifetime_guard{alive};
        auto subscription=invoke(*context,[id=request->id(),alive](runtime& engine){return engine.registry.subscribe(id,tasks::clock::now(),alive);});
        if(!subscription)return status(subscription.error());
        std::mutex mutex;std::condition_variable_any timer;
        // ponytail: one deadline guard thread per synchronous stream, capped32.
        // Use callback reactors/alarms when scaling beyond this teaching bound.
        std::jthread deadline([&](std::stop_token stop){
            std::unique_lock lock(mutex);timer.wait_for(lock,stop,std::chrono::seconds(5),[]{return false;});
            if(!stop.stop_requested())context->TryCancel();
        }); // Joined before ServerContext can be destroyed.
        while(!context->IsCancelled()) {
            auto event=invoke(*context,[id=*subscription](runtime& engine){return engine.registry.next_event(id);});
            if(!event)return status(event.error());
            if(*event) {
                rpc::TaskSnapshot output;assign(output,**event);
                if(!writer->Write(output))return status(tasks::error::request_cancelled);
                if(tasks::terminal((**event).phase))return grpc::Status::OK;
            }else std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Stream polling interval, not an ordering assertion.
        }
        return status(tasks::error::request_cancelled);
    }
};
}
