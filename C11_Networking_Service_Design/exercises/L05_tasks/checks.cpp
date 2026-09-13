#include <solution.hpp>
#include <c11/task_registry.hpp>
#include <check.hpp>
#include <iostream>
using namespace c11::tasks;
using namespace std::chrono_literals;
int main() {
  try {
    const c11::tasks::time now{};limits small;small.live=2;small.records=3;small.retention=100ms;
    basic_registry<exercise::policy> tasks(small);
    const spec input{20,22,3,5000};
    auto a=tasks.submit(input,"same-key",now);check(a.has_value(),"first task accepted");
    auto duplicate=tasks.submit(input,"same-key",now);
    check(duplicate && duplicate->task==a->task && tasks.live()==1 && tasks.accepted==1,"lost response retry reuses accepted task");
    auto conflict=tasks.submit(spec{21,22,3,5000},"same-key",now);
    check(!conflict && conflict.error()==error::conflict,"same key different input conflicts");
    check(!tasks.submit(input,"bad key",now),"untrusted key validation");
    for(const spec invalid:{spec{-1000001,0,1,1},spec{0,1000001,1,1},spec{0,0,1001,1},spec{0,0,1,0},spec{0,0,1,60001}}) {
        auto rejected=tasks.submit(invalid,"invalid",now);
        check(!rejected && rejected.error()==error::invalid,"numeric workload and time budgets validated before admission");
    }
    auto b=tasks.submit(input,"second",now);check(b.has_value(),"second task accepted");
    check(!tasks.submit(input,"overflow",now),"live quota rejects overload");
    auto wa=tasks.start_next(now),wb=tasks.start_next(now);check(wa && wb && !tasks.start_next(now),"only accepted queued work starts");
    auto watch=tasks.subscribe(a->task,now);check(watch.has_value(),"watch registered with initial snapshot");
    auto first=tasks.next_event(*watch);check(first && *first && (**first).phase==state::running,"watch starts with current state");
    check(tasks.cancel(a->task,now).has_value() && wa->signal->stop.stop_requested(),"worker receives cooperative stop");
    check(tasks.live()==2 && !tasks.submit(input,"still-full",now),"running cancel retains live quota until acknowledgement");
    auto finished=tasks.finish(a->task,42,now+1ms);
    check(finished && finished->phase==state::cancelled && !finished->value,"cancellation wins before worker acknowledgement");
    check(tasks.live()==1,"acknowledged worker releases quota once");
    check(tasks.finish(a->task,999,now+2ms)->phase==state::cancelled && tasks.live()==1,"duplicate completion cannot rewrite terminal or release twice");
    std::uint32_t revision=0;bool terminal_seen=false;
    while(auto e=tasks.next_event(*watch)) {
        if(!*e)break;
        check((**e).revision>revision,"watch changes ordered by owner revision");revision=(**e).revision;
        if(terminal((**e).phase)){terminal_seen=true;break;}
    }
    check(terminal_seen && tasks.subscriptions()==0,"terminal watch ends and releases subscription");
    auto third=tasks.submit(input,"third",now+2ms);check(third.has_value(),"remaining reserved record admitted");
    check(tasks.finish(b->task,42,now+3ms).has_value(),"second worker completes");
    check(tasks.cancel(third->task,now+4ms)->phase==state::cancelled,"queued cancellation finishes without physical worker");
    check(tasks.live()==0 && tasks.retained()==3 && !tasks.submit(input,"record-full",now+5ms),"terminal storage reserved at acceptance and not evicted early");
    tasks.tick(now+105ms);check(tasks.retained()==0,"only expired terminal records reaped");
    auto fresh=tasks.submit(input,"same-key",now+106ms);check(fresh && fresh->task!=a->task,"deduplication window is finite and process-local");
    basic_registry<exercise::policy> deadlines;
    auto d=deadlines.submit(spec{1,2,1,1},"d",now);check(d.has_value(),"deadline task accepted");
    auto dw=deadlines.start_next(now);check(dw.has_value(),"deadline worker started");
    deadlines.tick(now+2ms);check(deadlines.live()==1 && dw->signal->stop.stop_requested(),"expired running task retains physical ownership");
    check(deadlines.finish(d->task,3,now+3ms)->phase==state::expired,"task deadline includes queue/execution time");
    limits slow;slow.watch_queue=2;basic_registry<exercise::policy> subscriptions(slow);
    auto s=subscriptions.submit(input,"s",now);auto subscription=subscriptions.subscribe(s->task,now);auto worker=subscriptions.start_next(now);
    worker->signal->progress=1;subscriptions.tick(now+1ms);
    auto lag=subscriptions.next_event(*subscription);check(!lag && lag.error()==error::slow_consumer,"slow subscription closes without unbounded event buffering");
    check(subscriptions.live()==1 && !worker->signal->stop.stop_requested(),"slow watch does not cancel task");
    auto alive=std::make_shared<std::atomic_bool>(true);auto abandoned=subscriptions.subscribe(s->task,now,alive);
    check(abandoned.has_value(),"cross-thread subscription lifetime registered");
    alive->store(false,std::memory_order_relaxed);subscriptions.tick(now+1ms);
    check(subscriptions.subscriptions()==0 && subscriptions.live()==1,"abandoned consumer is reclaimed without a throwing cleanup post");
    subscriptions.begin_drain();check(!subscriptions.submit(input,"new",now),"shutdown stops new admission");
    check(subscriptions.submit(input,"s",now).has_value(),"retry may recover already accepted task during drain");
    subscriptions.cancel_all(now);check(subscriptions.live()==1,"shutdown cancellation waits for worker");
    check(subscriptions.finish(s->task,42,now+1ms).has_value() && subscriptions.live()==0,"shutdown work converges");
    limits ids;ids.id_ceiling=1;basic_registry<exercise::policy> exhausted(ids);
    check(exhausted.submit(input,"one",now).has_value(),"last representable configured id accepted");
    auto no_id=exhausted.submit(input,"two",now);check(!no_id && no_id.error()==error::id_exhausted,"id allocation refuses reuse instead of wrapping");
    basic_registry<exercise::policy> failed;
    auto f=failed.submit(input,"fail",now);failed.start_next(now);
    check(failed.finish(f->task,std::unexpected(worker_error::computation_failed),now)->phase==state::failed,"worker failure follows explicit channel");
    std::cout<<"task admission, idempotency, cancellation, watch and drain checks passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
